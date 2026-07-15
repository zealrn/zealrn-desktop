// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_APPLICATION_H
#define ZEAL_CORE_APPLICATION_H

#include <QObject>
#include <QDateTime>
#include <QUrl>
#include <QVersionNumber>

#include <optional>

class QNetworkAccessManager;
class QNetworkReply;
class QThread;

namespace Zeal {

namespace Registry {
class DocsetRegistry;
} // namespace Registry

namespace Core {

class Extractor;
class FileManager;
class HttpServer;
class Settings;
struct Session;

class Application final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Application)
public:
    struct ReleaseInfo {
        QString version;
        QString title;
        QDateTime publishedAt;
        QUrl pageUrl;
        bool prerelease = false;
    };

    explicit Application(quint16 httpServerPort = 0, QObject *parent = nullptr);
    ~Application() override;

    static Application *instance();

    QNetworkAccessManager *networkManager() const;
    Session *session() const;
    Settings *settings() const;

    Registry::DocsetRegistry *docsetRegistry() const;
    FileManager *fileManager() const;
    HttpServer *httpServer() const;
    Extractor *extractor() const;

    static QString cacheLocation();
    static QString configLocation();
    static QVersionNumber version();
    static QString versionString();
    static QString repositorySlug();
    static QUrl releasesApiUrl(bool includePrereleases = false);
    static QUrl releasesPageUrl();
    static std::optional<ReleaseInfo> publishedRelease(const QByteArray &json,
                                                       bool includePrereleases,
                                                       QString *error = nullptr);
    static int compareSemanticVersions(const QString &candidate, const QString &installed);
    static std::optional<QVersionNumber> latestPublishedRelease(const QByteArray &json,
                                                                QString *error = nullptr);

    QNetworkReply *download(const QUrl &url);
    bool checkForUpdates(bool quiet = false);

signals:
    void updateCheckDone(const QString &version = QString());
    void updateCheckNoReleases();
    void updateCheckError(const QString &message);
    void updateAvailable(const Zeal::Core::Application::ReleaseInfo &release);

private:
    void applySettings();
    static inline QString userAgent();
    QString userAgentJson() const;

    static Application *m_instance;

    Session *m_session = nullptr;
    Settings *m_settings = nullptr;

    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkAccessManager *m_updateNetworkManager = nullptr;
    QNetworkReply *m_updateReply = nullptr;

    FileManager *m_fileManager = nullptr;
    HttpServer *m_httpServer = nullptr;

    QThread *m_extractorThread = nullptr;
    Extractor *m_extractor = nullptr;

    Registry::DocsetRegistry *m_docsetRegistry = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_APPLICATION_H
