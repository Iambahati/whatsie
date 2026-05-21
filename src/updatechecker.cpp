#include "updatechecker.h"

#include "def.h"
#include "settingsmanager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>
#include <QVersionNumber>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)),
      m_dailyTimer(new QTimer(this)), m_state(Idle),
      m_downloadReply(nullptr) {

  // Schedule daily check at midnight
  connect(m_dailyTimer, &QTimer::timeout, this, &UpdateChecker::checkForUpdates);

  QTime midnight(0, 0);
  QDateTime now = QDateTime::currentDateTime();
  QDateTime nextMidnight = QDateTime(now.date().addDays(1), midnight);
  m_dailyTimer->start(now.msecsTo(nextMidnight));

  // Check immediately if last check was >24 hours ago
  if (shouldCheckNow()) {
    checkForUpdates();
  }
}

UpdateChecker::~UpdateChecker() {
  if (m_downloadReply) {
    m_downloadReply->abort();
    m_downloadReply->deleteLater();
  }
}

bool UpdateChecker::shouldCheckNow() const {
  QSettings &settings = SettingsManager::instance().settings();
  QString lastCheckStr = settings.value("lastUpdateCheck").toString();

  if (lastCheckStr.isEmpty()) {
    return true;
  }

  QDateTime lastCheck = QDateTime::fromString(lastCheckStr, Qt::ISODate);
  QDateTime now = QDateTime::currentDateTime();

  return lastCheck.secsTo(now) >= 24 * 60 * 60; // 24 hours
}

void UpdateChecker::checkForUpdates() {
  if (m_state == Downloading) {
    return; // Don't interrupt download
  }

  setState(Checking);

  QUrl url("https://api.github.com/repos/Iambahati/whatsie/releases/latest");
  QNetworkRequest request(url);
  request.setRawHeader("Accept", "application/vnd.github.v3+json");
  request.setRawHeader("User-Agent",
                       QCoreApplication::applicationName().toUtf8());

  QNetworkReply *reply = m_networkManager->get(request);
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onCheckFinished(reply); });
}

void UpdateChecker::onCheckFinished(QNetworkReply *reply) {
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    setState(Error);
    emit checkError(tr("Failed to check for updates: %1").arg(reply->errorString()));
    return;
  }

  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);

  if (!doc.isObject()) {
    setState(Error);
    emit checkError(tr("Invalid response from GitHub API"));
    return;
  }

  QJsonObject release = doc.object();
  QString tag = release["tag_name"].toString();

  // Store last check time
  SettingsManager::instance().settings().setValue(
      "lastUpdateCheck", QDateTime::currentDateTime().toString(Qt::ISODate));

  if (isNewerVersion(tag)) {
    m_latestVersion = tag;
    QJsonArray assets = release["assets"].toArray();
    m_downloadUrl = getAssetUrl(assets);

    if (m_downloadUrl.isEmpty()) {
      setState(Error);
      emit checkError(tr("No suitable asset found for this platform"));
      return;
    }

    setState(UpdateAvailable);
    emit updateAvailable(m_latestVersion, m_downloadUrl);
  } else {
    setState(UpToDate);
    emit noUpdateAvailable();
  }
}

bool UpdateChecker::isNewerVersion(const QString &remote) const {
  // Strip 'v' prefix if present
  QString remoteClean = remote;
  if (remoteClean.startsWith("v", Qt::CaseInsensitive)) {
    remoteClean = remoteClean.mid(1);
  }

  QString current = VERSIONSTR;
  if (current.startsWith("v", Qt::CaseInsensitive)) {
    current = current.mid(1);
  }

  QVersionNumber remoteVer = QVersionNumber::fromString(remoteClean);
  QVersionNumber currentVer = QVersionNumber::fromString(current);

  return remoteVer > currentVer;
}

QString UpdateChecker::getAssetUrl(const QJsonArray &assets) const {
  for (const QJsonValue &val : assets) {
    QJsonObject asset = val.toObject();
    QString name = asset["name"].toString().toLower();

    // Look for Linux x86_64 assets (tar.gz or AppImage)
    if (name.contains("linux") && name.contains("x86_64")) {
      return asset["browser_download_url"].toString();
    }
  }
  return QString();
}

void UpdateChecker::downloadUpdate(const QString &url) {
  if (m_state != UpdateAvailable) {
    return;
  }

  setState(Downloading);

  QNetworkRequest request{QUrl(url)};
  request.setRawHeader("User-Agent",
                       QCoreApplication::applicationName().toUtf8());

  m_downloadReply = m_networkManager->get(request);
  connect(m_downloadReply, &QNetworkReply::downloadProgress, this,
          &UpdateChecker::onDownloadProgress);
  connect(m_downloadReply, &QNetworkReply::finished, this,
          [this]() { onDownloadFinished(m_downloadReply); });
}

void UpdateChecker::onDownloadProgress(qint64 received, qint64 total) {
  emit downloadProgress(received, total);
}

void UpdateChecker::onDownloadFinished(QNetworkReply *reply) {
  reply->deleteLater();
  m_downloadReply = nullptr;

  if (reply->error() != QNetworkReply::NoError) {
    setState(Error);
    emit checkError(tr("Download failed: %1").arg(reply->errorString()));
    return;
  }

  QByteArray data = reply->readAll();

  QString updateDir = getUpdateDir();
  QDir dir(updateDir);
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  QString filename = QUrl(reply->url()).fileName();
  if (filename.isEmpty()) {
    filename = "whatsie-update.tar.gz";
  }

  m_downloadedFilePath = dir.filePath(filename);

  QFile file(m_downloadedFilePath);
  if (!file.open(QIODevice::WriteOnly)) {
    setState(Error);
    emit checkError(tr("Failed to write update file: %1").arg(file.errorString()));
    return;
  }

  file.write(data);
  file.close();

  // Make it executable
  file.setPermissions(file.permissions() | QFileDevice::ExeUser);

  // Store path for restart
  SettingsManager::instance().settings().setValue("pendingUpdatePath",
                                                  m_downloadedFilePath);

  setState(ReadyToRestart);
  emit downloadFinished(m_downloadedFilePath);
}

QString UpdateChecker::getUpdateDir() const {
  return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
         "/update";
}

void UpdateChecker::setState(State newState) {
  if (m_state != newState) {
    m_state = newState;
    emit stateChanged(m_state);
  }
}
