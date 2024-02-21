#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <QObject>
#include <QThread>
#include <QList>
#include <QMutex>
#include <QByteArray>
#include <QString>

class ImageCache : public QThread
{
    Q_OBJECT
public:
    ImageCache();

    void setup(const QString &fileExt, const QString &outputFolder);

    void startStream();
    void stop();
    void reset();
    void shutdown();
    int  cacheSize() const;

    void enqueue(const QByteArray &imageData);
    int  count() const;
    int  dumpToDisk(const QString &tocFilename = "", int tocFPS = -1, int width = -1, int height = -1);

signals:
    void queueChanged(int queueSize);
    void dataAppendedInternal();
    void requestShutdownInternal();

private slots:
    void processStream();

private:
    void createCinelerraToc(const QStringList &fileNames, const QString &tocFilename, int tocFPS, int width, int height) const;

    mutable QMutex    mMutex;
    QString           mFileExt;
    QString           mFolder;
    QList<QByteArray> mCache;

    // Disk Streaming
    bool              mStreamIsEnabled;
    QStringList       mStreamFiles;
};

#endif // IMAGECACHE_H
