#include "workerthread.h"
#include <QCoreApplication>
#include <QBuffer>
#include <QImage>

//-------------------------------------------------------------------------------------------------
WorkerThread::WorkerThread()
 : mFilterFreezeFrames(false)
 , mLastFrame(nullptr)
 , mProcessedFrames(0)
{
    moveToThread(this);
    connect(this, &WorkerThread::requestProcess, this, &WorkerThread::process, Qt::QueuedConnection);
    connect(this, &WorkerThread::requestShutdown, this, &WorkerThread::quit, Qt::QueuedConnection);
    mImageFormat = "PNG";
}

//-------------------------------------------------------------------------------------------------
WorkerThread::~WorkerThread()
{
    qDeleteAll(mFrames);
    mFrames.clear();
}

//-------------------------------------------------------------------------------------------------
void WorkerThread::setImageFormat(const QString &formatName)
{
    mMutex.lock();
    mImageFormat = formatName;
    Q_ASSERT(!mImageFormat.isEmpty());
    mMutex.unlock();
}

//-------------------------------------------------------------------------------------------------
void WorkerThread::queue(QPixmap *frame)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    mMutex.lock();
    mFrames << frame;
    mMutex.unlock();

    emit requestProcess();
}

//-------------------------------------------------------------------------------------------------
void WorkerThread::reset()
{
    mMutex.lock();
    qDeleteAll(mFrames);
    mFrames.clear();
    if (mLastFrame) {
        delete mLastFrame;
        mLastFrame = nullptr;
    }
    mProcessedFrames = 0;
    mMutex.unlock();
}

//-------------------------------------------------------------------------------------------------
void WorkerThread::shutdown()
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    emit requestShutdown();
}

//-------------------------------------------------------------------------------------------------
void WorkerThread::setFilterFreezeFrames(bool filter)
{
    mFilterFreezeFrames = filter;
}

//-------------------------------------------------------------------------------------------------
int WorkerThread::processedFrames() const
{
    mMutex.lock();
    int ret = mProcessedFrames;
    mMutex.unlock();
    return ret;
}

//-------------------------------------------------------------------------------------------------
void WorkerThread::process()
{
    Q_ASSERT(QThread::currentThread() == this);

    QPixmap *p = nullptr;

    mMutex.lock();
    p = mFrames.isEmpty() ? nullptr : mFrames.takeFirst();

    bool frameIncrease = !!p;

    if (p && mLastFrame && mFilterFreezeFrames) {
        if (isIdenticalFrame(*p, *mLastFrame)) { // skip frame
            delete p;
            p = nullptr;
        }
    }

    if (frameIncrease)
        mProcessedFrames++;

    mMutex.unlock();

    if (!p) {
        if (frameIncrease)
            emit frameProcessed();
        return;
    }

    QByteArray pngData;
    QBuffer    buffer(&pngData);

    if (mImageFormat.startsWith("JPG-"))
        p->save(&buffer,"JPG",mImageFormat.split("-")[1].toInt());
    else
        p->save(&buffer,mImageFormat.toUtf8().constData());

    emit processedFrame(pngData);
    if (mLastFrame)
        delete mLastFrame;
    mLastFrame = p;

    if (frameIncrease)
        emit frameProcessed();
}

//-------------------------------------------------------------------------------------------------
bool WorkerThread::isIdenticalFrame(const QPixmap &p1, const QPixmap &p2)
{
    if (p1.width() != p2.width())
        return false;
    if (p1.height() != p2.height())
        return false;


    int previewWidth = p1.width()  > 10 ? 10 : p1.width();
    int previewHeith = p1.height() > 10 ? 10 : p1.height();

    QImage preview1 = p1.scaled(previewWidth,previewHeith).toImage();
    QImage preview2 = p2.scaled(previewWidth,previewHeith).toImage();

    for (int y=0; y<preview1.height(); y++) {
        for (int x=0; x<preview1.width(); x++) {
            if (preview1.pixel(x,y) != preview2.pixel(x,y))
                return false;
        }
    }

    return true;
}

