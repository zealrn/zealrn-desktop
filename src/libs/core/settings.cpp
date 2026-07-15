// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.h"

#include "application.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSettings>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QUrl>
#include <QUuid>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#else
#include <QPalette>
#endif

namespace Zeal::Core {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.core.settings")

using Qt::Literals::StringLiterals::operator""_L1;

// Configuration file groups
constexpr auto GroupUI = "ui"_L1;
constexpr auto GroupContent = "content"_L1;
constexpr auto GroupDocsets = "docsets"_L1;
constexpr auto GroupGlobalShortcuts = "global_shortcuts"_L1;
constexpr auto GroupSearch = "search"_L1;
constexpr auto GroupTabs = "tabs"_L1;
constexpr auto GroupInternal = "internal"_L1;
constexpr auto GroupProxy = "proxy"_L1;
constexpr auto GroupGettingStarted = "getting_started"_L1;
constexpr auto GroupUpdates = "updates"_L1;

bool boolValue(const QSettings &settings, const QString &key, bool defaultValue)
{
    const QVariant value = settings.value(key);
    if (!value.isValid()) {
        return defaultValue;
    }
    if (value.metaType().id() == QMetaType::Bool) {
        return value.toBool();
    }
    const QString text = value.toString().trimmed().toLower();
    if (text == QLatin1String("true") || text == QLatin1String("1")) {
        return true;
    }
    if (text == QLatin1String("false") || text == QLatin1String("0")) {
        return false;
    }
    return defaultValue;
}
} // namespace

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init): initialized by load().
Settings::Settings(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<ContentAppearance>("ContentAppearance");
    qRegisterMetaType<ExternalLinkPolicy>("ExternalLinkPolicy");

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Notify application and web views when the system appearance changes.
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() {
        if (contentAppearance == ContentAppearance::Automatic) {
            emit updated();
        }
    });
#endif

    load();
    m_startupDarkMode = isDarkModeEnabled();
}

Settings::~Settings()
{
    save();
}

bool Settings::isDarkModeEnabled() const
{
    if (contentAppearance == ContentAppearance::Dark) {
        return true;
    }

    if (contentAppearance == ContentAppearance::Automatic && colorScheme() == ColorScheme::Dark) {
        return true;
    }

    return false;
}

bool Settings::isDocumentationThemeRestartRequired() const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    return m_startupDarkMode != isDarkModeEnabled();
#else
    return false;
#endif
}

bool Settings::isAutomaticUpdateCheckDue(const QDateTime &now) const
{
    if (!checkForUpdate || updateFrequency == UpdateFrequency::Never) {
        return false;
    }
    if (!updateLastAttempt.isValid() || updateLastAttempt > now) {
        return true;
    }

    const qint64 interval = updateFrequency == UpdateFrequency::Weekly ? 7 * 24 * 60 * 60 : 24 * 60 * 60;
    return updateLastAttempt.secsTo(now) >= interval;
}

bool Settings::isTrayActive() const
{
    return showSystrayIcon && QSystemTrayIcon::isSystemTrayAvailable();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
void Settings::applyColorScheme() const
{
    Qt::ColorScheme scheme = Qt::ColorScheme::Unknown;
    switch (contentAppearance) {
    case ContentAppearance::Light:
        scheme = Qt::ColorScheme::Light;
        break;
    case ContentAppearance::Dark:
        scheme = Qt::ColorScheme::Dark;
        break;
    default:
        break;
    }

    // Setting the scheme updates the platform palette. The full widget refresh
    // (re-deriving the palette and re-polishing existing widgets) is handled by
    // re-installing the application style on QStyleHints::colorSchemeChanged; see
    // app/main.cpp. Doing it there preserves the active QProxyStyle wrapper that
    // restyles the tab close button, which re-creating the style here would drop.
    qApp->styleHints()->setColorScheme(scheme);
}
#endif

Settings::ColorScheme Settings::colorScheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QApplication::styleHints()->colorScheme();
#else
    // Pre Qt 6.5 detection from https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5.
    static const ColorScheme scheme = []() {
        const QPalette p;
        return p.color(QPalette::WindowText).lightness() > p.color(QPalette::Window).lightness()
                   ? ColorScheme::Dark
                   : ColorScheme::Light;
    }();
    return scheme;
#endif
}

void Settings::load()
{
    auto settings = qsettings();
    qCDebug(log, "Using settings file: %s", qPrintable(settings->fileName()));
    migrate(settings.get());

    // TODO: Put everything in groups
    startMinimized = settings->value(QStringLiteral("start_minimized"), false).toBool();
#ifdef ZEAL_FEATURE_UPDATE_CHECK
    checkForUpdate = settings->value(QStringLiteral("check_for_update"), true).toBool();
#else
    checkForUpdate = settings->value(QStringLiteral("check_for_update"), false).toBool();
#endif

    settings->beginGroup(GroupUpdates);
    const int updateFrequencyValue = settings->value(QStringLiteral("frequency"),
                                                      static_cast<int>(UpdateFrequency::Daily))
                                         .toInt();
    switch (static_cast<UpdateFrequency>(updateFrequencyValue)) {
    case UpdateFrequency::Daily:
    case UpdateFrequency::Weekly:
    case UpdateFrequency::Never:
        updateFrequency = static_cast<UpdateFrequency>(updateFrequencyValue);
        break;
    default:
        updateFrequency = UpdateFrequency::Daily;
        break;
    }
    updateIncludePrereleases = boolValue(*settings, QStringLiteral("include_prereleases"), false);
    updateLastAttempt = settings->value(QStringLiteral("last_attempt")).toDateTime();
    updateLastSuccess = settings->value(QStringLiteral("last_success")).toDateTime();
    updateEtag = settings->value(QStringLiteral("etag")).toByteArray();
    updateSkippedVersion = settings->value(QStringLiteral("skipped_version")).toString();
    updateCachedVersion = settings->value(QStringLiteral("cached_version")).toString();
    updateCachedTitle = settings->value(QStringLiteral("cached_title")).toString();
    updateCachedPublishedAt = settings->value(QStringLiteral("cached_published_at")).toDateTime();
    updateCachedPageUrl = settings->value(QStringLiteral("cached_page_url")).toString();
    updateCachedPrerelease = boolValue(*settings, QStringLiteral("cached_prerelease"), false);
    settings->endGroup();

    showSystrayIcon = settings->value(QStringLiteral("show_systray_icon"), true).toBool();
    minimizeToSystray = settings->value(QStringLiteral("minimize_to_systray"), false).toBool();
    hideOnClose = settings->value(QStringLiteral("hide_on_close"), false).toBool();

    settings->beginGroup(GroupUI);
    hideMenuBar = settings->value(QStringLiteral("hide_menu_bar"), false).toBool();
    hideSidebar = settings->value(QStringLiteral("hide_sidebar"), false).toBool();
    terminalSafetyAcknowledged = settings->value(QStringLiteral("terminal_safety_acknowledged"), false).toBool();
    terminalStartOnOpen = settings->value(QStringLiteral("terminal_start_on_open"), true).toBool();
    terminalShell = settings->value(QStringLiteral("terminal_shell")).toString();
    terminalWorkingDirectory = settings->value(QStringLiteral("terminal_working_directory")).toString();
    terminalFontSize = qBound(10, settings->value(QStringLiteral("terminal_font_size"), 14).toInt(), 28);
    bottomDevelopmentTool = settings->value(QStringLiteral("bottom_development_tool"), 0).toInt();
    if (bottomDevelopmentTool != 0 && bottomDevelopmentTool != 1) {
        bottomDevelopmentTool = 0;
    }
    learningNotesZoom = qBound(80, settings->value(QStringLiteral("learning_notes_zoom"), 115).toInt(), 200);
    learningNotesLineWrap = settings->value(QStringLiteral("learning_notes_line_wrap"), true).toBool();
    settings->endGroup();

    settings->beginGroup(GroupGlobalShortcuts);
    showShortcut = settings->value(QStringLiteral("show")).value<QKeySequence>();
    settings->endGroup();

    settings->beginGroup(GroupTabs);
    openNewTabAfterActive = settings->value(QStringLiteral("open_new_tab_after_active"), false).toBool();
    showTabCloseButton = settings->value(QStringLiteral("show_tab_close_button"), true).toBool();
    settings->endGroup();

    settings->beginGroup(GroupSearch);
    isFuzzySearchEnabled = settings->value(QStringLiteral("fuzzy_search_enabled"), true).toBool();
    settings->endGroup();

    settings->beginGroup(GroupGettingStarted);
    quickTourShown = boolValue(*settings, QStringLiteral("quick_tour_shown"), false);
    quickTourCompleted = boolValue(*settings, QStringLiteral("quick_tour_completed"), false);
    quickTourNextLaunch = boolValue(*settings, QStringLiteral("quick_tour_next_launch"), false);
    openStartNoteOnLaunch = boolValue(*settings, QStringLiteral("open_start_note"), true);
    openLastDocumentationOnLaunch = boolValue(*settings, QStringLiteral("open_last_documentation"), true);
    lastDocumentationDocsetId = settings->value(QStringLiteral("last_documentation_docset")).toString();
    lastDocumentationPagePath = settings->value(QStringLiteral("last_documentation_page")).toString();
    bool checklistOk = false;
    const qlonglong checklist = settings->value(QStringLiteral("checklist"), 0).toLongLong(&checklistOk);
    gettingStartedChecklist = checklistOk && checklist >= 0 && checklist <= 0x7f ? static_cast<quint32>(checklist) : 0;
    gettingStartedChecklistDismissed = boolValue(*settings, QStringLiteral("checklist_dismissed"), false);
    dismissedHelpTips = settings->value(QStringLiteral("dismissed_help_tips")).toStringList();
    settings->endGroup();

    settings->beginGroup(GroupContent);

    contentAppearance = settings->value(QStringLiteral("appearance"), QVariant::fromValue(ContentAppearance::Automatic))
                            .value<ContentAppearance>();
    switch (contentAppearance) {
    case ContentAppearance::Automatic:
    case ContentAppearance::Light:
    case ContentAppearance::Dark:
        break;
    default:
        contentAppearance = ContentAppearance::Automatic;
        break;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    // Dark mode needs to be applied before Qt WebEngine is initialized.
    if (isDarkModeEnabled()) {
        QByteArray chromiumFlags = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
        if (!chromiumFlags.isEmpty()) {
            chromiumFlags += ' ';
        }

        chromiumFlags += "--blink-settings=forceDarkModeEnabled=true,darkModeInversionAlgorithm=4";
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", chromiumFlags);
    }
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    applyColorScheme();
#endif

    // Fonts
    QWebEngineSettings *webSettings = QWebEngineProfile::defaultProfile()->settings();
    serifFontFamily = settings
                          ->value(QStringLiteral("serif_font_family"),
                                  webSettings->fontFamily(QWebEngineSettings::SerifFont))
                          .toString();
    sansSerifFontFamily = settings
                              ->value(QStringLiteral("sans_serif_font_family"),
                                      webSettings->fontFamily(QWebEngineSettings::SansSerifFont))
                              .toString();
    fixedFontFamily = settings
                          ->value(QStringLiteral("fixed_font_family"),
                                  webSettings->fontFamily(QWebEngineSettings::FixedFont))
                          .toString();

    static const QMap<QString, QWebEngineSettings::FontFamily> fontFamilies =
        {{QStringLiteral("sans-serif"), QWebEngineSettings::SansSerifFont},
         {QStringLiteral("serif"), QWebEngineSettings::SerifFont},
         {QStringLiteral("monospace"), QWebEngineSettings::FixedFont}};

    defaultFontFamily = settings->value(QStringLiteral("default_font_family"), QStringLiteral("serif")).toString();

    // Fallback to the serif font family.
    if (!fontFamilies.contains(defaultFontFamily)) {
        defaultFontFamily = QStringLiteral("serif");
    }

    webSettings->setFontFamily(QWebEngineSettings::SansSerifFont, sansSerifFontFamily);
    webSettings->setFontFamily(QWebEngineSettings::SerifFont, serifFontFamily);
    webSettings->setFontFamily(QWebEngineSettings::FixedFont, fixedFontFamily);

    const QString defaultFontFamilyResolved = webSettings->fontFamily(fontFamilies.value(defaultFontFamily));
    webSettings->setFontFamily(QWebEngineSettings::StandardFont, defaultFontFamilyResolved);

    defaultFontSize = settings
                          ->value(QStringLiteral("default_font_size"),
                                  webSettings->fontSize(QWebEngineSettings::DefaultFontSize))
                          .toInt();
    defaultFixedFontSize = settings
                               ->value(QStringLiteral("default_fixed_font_size"),
                                       webSettings->fontSize(QWebEngineSettings::DefaultFixedFontSize))
                               .toInt();
    minimumFontSize = settings
                          ->value(QStringLiteral("minimum_font_size"),
                                  webSettings->fontSize(QWebEngineSettings::MinimumFontSize))
                          .toInt();

    webSettings->setFontSize(QWebEngineSettings::DefaultFontSize, defaultFontSize);
    webSettings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, defaultFixedFontSize);
    webSettings->setFontSize(QWebEngineSettings::MinimumFontSize, minimumFontSize);

    isHighlightOnNavigateEnabled = settings->value(QStringLiteral("highlight_on_navigate"), true).toBool();
    customCssFile = settings->value(QStringLiteral("custom_css_file")).toString();
    externalLinkPolicy = settings
                             ->value(QStringLiteral("external_link_policy"),
                                     QVariant::fromValue(ExternalLinkPolicy::Ask))
                             .value<ExternalLinkPolicy>();
    isSmoothScrollingEnabled = settings->value(QStringLiteral("smooth_scrolling"), true).toBool();
    settings->endGroup();

    settings->beginGroup(GroupProxy);
    proxyType = settings->value(QStringLiteral("type"), QVariant::fromValue(ProxyType::System)).value<ProxyType>();
    proxyHost = settings->value(QStringLiteral("host")).toString();
    proxyPort = static_cast<quint16>(settings->value(QStringLiteral("port"), 0).toUInt());
    proxyAuthenticate = settings->value(QStringLiteral("authenticate"), false).toBool();
    proxyUserName = settings->value(QStringLiteral("username")).toString();
    proxyPassword = settings->value(QStringLiteral("password")).toString();
    isIgnoreSslErrorsEnabled = settings->value(QStringLiteral("ignore_ssl_errors"), false).toBool();
    settings->endGroup();

    settings->beginGroup(GroupDocsets);
    docsetPathReadOnly = settings->value(QStringLiteral("read_only"), false).toBool();
    if (settings->contains(QStringLiteral("path"))) {
        docsetPath = settings->value(QStringLiteral("path")).toString();
    } else {
#ifndef PORTABLE_BUILD
        docsetPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QLatin1String("/docsets");
#else
        docsetPath = QStringLiteral("docsets");
#endif
    }
    settings->endGroup();

    // Create the docset storage directory if it doesn't exist.
    const QFileInfo fi(docsetPath);
    if (!fi.exists()) {
        const QString path = fi.isRelative() ? QCoreApplication::applicationDirPath() + QLatin1String("/") + docsetPath
                                             : docsetPath;
        if (!QDir().mkpath(path)) {
            qCWarning(log, "Failed to create docset storage directory '%s'.", qPrintable(path));
        }
    }

    settings->beginGroup(GroupInternal);
    installId = settings
                    ->value(QStringLiteral("install_id"),
                            // Avoid curly braces (QTBUG-885)
                            QUuid::createUuid().toString().mid(1, 36))
                    .toString();
    settings->endGroup();
}

void Settings::save()
{
    auto settings = qsettings();

    // TODO: Put everything in groups
    settings->setValue(QStringLiteral("start_minimized"), startMinimized);
    settings->setValue(QStringLiteral("check_for_update"), checkForUpdate);

    settings->beginGroup(GroupUpdates);
    settings->setValue(QStringLiteral("frequency"), static_cast<int>(updateFrequency));
    settings->setValue(QStringLiteral("include_prereleases"), updateIncludePrereleases);
    settings->setValue(QStringLiteral("last_attempt"), updateLastAttempt);
    settings->setValue(QStringLiteral("last_success"), updateLastSuccess);
    settings->setValue(QStringLiteral("etag"), updateEtag);
    settings->setValue(QStringLiteral("skipped_version"), updateSkippedVersion);
    settings->setValue(QStringLiteral("cached_version"), updateCachedVersion);
    settings->setValue(QStringLiteral("cached_title"), updateCachedTitle);
    settings->setValue(QStringLiteral("cached_published_at"), updateCachedPublishedAt);
    settings->setValue(QStringLiteral("cached_page_url"), updateCachedPageUrl);
    settings->setValue(QStringLiteral("cached_prerelease"), updateCachedPrerelease);
    settings->endGroup();

    settings->setValue(QStringLiteral("show_systray_icon"), showSystrayIcon);
    settings->setValue(QStringLiteral("minimize_to_systray"), minimizeToSystray);
    settings->setValue(QStringLiteral("hide_on_close"), hideOnClose);

    settings->beginGroup(GroupUI);
    settings->setValue(QStringLiteral("hide_menu_bar"), hideMenuBar);
    settings->setValue(QStringLiteral("hide_sidebar"), hideSidebar);
    settings->setValue(QStringLiteral("terminal_safety_acknowledged"), terminalSafetyAcknowledged);
    settings->setValue(QStringLiteral("terminal_start_on_open"), terminalStartOnOpen);
    settings->setValue(QStringLiteral("terminal_shell"), terminalShell);
    settings->setValue(QStringLiteral("terminal_working_directory"), terminalWorkingDirectory);
    settings->setValue(QStringLiteral("terminal_font_size"), terminalFontSize);
    settings->setValue(QStringLiteral("bottom_development_tool"), bottomDevelopmentTool);
    settings->setValue(QStringLiteral("learning_notes_zoom"), learningNotesZoom);
    settings->setValue(QStringLiteral("learning_notes_line_wrap"), learningNotesLineWrap);
    settings->endGroup();

    settings->beginGroup(GroupGlobalShortcuts);
    settings->setValue(QStringLiteral("show"), showShortcut);
    settings->endGroup();

    settings->beginGroup(GroupTabs);
    settings->setValue(QStringLiteral("open_new_tab_after_active"), openNewTabAfterActive);
    settings->setValue(QStringLiteral("show_tab_close_button"), showTabCloseButton);
    settings->endGroup();

    settings->beginGroup(GroupSearch);
    settings->setValue(QStringLiteral("fuzzy_search_enabled"), isFuzzySearchEnabled);
    settings->endGroup();

    settings->beginGroup(GroupGettingStarted);
    settings->setValue(QStringLiteral("quick_tour_shown"), quickTourShown);
    settings->setValue(QStringLiteral("quick_tour_completed"), quickTourCompleted);
    settings->setValue(QStringLiteral("quick_tour_next_launch"), quickTourNextLaunch);
    settings->setValue(QStringLiteral("open_start_note"), openStartNoteOnLaunch);
    settings->setValue(QStringLiteral("open_last_documentation"), openLastDocumentationOnLaunch);
    settings->setValue(QStringLiteral("last_documentation_docset"), lastDocumentationDocsetId);
    settings->setValue(QStringLiteral("last_documentation_page"), lastDocumentationPagePath);
    settings->setValue(QStringLiteral("checklist"), gettingStartedChecklist);
    settings->setValue(QStringLiteral("checklist_dismissed"), gettingStartedChecklistDismissed);
    settings->setValue(QStringLiteral("dismissed_help_tips"), dismissedHelpTips);
    settings->endGroup();

    settings->beginGroup(GroupContent);
    settings->setValue(QStringLiteral("default_font_family"), defaultFontFamily);
    settings->setValue(QStringLiteral("serif_font_family"), serifFontFamily);
    settings->setValue(QStringLiteral("sans_serif_font_family"), sansSerifFontFamily);
    settings->setValue(QStringLiteral("fixed_font_family"), fixedFontFamily);

    settings->setValue(QStringLiteral("default_font_size"), defaultFontSize);
    settings->setValue(QStringLiteral("default_fixed_font_size"), defaultFixedFontSize);
    settings->setValue(QStringLiteral("minimum_font_size"), minimumFontSize);

    settings->setValue(QStringLiteral("appearance"), QVariant::fromValue(contentAppearance));
    settings->setValue(QStringLiteral("highlight_on_navigate"), isHighlightOnNavigateEnabled);
    settings->setValue(QStringLiteral("custom_css_file"), customCssFile);
    settings->setValue(QStringLiteral("external_link_policy"), QVariant::fromValue(externalLinkPolicy));
    settings->setValue(QStringLiteral("smooth_scrolling"), isSmoothScrollingEnabled);
    settings->endGroup();

    settings->beginGroup(GroupProxy);
    settings->setValue(QStringLiteral("type"), QVariant::fromValue(proxyType));
    settings->setValue(QStringLiteral("host"), proxyHost);
    settings->setValue(QStringLiteral("port"), proxyPort);
    settings->setValue(QStringLiteral("authenticate"), proxyAuthenticate);
    settings->setValue(QStringLiteral("username"), proxyUserName);
    settings->setValue(QStringLiteral("password"), proxyPassword);
    settings->setValue(QStringLiteral("ignore_ssl_errors"), isIgnoreSslErrorsEnabled);
    settings->endGroup();

    settings->beginGroup(GroupDocsets);
    settings->setValue(QStringLiteral("path"), docsetPath);
    settings->setValue(QStringLiteral("read_only"), docsetPathReadOnly);
    settings->endGroup();

    settings->beginGroup(GroupInternal);
    settings->setValue(QStringLiteral("install_id"), installId);
    // Version of configuration file format, should match Zeal version. Used for migration rules.
    settings->setValue(QStringLiteral("version"), Application::version().toString());
    settings->endGroup();

    settings->sync();

    emit updated();
}

void Settings::resetGettingStartedChecklist()
{
    gettingStartedChecklist = 0;
    gettingStartedChecklistDismissed = false;
    save();
}

void Settings::resetHelpTips()
{
    dismissedHelpTips.clear();
    save();
}

/*!
 * \internal
 * \brief Migrates settings from older application versions.
 * \param settings QSettings object to update.
 *
 * The settings migration process relies on 'internal/version' option, that was introduced in the
 * release 0.2.0, so a missing option indicates pre-0.2 release.
 */
void Settings::migrate(QSettings *settings)
{
    settings->beginGroup(GroupInternal);
    const auto version = QVersionNumber::fromString(settings->value(QStringLiteral("version")).toString());
    settings->endGroup();

    //
    // 0.6.0
    //

    // Unset content.default_fixed_font_size.
    // The causing bug was 0.6.1 (#903), but the incorrect setting still comes to haunt us (#1054).
    if (version == QVersionNumber(0, 6, 0)) {
        settings->beginGroup(GroupContent);
        settings->remove(QStringLiteral("default_fixed_font_size"));
        settings->endGroup();
    }

    //
    // Pre 0.4
    //

    // Rename 'browser' group into 'content'.
    if (version < QVersionNumber(0, 4, 0)) {
        settings->beginGroup(QStringLiteral("browser"));
        const QVariant tmpMinimumFontSize = settings->value(QStringLiteral("minimum_font_size"));
        settings->endGroup();
        settings->remove(QStringLiteral("browser"));

        if (tmpMinimumFontSize.isValid()) {
            settings->beginGroup(GroupContent);
            settings->setValue(QStringLiteral("minimum_font_size"), tmpMinimumFontSize);
            settings->endGroup();
        }
    }
}

/*!
 * \internal
 * \brief Returns an initialized QSettings object.
 * \param parent Optional parent object.
 * \return QSettings object.
 *
 * QSettings is initialized according to build options, e.g. standard vs portable.
 * Caller is responsible for deleting the returned object.
 */
std::unique_ptr<QSettings> Settings::qsettings()
{
#ifndef PORTABLE_BUILD
    return std::make_unique<QSettings>();
#else
    return std::make_unique<QSettings>(QCoreApplication::applicationDirPath() + QLatin1String("/zeal.ini"),
                                       QSettings::IniFormat);
#endif
}

} // namespace Zeal::Core

QDataStream &operator<<(QDataStream &out, Zeal::Core::Settings::ContentAppearance policy)
{
    out << static_cast<std::underlying_type_t<Zeal::Core::Settings::ContentAppearance>>(policy);
    return out;
}

QDataStream &operator>>(QDataStream &in, Zeal::Core::Settings::ContentAppearance &policy)
{
    std::underlying_type_t<Zeal::Core::Settings::ContentAppearance> value = 0;
    in >> value;
    using Appearance = Zeal::Core::Settings::ContentAppearance;
    switch (static_cast<Appearance>(value)) {
    case Appearance::Automatic:
    case Appearance::Light:
    case Appearance::Dark:
        policy = static_cast<Appearance>(value);
        break;
    default:
        policy = Appearance::Automatic;
        break;
    }
    return in;
}

QDataStream &operator<<(QDataStream &out, Zeal::Core::Settings::ExternalLinkPolicy policy)
{
    out << static_cast<std::underlying_type_t<Zeal::Core::Settings::ExternalLinkPolicy>>(policy);
    return out;
}

QDataStream &operator>>(QDataStream &in, Zeal::Core::Settings::ExternalLinkPolicy &policy)
{
    std::underlying_type_t<Zeal::Core::Settings::ExternalLinkPolicy> value = 0;
    in >> value;
    policy = static_cast<Zeal::Core::Settings::ExternalLinkPolicy>(value);
    return in;
}
