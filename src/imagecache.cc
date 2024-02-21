#include <QDir>
#include <QFile>
#include <QThread>
#include <QCoreApplication>
#include <QMutexLocker>
#include "imagecache.h"


//-------------------------------------------------------------------------------------------------
ImageCache::ImageCache()
 : mStreamIsEnabled(false)
{
    connect(this, &ImageCache::requestShutdownInternal, this, &ImageCache::quit, Qt::QueuedConnection);
    connect(this, &ImageCache::dataAppendedInternal, this,    &ImageCache::processStream, Qt::QueuedConnection);
    moveToThread(this);
}

//-------------------------------------------------------------------------------------------------
void ImageCache::setup(const QString &fileExt, const QString &outputFolder)
{
    QMutexLocker locker(&mMutex);
    mFileExt = fileExt;
    mFolder  = outputFolder;
}

//-------------------------------------------------------------------------------------------------
void ImageCache::startStream()
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    QMutexLocker locker(&mMutex);

    Q_ASSERT(!mFolder.isEmpty());

    QDir d;
    d.mkpath(mFolder);
    mStreamFiles.clear();
    mStreamIsEnabled = true;
}

//-------------------------------------------------------------------------------------------------
void ImageCache::reset()
{
    QMutexLocker locker(&mMutex);
    mStreamIsEnabled = false;
    mCache.clear();
}

//-------------------------------------------------------------------------------------------------
void ImageCache::shutdown()
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    emit requestShutdownInternal();
}

//-------------------------------------------------------------------------------------------------
int ImageCache::cacheSize() const
{
    QMutexLocker locker(&mMutex);
    int ret = 0;
    for(int i=0; i<mCache.count(); i++)
        ret += mCache.at(i).size();
    return ret;
}

//-------------------------------------------------------------------------------------------------
void ImageCache::enqueue(const QByteArray &imageData)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    mMutex.lock();
    mCache << imageData;
    int ret = mCache.count();
    mMutex.unlock();
    emit queueChanged(ret);
    emit dataAppendedInternal();
}

//-------------------------------------------------------------------------------------------------
int ImageCache::count() const
{
    QMutexLocker locker(&mMutex);
    return mCache.count();
}

//-------------------------------------------------------------------------------------------------
int ImageCache::dumpToDisk(const QString &tocFilename, int tocFPS, int width, int height)
{
    QMutexLocker locker(&mMutex);

    QStringList createdFiles;

    QDir d;
    d.mkpath(mFolder);

    for(int i=0; i<mCache.count(); i++) {
        QFile f(QString("%1/%2.%3").arg(mFolder).arg(i+1,6,10,QChar('0')).arg(mFileExt));
        if (!f.open(QIODevice::WriteOnly))
            return createdFiles.count();
        f.write(mCache[i]);
        createdFiles << f.fileName();
    }

    if (!tocFilename.isEmpty())
        createCinelerraToc(createdFiles,tocFilename,tocFPS,width,height);

    return createdFiles.count();
}

//-------------------------------------------------------------------------------------------------
void ImageCache::processStream()
{
    if (!mStreamIsEnabled)
        return;

    mMutex.lock();
    Q_ASSERT(QThread::currentThread() == this);
    auto cache = mCache;
    mCache.clear();
    mMutex.unlock();
    if (cache.isEmpty())
        return;

    emit queueChanged(0);

    for(int i=0; i<cache.count(); i++) {
        QFile f(QString("%1/%2.%3").arg(mFolder).arg(mStreamFiles.count()+1,6,10,QChar('0')).arg(mFileExt));
        if (!f.open(QIODevice::WriteOnly))
            return;
        f.write(cache[i]);
        mStreamFiles << f.fileName();
    }
}

//-------------------------------------------------------------------------------------------------
void ImageCache::createCinelerraToc(const QStringList &fileNames, const QString &tocFilename, int tocFPS, int width, int height) const
{
    Q_ASSERT(!fileNames.isEmpty());
    Q_ASSERT(!tocFilename.isEmpty());

    QFile f(QString("%1/%2.toc").arg(mFolder).arg(tocFilename));
    if (!f.open(QIODevice::WriteOnly))
        return;

    QString contentType = mFileExt.toUpper();
    Q_ASSERT(!contentType.isEmpty());

    f.write(QString("%1LIST\n").arg(contentType).toUtf8()); // usually "PNGLIST"... supported by Cinelerra.. I support here also JPG etc..
    // f.write("PNGLIST\n");
    f.write("#CREATED BY QuickScreenCast\n");
    f.write(QString("%1 #FPS\n").arg(tocFPS).toUtf8());
    f.write(QString("%1 #Width\n").arg(width).toUtf8());
    f.write(QString("%1 #Height\n").arg(height).toUtf8());
    f.write("#FILES\n");
    for (auto name: fileNames)
        f.write(QString("%1\n").arg(name).toUtf8());
    f.close();
}
