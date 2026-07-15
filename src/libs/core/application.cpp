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
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QStandardPaths>
#include <QSysInfo>
#include <QThread>

#include <memory>

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
    m_updateNetworkManager = new QNetworkAccessManager(this);

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

QString Application::repositorySlug()
{
    return QStringLiteral("zealrn/zealrn-desktop");
}

QUrl Application::releasesApiUrl(bool includePrereleases)
{
    const QString path = includePrereleases ? QStringLiteral("/releases?per_page=20")
                                            : QStringLiteral("/releases/latest");
    return QUrl(QStringLiteral("https://api.github.com/repos/%1%2").arg(repositorySlug(), path));
}

namespace {
struct SemanticVersion {
    QList<quint64> core;
    QStringList prerelease;
};

std::optional<SemanticVersion> parseSemanticVersion(QString text)
{
    if (text.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        text.remove(0, 1);
    }
    static const QRegularExpression pattern(
        QStringLiteral(R"(^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?(?:\+[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*)?$)"));
    const QRegularExpressionMatch match = pattern.match(text);
    if (!match.hasMatch()) {
        return std::nullopt;
    }

    SemanticVersion result;
    for (int index = 1; index <= 3; ++index) {
        bool ok = false;
        const quint64 value = match.captured(index).toULongLong(&ok);
        if (!ok) {
            return std::nullopt;
        }
        result.core.append(value);
    }
    if (!match.captured(4).isEmpty()) {
        result.prerelease = match.captured(4).split(QLatin1Char('.'));
    }
    return result;
}

bool isTrustedReleaseUrl(const QUrl &url)
{
    return url.scheme() == QLatin1String("https") && url.host() == QLatin1String("github.com")
           && url.path().startsWith(QStringLiteral("/%1/releases/").arg(Application::repositorySlug()));
}
} // namespace

int Application::compareSemanticVersions(const QString &candidate, const QString &installed)
{
    const auto left = parseSemanticVersion(candidate);
    const auto right = parseSemanticVersion(installed);
    if (!left || !right) {
        return 0;
    }
    for (int index = 0; index < 3; ++index) {
        if (left->core.at(index) != right->core.at(index)) {
            return left->core.at(index) > right->core.at(index) ? 1 : -1;
        }
    }
    if (left->prerelease.isEmpty() || right->prerelease.isEmpty()) {
        if (left->prerelease.isEmpty() == right->prerelease.isEmpty()) {
            return 0;
        }
        return left->prerelease.isEmpty() ? 1 : -1;
    }
    const int count = qMin(left->prerelease.size(), right->prerelease.size());
    for (int index = 0; index < count; ++index) {
        const QString &leftPart = left->prerelease.at(index);
        const QString &rightPart = right->prerelease.at(index);
        bool leftNumeric = false;
        bool rightNumeric = false;
        const quint64 leftNumber = leftPart.toULongLong(&leftNumeric);
        const quint64 rightNumber = rightPart.toULongLong(&rightNumeric);
        if (leftNumeric && rightNumeric && leftNumber != rightNumber) {
            return leftNumber > rightNumber ? 1 : -1;
        }
        if (leftNumeric != rightNumeric) {
            return leftNumeric ? -1 : 1;
        }
        const int comparison = QString::compare(leftPart, rightPart, Qt::CaseSensitive);
        if (comparison != 0) {
            return comparison > 0 ? 1 : -1;
        }
    }
    return left->prerelease.size() == right->prerelease.size()
               ? 0
               : (left->prerelease.size() > right->prerelease.size() ? 1 : -1);
}

std::optional<Application::ReleaseInfo> Application::publishedRelease(const QByteArray &json,
                                                                      bool includePrereleases,
                                                                      QString *error)
{
    if (error != nullptr) {
        error->clear();
    }
    QJsonParseError jsonError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || (!document.isObject() && !document.isArray())) {
        if (error != nullptr) {
            *error = jsonError.error != QJsonParseError::NoError ? jsonError.errorString()
                                                                  : tr("Server returned invalid release data.");
        }
        return std::nullopt;
    }

    const QJsonArray releases = document.isObject() ? QJsonArray{document.object()} : document.array();
    std::optional<ReleaseInfo> latest;
    for (const QJsonValue &value : releases) {
        if (!value.isObject()) {
            if (error != nullptr) {
                *error = tr("Server returned invalid release data.");
            }
            return std::nullopt;
        }
        const QJsonObject object = value.toObject();
        if (!object.value(QStringLiteral("draft")).isBool()
            || !object.value(QStringLiteral("prerelease")).isBool()) {
            if (error != nullptr) {
                *error = tr("Server returned invalid release data.");
            }
            return std::nullopt;
        }
        const bool prerelease = object.value(QStringLiteral("prerelease")).toBool();
        if (object.value(QStringLiteral("draft")).toBool() || (prerelease && !includePrereleases)) {
            continue;
        }

        QString version = object.value(QStringLiteral("tag_name")).toString();
        if (version.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
            version.remove(0, 1);
        }
        const QUrl pageUrl(object.value(QStringLiteral("html_url")).toString());
        const QDateTime publishedAt = QDateTime::fromString(object.value(QStringLiteral("published_at")).toString(),
                                                            Qt::ISODate);
        if (!parseSemanticVersion(version) || !publishedAt.isValid() || !isTrustedReleaseUrl(pageUrl)) {
            if (error != nullptr) {
                *error = tr("Server returned invalid release data.");
            }
            return std::nullopt;
        }

        ReleaseInfo info{version,
                         object.value(QStringLiteral("name")).toString().trimmed(),
                         publishedAt,
                         pageUrl,
                         prerelease};
        if (info.title.isEmpty()) {
            info.title = QStringLiteral("ZealRN %1").arg(info.version);
        }
        if (!latest || compareSemanticVersions(info.version, latest->version) > 0) {
            latest = info;
        }
    }
    return latest;
}

QUrl Application::releasesPageUrl()
{
    return QUrl(QStringLiteral("https://github.com/abnzrdev/zealrn/releases"));
}

std::optional<QVersionNumber> Application::latestPublishedRelease(const QByteArray &json, QString *error)
{
    const auto release = publishedRelease(json, false, error);
    return release ? std::optional<QVersionNumber>(QVersionNumber::fromString(release->version)) : std::nullopt;
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
bool Application::checkForUpdates(bool quiet)
{
    if (m_updateReply != nullptr) {
        return false;
    }

    constexpr qsizetype MaxResponseSize = 1024 * 1024;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    m_settings->updateLastAttempt = now;
    m_settings->save();

    QNetworkRequest request(releasesApiUrl(m_settings->updateIncludePrereleases));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(15'000);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    if (!m_settings->updateEtag.isEmpty()) {
        request.setRawHeader("If-None-Match", m_settings->updateEtag);
    }

    m_updateReply = m_updateNetworkManager->get(request);
    QNetworkReply *reply = m_updateReply;
    auto body = std::make_shared<QByteArray>();
    auto oversized = std::make_shared<bool>(false);
    connect(reply, &QIODevice::readyRead, this, [reply, body, oversized]() {
        body->append(reply->readAll());
        if (body->size() > MaxResponseSize) {
            *oversized = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, body, oversized, quiet, now]() {
        QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> cleanup(reply);
        m_updateReply = nullptr;
        body->append(reply->readAll());

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        auto reportError = [this, quiet](const QString &message) {
            if (!quiet) {
                emit updateCheckError(message);
            }
        };
        if (*oversized || body->size() > MaxResponseSize) {
            reportError(tr("The release response was too large."));
            return;
        }

        std::optional<ReleaseInfo> release;
        if (status == 304) {
            const ReleaseInfo cached{m_settings->updateCachedVersion,
                                     m_settings->updateCachedTitle,
                                     m_settings->updateCachedPublishedAt,
                                     QUrl(m_settings->updateCachedPageUrl),
                                     m_settings->updateCachedPrerelease};
            if (!cached.version.isEmpty() && cached.publishedAt.isValid() && isTrustedReleaseUrl(cached.pageUrl)) {
                release = cached;
            }
        } else if (status == 404) {
            m_settings->updateLastSuccess = now;
            m_settings->save();
            if (!quiet) {
                emit updateCheckNoReleases();
            }
            return;
        } else if (status == 403) {
            const QByteArray remaining = reply->rawHeader("X-RateLimit-Remaining");
            reportError(remaining == "0" ? tr("GitHub's API rate limit has been reached. Try again later.")
                                          : tr("GitHub refused the update request (HTTP 403)."));
            return;
        } else if (reply->error() != QNetworkReply::NoError) {
            reportError(reply->errorString());
            return;
        } else if (status < 200 || status >= 300) {
            reportError(tr("GitHub returned HTTP status %1.").arg(status));
            return;
        } else if (body->isEmpty()) {
            reportError(tr("GitHub returned an empty release response."));
            return;
        } else {
            QString error;
            release = publishedRelease(*body, m_settings->updateIncludePrereleases, &error);
            if (!error.isEmpty()) {
                reportError(error);
                return;
            }
            m_settings->updateEtag = reply->rawHeader("ETag");
            if (release) {
                m_settings->updateCachedVersion = release->version;
                m_settings->updateCachedTitle = release->title;
                m_settings->updateCachedPublishedAt = release->publishedAt;
                m_settings->updateCachedPageUrl = release->pageUrl.toString();
                m_settings->updateCachedPrerelease = release->prerelease;
            }
        }

        m_settings->updateLastSuccess = now;
        m_settings->save();
        if (!release) {
            if (!quiet) {
                emit updateCheckNoReleases();
            }
            return;
        }
        if (compareSemanticVersions(release->version, QCoreApplication::applicationVersion()) > 0
            && release->version != m_settings->updateSkippedVersion) {
            emit updateAvailable(*release);
            emit updateCheckDone(release->version);
        } else if (!quiet) {
            emit updateCheckDone();
        }
    });
    return true;
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
