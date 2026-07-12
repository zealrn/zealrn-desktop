// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include <core/application.h>
#include <core/applicationsingleton.h>
#include <core/docsetstorage.h>
#include <core/httpserver.h>
#include <core/settings.h>
#include <registry/searchquery.h>
#include <ui/widgets/proxystyle.h>
#include <ui/windowmanager.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QColor>
#include <QDataStream>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTextStream>
#include <QTimer>
#include <QUrlQuery>

#ifdef Q_OS_WINDOWS
#include <QAbstractNativeEventFilter>
#include <qt_windows.h>

#include <utility> // for std::ignore
#endif

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>

namespace {

// Qt WebEngine is Chromium, which refuses to connect to a fixed set of ports;
// a server bound to one would be unreachable from the embedded browser. Ports
// below 1024 are excluded by the unprivileged-range check, so only the
// restricted ports at or above 1024 need listing here.
// See net/base/port_util.cc (kRestrictedPorts) in Chromium.
constexpr std::array<quint16, 17> BrowserRestrictedPorts =
    {1719, 1720, 1723, 2049, 3659, 4045, 5060, 5061, 6000, 6566, 6665, 6666, 6667, 6668, 6669, 6697, 10080};

bool isDarkPalette(const QPalette &palette)
{
    return palette.color(QPalette::WindowText).lightness() > palette.color(QPalette::Window).lightness();
}

QPalette fallbackPalette(bool dark)
{
    const QColor window = dark ? QColor(53, 53, 53) : QColor(240, 240, 240);
    const QColor base = dark ? QColor(35, 35, 35) : QColor(Qt::white);
    const QColor alternateBase = dark ? QColor(45, 45, 45) : QColor(245, 245, 245);
    const QColor text = dark ? QColor(242, 242, 242) : QColor(32, 32, 32);
    const QColor disabledText = dark ? QColor(150, 150, 150) : QColor(112, 112, 112);
    const QColor placeholderText = dark ? QColor(170, 170, 170) : QColor(96, 96, 96);
    const QColor highlight = dark ? QColor(42, 130, 218) : QColor(48, 140, 198);

    QPalette palette(window);
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, alternateBase);
    palette.setColor(QPalette::ToolTipBase, base);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, window);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, dark ? QColor(93, 169, 233) : QColor(0, 102, 204));
    palette.setColor(QPalette::LinkVisited, dark ? QColor(183, 132, 214) : QColor(85, 26, 139));
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::PlaceholderText, placeholderText);
    palette.setColor(QPalette::Mid, dark ? QColor(75, 75, 75) : QColor(184, 184, 184));

    for (const auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        palette.setColor(QPalette::Disabled, role, disabledText);
    }
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, disabledText);

    return palette;
}

void applyApplicationAppearance(const Zeal::Core::Settings &settings, const QString &systemStyleName)
{
    QStyle *baseStyle = nullptr;
#if defined(Q_OS_WINDOWS) && (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
    const auto osName = QSysInfo::prettyProductName();
    if (osName.startsWith("Windows 10") || osName.startsWith("Windows 11")) {
        baseStyle = QStyleFactory::create(QStringLiteral("fusion"));
    }
#endif
    if (baseStyle == nullptr) {
        baseStyle = QStyleFactory::create(systemStyleName);
    }

    QApplication::setStyle(new Zeal::WidgetUi::ProxyStyle(baseStyle));
    QApplication::setPalette(QApplication::style()->standardPalette());

    if (settings.contentAppearance == Zeal::Core::Settings::ContentAppearance::Automatic) {
        return;
    }

    const bool dark = settings.isDarkModeEnabled();
    if (isDarkPalette(QApplication::palette()) == dark) {
        return;
    }

    QApplication::setStyle(new Zeal::WidgetUi::ProxyStyle(QStyleFactory::create(QStringLiteral("fusion"))));
    QApplication::setPalette(fallbackPalette(dark));
}

#if defined(Q_OS_WINDOWS) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
// Windows fills new window client areas with COLOR_WINDOW (always white, even in dark mode)
// via WM_ERASEBKGND before Qt gets to paint. This filter intercepts the message and fills
// with the current palette color instead, preventing a white flash on window creation.
// See QTBUG-106583 and https://codereview.qt-project.org/c/qt/qtbase/+/440060 (reverted).
class DarkModeEraseFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override
    {
        Q_UNUSED(eventType)
        auto *msg = static_cast<MSG *>(message);
        if (msg->message == WM_ERASEBKGND) {
            const QColor bg = QApplication::palette().color(QPalette::Window);
            // Canonical Win32 pattern: WM_ERASEBKGND's wParam encodes the device-context handle.
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
            auto *hdc = reinterpret_cast<HDC>(msg->wParam);
            RECT rc;
            GetClientRect(msg->hwnd, &rc);
            HBRUSH brush = CreateSolidBrush(RGB(bg.red(), bg.green(), bg.blue()));
            FillRect(hdc, &rc, brush);
            DeleteObject(brush);
            *result = 1;

            // Remove ourselves after the first paint — only needed for startup.
            QApplication::instance()->removeNativeEventFilter(this);

            return true;
        }

        return false;
    }
};
#endif

struct CommandLineParameters
{
    bool forceMinimized = false;
    bool preventActivation = false;

    quint16 httpServerPort = 0;

#ifdef Q_OS_WINDOWS
    bool registerProtocolHandlers = false;
    bool unregisterProtocolHandlers = false;
#endif

    Zeal::Registry::SearchQuery query;
};

QString stripParameterUrl(const QString &url, const QString &scheme)
{
    QString str = url.mid(scheme.length() + 1);

    if (str.startsWith(QLatin1String("//"))) {
        str = str.mid(2);
    }

    if (str.endsWith(QLatin1Char('/'))) {
        str = str.left(str.length() - 1);
    }

    return str;
}

// Reports a fatal startup error both on the console and in a dialog, so it is
// visible whether Zeal was launched from a terminal or a desktop shortcut.
void showStartupError(const QString &message)
{
    QTextStream(stderr) << message << '\n';
    QMessageBox::critical(nullptr, QObject::tr("ZealRN"), message);
}

void configureInitialDocsetStorage()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("docsets"));
    const bool configured = settings.contains(QStringLiteral("path"));
    settings.endGroup();
    if (configured) {
        return;
    }

    const QStringList existingPaths = Zeal::Core::existingZealDocsetPaths(QDir::homePath());
    if (existingPaths.isEmpty()) {
        return;
    }

    QMessageBox prompt;
    prompt.setWindowTitle(QObject::tr("ZealRN Documentation"));
    prompt.setIcon(QMessageBox::Question);
    prompt.setText(QObject::tr("Reuse your existing Zeal documentation library?"));
    prompt.setInformativeText(existingPaths.first());
    QAbstractButton *reuseButton = prompt.addButton(QObject::tr("Reuse Library"), QMessageBox::AcceptRole);
    QAbstractButton *chooseButton = prompt.addButton(QObject::tr("Choose Another Directory"), QMessageBox::ActionRole);
    prompt.addButton(QObject::tr("Start Empty"), QMessageBox::RejectRole);
    prompt.exec();

    QString selectedPath;
    bool readOnly = false;
    if (prompt.clickedButton() == reuseButton) {
        selectedPath = existingPaths.first();
        readOnly = true;
    } else if (prompt.clickedButton() == chooseButton) {
        selectedPath = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Choose Documentation Directory"));
    }
    if (selectedPath.isEmpty()) {
        return;
    }

    settings.beginGroup(QStringLiteral("docsets"));
    settings.setValue(QStringLiteral("path"), selectedPath);
    settings.setValue(QStringLiteral("read_only"), readOnly);
    settings.endGroup();
    settings.sync();
}

CommandLineParameters parseCommandLine(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("ZealRN - Offline documentation learning tool."));
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions({{QStringLiteral("minimized"), QObject::tr("Start minimized regardless of settings.")},
                       {QStringLiteral("port"),
                        QObject::tr("Bind the local documentation server to a fixed port."),
                        QObject::tr("port")}});

#ifdef Q_OS_WINDOWS
    // --attach-console is acted on at the top of main() (before any parsing) so that
    // --version/--help output is visible; it is registered here only so --help lists
    // it and process() accepts it.
    parser.addOptions({{QStringLiteral("attach-console"), QObject::tr("Attach console for logging.")},
                       {QStringLiteral("register"), QObject::tr("Register protocol handlers.")},
                       {QStringLiteral("unregister"), QObject::tr("Unregister protocol handlers.")}});
#endif

    parser.addPositionalArgument(QStringLiteral("url"), QObject::tr("dash[-plugin]:// URL"));
    parser.process(arguments);

    CommandLineParameters clParams;
    clParams.forceMinimized = parser.isSet(QStringLiteral("minimized"));
    clParams.preventActivation = false;

    if (parser.isSet(QStringLiteral("port"))) {
        const QString value = parser.value(QStringLiteral("port"));
        bool ok = false;
        const uint port = value.toUInt(&ok);
        const bool restricted = std::ranges::find(BrowserRestrictedPorts, port) != BrowserRestrictedPorts.cend();
        if (!ok || port < 1024 || port > std::numeric_limits<quint16>::max() || restricted) {
            showStartupError(QObject::tr("Invalid port: %1. Specify an unprivileged port (1024-65535) that the "
                                         "browser permits.")
                                 .arg(value));
            ::exit(EXIT_FAILURE);
        }
        clParams.httpServerPort = static_cast<quint16>(port);
    }

#ifdef Q_OS_WINDOWS
    clParams.registerProtocolHandlers = parser.isSet(QStringLiteral("register"));
    clParams.unregisterProtocolHandlers = parser.isSet(QStringLiteral("unregister"));

    if (clParams.registerProtocolHandlers && clParams.unregisterProtocolHandlers) {
        QTextStream(stderr) << QObject::tr("Parameter conflict: --register and --unregister.\n");
        ::exit(EXIT_FAILURE);
    }
#endif

    // TODO: Support dash-feed:// protocol
    const QString arg = QUrl::fromPercentEncoding(parser.positionalArguments().value(0).toUtf8());

    if (arg.startsWith(QLatin1String("dash:"))) {
        clParams.query.setQuery(stripParameterUrl(arg, QStringLiteral("dash")));
    } else if (arg.startsWith(QLatin1String("dash-plugin:"))) {
        const QUrlQuery urlQuery(stripParameterUrl(arg, QStringLiteral("dash-plugin")));

        const QString keys = urlQuery.queryItemValue(QStringLiteral("keys"));
        if (!keys.isEmpty()) {
            clParams.query.setKeywords(keys.split(QLatin1Char(',')));
        }

        clParams.query.setQuery(urlQuery.queryItemValue(QStringLiteral("query")));

        const QString preventActivation = urlQuery.queryItemValue(QStringLiteral("prevent_activation"));
        clParams.preventActivation = preventActivation == QLatin1String("true");
    } else {
        clParams.query.setQuery(arg);
    }

    return clParams;
}

#ifdef Q_OS_WINDOWS
void registerProtocolHandler(const QString &scheme, const QString &description)
{
    const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString regPath = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\") + scheme;

    QSettings registry(regPath, QSettings::NativeFormat);

    registry.setValue(QStringLiteral("Default"), description);
    registry.setValue(QStringLiteral("URL Protocol"), QString());

    registry.beginGroup(QStringLiteral("DefaultIcon"));
    registry.setValue(QStringLiteral("Default"), QString("%1,1").arg(appPath));
    registry.endGroup();

    registry.beginGroup(QStringLiteral("shell"));
    registry.beginGroup(QStringLiteral("open"));
    registry.beginGroup(QStringLiteral("command"));
    registry.setValue(QStringLiteral("Default"), QVariant(appPath + QLatin1String(" %1")));
}

void registerProtocolHandlers(const QHash<QString, QString> &protocols, bool force = false)
{
    const QString regPath = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes");
    const QSettings registry(regPath, QSettings::NativeFormat);

    const QStringList groups = registry.childGroups();
    for (auto it = protocols.cbegin(); it != protocols.cend(); ++it) {
        if (force || !groups.contains(it.key())) {
            registerProtocolHandler(it.key(), it.value());
        }
    }
}

void unregisterProtocolHandlers(const QHash<QString, QString> &protocols)
{
    const QString regPath = QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes");
    QSettings registry(regPath, QSettings::NativeFormat);

    for (auto it = protocols.cbegin(); it != protocols.cend(); ++it) {
        registry.remove(it.key());
    }
}
#endif

} // namespace

int main(int argc, char *argv[])
{
    // Do not allow Qt version lower than the app was compiled with.
    QT_REQUIRE_VERSION(argc, argv, QT_VERSION_STR)

    QCoreApplication::setApplicationName(QStringLiteral("ZealRN"));
    QCoreApplication::setApplicationVersion(ZEAL_VERSION);
    QCoreApplication::setOrganizationDomain(QStringLiteral("abnzrdev.github.io"));
    QCoreApplication::setOrganizationName(QStringLiteral("abnzrdev"));

    // Handle --version (and --attach-console) before creating QApplication to avoid
    // initializing the platform/graphics stack just to print a version string.
    {
        const QCoreApplication coreApp(argc, argv);

#ifdef Q_OS_WINDOWS
        // Attach to the parent console before any output so --version/--help (and
        // runtime logging) are visible when launched from a terminal; this must
        // precede the version/help handling, which prints and exits.
        if (QCoreApplication::arguments().contains(QStringLiteral("--attach-console"))) {
            if (AttachConsole(ATTACH_PARENT_PROCESS) != 0) {
                FILE *fp = nullptr;
                std::ignore = freopen_s(&fp, "CONOUT$", "w", stdout);
                std::ignore = freopen_s(&fp, "CONOUT$", "w", stderr);
                std::ignore = freopen_s(&fp, "CONIN$", "r", stdin);
            }
        }
#endif

        QCommandLineParser parser;
        parser.addVersionOption();
        parser.parse(QCoreApplication::arguments());
        if (parser.isSet(QStringLiteral("version"))) {
            parser.showVersion();
        }
    }

    QApplication qapp(argc, argv);
    const QString systemStyleName = QApplication::style()->objectName();

#if defined(Q_OS_WINDOWS) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    DarkModeEraseFilter darkModeEraseFilter;
#endif

    const CommandLineParameters clParams = parseCommandLine(QApplication::arguments());

#ifdef Q_OS_WINDOWS
    const static QHash<QString, QString> protocols = {{QStringLiteral("dash"),
                                                       QStringLiteral("URL:Dash Protocol (Zeal)")},
                                                      {QStringLiteral("dash-plugin"),
                                                       QStringLiteral("URL:Dash Plugin Protocol (Zeal)")}};

    if (clParams.registerProtocolHandlers) {
        registerProtocolHandlers(protocols, clParams.registerProtocolHandlers);
        return EXIT_SUCCESS;
    }

    if (clParams.unregisterProtocolHandlers) {
        unregisterProtocolHandlers(protocols);
        return EXIT_SUCCESS;
    }
#endif

    using Zeal::Core::ApplicationSingleton;
    ApplicationSingleton appSingleton;
    if (appSingleton.state() == ApplicationSingleton::State::Failed) {
        QTextStream(stderr) << "Failed to initialize application singleton." << '\n';
        return EXIT_FAILURE;
    }

    if (appSingleton.state() == ApplicationSingleton::State::Secondary) {
#ifdef Q_OS_WINDOWS
        ::AllowSetForegroundWindow(appSingleton.primaryPid());
#endif
        QByteArray ba;
        QDataStream out(&ba, QIODevice::WriteOnly);
        out << clParams.query << clParams.preventActivation;
        if (!appSingleton.sendMessage(ba)) {
            QTextStream(stderr) << "Failed to send query to the primary instance." << '\n';
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // Set application-wide window icon. All message boxes and other windows will use it by default.
    QApplication::setDesktopFileName(QStringLiteral("io.github.abnzrdev.zealrn"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("zealrn"), QIcon(QStringLiteral(":/zeal.svg"))));

    QDir::setSearchPaths(QStringLiteral("typeIcon"), {QStringLiteral(":/icons/type")});

    using Zeal::Core::Application;
    configureInitialDocsetStorage();
    Application app(clParams.httpServerPort);

    const auto applyAppearance = [&app, systemStyleName]() {
        applyApplicationAppearance(*app.settings(), systemStyleName);
    };
    QObject::connect(app.settings(), &Zeal::Core::Settings::updated, &qapp, applyAppearance);
    applyAppearance();

#if defined(Q_OS_WINDOWS) && QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const auto applyWindowsDarkModeFilter = [&app, &qapp, &darkModeEraseFilter]() {
        qapp.removeNativeEventFilter(&darkModeEraseFilter);
        if (app.settings()->isDarkModeEnabled()) {
            qapp.installNativeEventFilter(&darkModeEraseFilter);
        }
    };
    QObject::connect(app.settings(),
                     &Zeal::Core::Settings::updated,
                     &qapp,
                     applyWindowsDarkModeFilter);
    applyWindowsDarkModeFilter();
#endif

    if (!app.httpServer()->isListening()) {
        showStartupError(
            clParams.httpServerPort != 0
                ? QObject::tr("Failed to bind the documentation server to port %1.").arg(clParams.httpServerPort)
                : QObject::tr("Failed to start the documentation server."));
        return EXIT_FAILURE;
    }

    using Zeal::WidgetUi::WindowManager;
    WindowManager wm(&app);

    QObject::connect(&appSingleton, &ApplicationSingleton::messageReceived, &wm, [&wm](const QByteArray &data) {
        Zeal::Registry::SearchQuery query;
        bool preventActivation = false;

        QDataStream in(data);
        in >> query >> preventActivation;

        wm.executeQuery(query, preventActivation);
    });

    wm.openWindow(clParams.forceMinimized);

    if (!clParams.query.isEmpty()) {
        QTimer::singleShot(0, &wm, [&wm, clParams] {
            wm.executeQuery(clParams.query, clParams.preventActivation);
        });
    }

    return QApplication::exec();
}
