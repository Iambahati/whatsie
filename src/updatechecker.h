#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>

class UpdateChecker : public QObject {
  Q_OBJECT
public:
  explicit UpdateChecker(QObject *parent = nullptr);
  ~UpdateChecker();

  enum State {
    Idle,
    Checking,
    UpToDate,
    UpdateAvailable,
    Downloading,
    ReadyToRestart,
    Error
  };

  State state() const { return m_state; }
  QString latestVersion() const { return m_latestVersion; }
  QString downloadUrl() const { return m_downloadUrl; }
  QString downloadedFilePath() const { return m_downloadedFilePath; }

public slots:
  void checkForUpdates();
  void downloadUpdate(const QString &url);

signals:
  void stateChanged(State state);
  void updateAvailable(QString version, QString downloadUrl);
  void noUpdateAvailable();
  void downloadProgress(qint64 received, qint64 total);
  void downloadFinished(QString localPath);
  void checkError(QString message);

private slots:
  void onCheckFinished(QNetworkReply *reply);
  void onDownloadProgress(qint64 received, qint64 total);
  void onDownloadFinished(QNetworkReply *reply);

private:
  bool shouldCheckNow() const;
  bool isNewerVersion(const QString &remote) const;
  QString getAssetUrl(const QJsonArray &assets) const;
  QString getUpdateDir() const;
  void setState(State newState);

  QNetworkAccessManager *m_networkManager;
  QTimer *m_dailyTimer;
  State m_state;
  QString m_latestVersion;
  QString m_downloadUrl;
  QString m_downloadedFilePath;
  QNetworkReply *m_downloadReply;
};

#endif // UPDATECHECKER_H
