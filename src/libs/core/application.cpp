// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application.h"

#include "extractor.h"
#include "filemanager.h"
#include "httpserver.h"
#include "networkaccessmanager.h"
#include "session.h"
#include "settings.h"

#include <registry/docsetregistry.h>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QStandardPaths>
#include <QSysInfo>
#include <QThread>

namespace Zeal::Core {

Application *Application::m_instance = nullptr;

Application::Application(quint16 httpServerPort, QObject *parent)
    : QObject(parent)
{
    // Ensure only one instance of Application
    Q_ASSERT(!m_instance);
    m_instance = this;

    m_settings = new Settings(this);
    m_session = new Session();
    m_session->load();

    m_networkManager = new NetworkAccessManager(this);

    m_fileManager = new FileManager(this);
    m_httpServer = new HttpServer(httpServerPort, this);

    connect(m_networkManager,
            &QNetworkAccessManager::sslErrors,
            this,
            [this](QNetworkReply *reply, const QList<QSslError> &errors) {
        Q_UNUSED(errors);
        if (m_settings->isIgnoreSslErrorsEnabled) {
            reply->ignoreSslErrors();
        }
    });

    // Extractor setup
    m_extractorThread = new QThread(this);
    m_extractor = new Extractor();
    m_extractor->moveToThread(m_extractorThread);
    m_extractorThread->start();

    m_docsetRegistry = new Registry::DocsetRegistry(m_httpServer);

    connect(m_settings, &Settings::updated, this, &Application::applySettings);
    applySettings();
}

Application::~Application()
{
    m_extractorThread->quit();
    m_extractorThread->wait();
    delete m_extractor;
    delete m_docsetRegistry;

    m_session->save();
    delete m_session;

    m_instance = nullptr;
}

/*!
 * \internal
 * \brief Returns a pointer to the Core::Application instance.
 * \return A pointer or \c nullptr, if no instance has been created.
 */
Application *Application::instance()
{
    return m_instance;
}

QNetworkAccessManager *Application::networkManager() const
{
    return m_networkManager;
}

Session *Application::session() const
{
    return m_session;
}

Settings *Application::settings() const
{
    return m_settings;
}

Registry::DocsetRegistry *Application::docsetRegistry() const
{
    return m_docsetRegistry;
}

FileManager *Application::fileManager() const
{
    return m_fileManager;
}

HttpServer *Application::httpServer() const
{
    return m_httpServer;
}

Extractor *Application::extractor() const
{
    return m_extractor;
}

QString Application::cacheLocation()
{
#ifndef PORTABLE_BUILD
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    return QCoreApplication::applicationDirPath() + QLatin1String("/cache");
#endif
}

QString Application::configLocation()
{
#ifndef PORTABLE_BUILD
    // TODO: Replace 'Zeal/Zeal' with 'zeal'.
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
#else
    return QCoreApplication::applicationDirPath() + QLatin1String("/config");
#endif
}

QVersionNumber Application::version()
{
    static const auto vn = QVersionNumber::fromString(QCoreApplication::applicationVersion());
    return vn;
}

QString Application::versionString()
{
    static const auto v = QStringLiteral("v%1").arg(QCoreApplication::applicationVersion());
    return v;
}

QUrl Application::releasesApiUrl()
{
    return QUrl(QStringLiteral("https://api.github.com/repos/abnzrdev/zealrn/releases"));
}

QUrl Application::releasesPageUrl()
{
    return QUrl(QStringLiteral("https://github.com/abnzrdev/zealrn/releases"));
}

std::optional<QVersionNumber> Application::latestPublishedRelease(const QByteArray &json, QString *error)
{
    if (error != nullptr) {
        error->clear();
    }

    QJsonParseError jsonError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !document.isArray()) {
        if (error != nullptr) {
            *error = jsonError.error != QJsonParseError::NoError ? jsonError.errorString()
                                                                  : tr("Server returned an invalid release list.");
        }
        return std::nullopt;
    }

    QVersionNumber latest;
    for (const QJsonValue &value : document.array()) {
        if (!value.isObject()) {
            if (error != nullptr) {
                *error = tr("Server returned an invalid release list.");
            }
            return std::nullopt;
        }

        const QJsonObject release = value.toObject();
        if (release.value(QStringLiteral("draft")).toBool()
            || release.value(QStringLiteral("prerelease")).toBool()) {
            continue;
        }

        QString tag = release.value(QStringLiteral("tag_name")).toString();
        if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
            tag.remove(0, 1);
        }
        qsizetype suffixIndex = 0;
        const QVersionNumber version = QVersionNumber::fromString(tag, &suffixIndex);
        if (version.isNull() || suffixIndex != tag.size()) {
            if (error != nullptr) {
                *error = tr("Server returned an invalid release list.");
            }
            return std::nullopt;
        }
        if (latest.isNull() || version > latest) {
            latest = version;
        }
    }

    return latest.isNull() ? std::nullopt : std::optional<QVersionNumber>(latest);
}

QNetworkReply *Application::download(const QUrl &url)
{
    static const QString ua = userAgent();
    static const QByteArray uaJson = userAgentJson().toUtf8();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, ua);

    if (url.host().endsWith(QLatin1String(".zealdocs.org", Qt::CaseInsensitive))) {
        request.setRawHeader("X-Zeal-User-Agent", uaJson);
    }

    return m_networkManager->get(request);
}

/*!
  \internal

  Performs a check whether a new ZealRN version is available. Setting \a quiet to true suppresses
  error and "you are using the latest version" message boxes.
*/
void Application::checkForUpdates(bool quiet)
{
    const QNetworkReply *reply = download(releasesApiUrl());
    connect(reply, &QNetworkReply::finished, this, [this, quiet]() {
        const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(qobject_cast<QNetworkReply *>(sender()));

        if (reply->error() != QNetworkReply::NoError) {
            if (!quiet) {
                emit updateCheckError(reply->errorString());
            }
            return;
        }

        QString error;
        const auto latestVersion = latestPublishedRelease(reply->readAll(), &error);
        if (!error.isEmpty()) {
            if (!quiet) {
                emit updateCheckError(error);
            }
            return;
        }

        if (!latestVersion.has_value()) {
            if (!quiet) {
                emit updateCheckNoReleases();
            }
            return;
        }

        if (*latestVersion > version()) {
            emit updateCheckDone(latestVersion->toString());
        } else if (!quiet) {
            emit updateCheckDone();
        }
    });
}

void Application::applySettings()
{
    m_docsetRegistry->setFuzzySearchEnabled(m_settings->isFuzzySearchEnabled);
    m_docsetRegistry->setStoragePath(m_settings->docsetPath);

    // HTTP Proxy Settings
    switch (m_settings->proxyType) {
    case Settings::ProxyType::None:
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        break;

    case Settings::ProxyType::System:
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        break;

    case Settings::ProxyType::Http:
    case Settings::ProxyType::Socks5: {
        const QNetworkProxy::ProxyType type = m_settings->proxyType == Settings::ProxyType::Socks5
                                                ? QNetworkProxy::Socks5Proxy
                                                : QNetworkProxy::HttpProxy;

        QNetworkProxy proxy(type, m_settings->proxyHost, m_settings->proxyPort);
        if (m_settings->proxyAuthenticate) {
            proxy.setUser(m_settings->proxyUserName);
            proxy.setPassword(m_settings->proxyPassword);
        }

        QNetworkProxy::setApplicationProxy(proxy);

        break;
    }
    }

    // Force NM to pick up changes.
    m_networkManager->clearAccessCache();
}

QString Application::userAgent()
{
    return QStringLiteral("Zeal/%1").arg(QCoreApplication::applicationVersion());
}

QString Application::userAgentJson() const
{
    const QJsonObject app = {{QStringLiteral("version"), QCoreApplication::applicationVersion()},
                             {QStringLiteral("qt_version"), qVersion()},
                             {QStringLiteral("install_id"), m_settings->installId}};

    const QJsonObject os = {{QStringLiteral("arch"), QSysInfo::currentCpuArchitecture()},
                            {QStringLiteral("name"), QSysInfo::prettyProductName()},
                            {QStringLiteral("product_type"), QSysInfo::productType()},
                            {QStringLiteral("product_version"), QSysInfo::productVersion()},
                            {QStringLiteral("kernel_type"), QSysInfo::kernelType()},
                            {QStringLiteral("kernel_version"), QSysInfo::kernelVersion()},
                            {QStringLiteral("locale"), QLocale::system().name()}};

    const QJsonObject ua = {{QStringLiteral("app"), app}, {QStringLiteral("os"), os}};

    return QString::fromUtf8(QJsonDocument(ua).toJson(QJsonDocument::Compact));
}

} // namespace Zeal::Core
