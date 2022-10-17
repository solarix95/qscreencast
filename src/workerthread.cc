#include "workerthread.h"
#include <QCoreApplication>
#include <QBuffer>

//-------------------------------------------------------------------------------------------------
WorkerThread::WorkerThread()
{
    moveToThread(this);
    connect(this, &WorkerThread::requestProcess, this, &WorkerThread::process, Qt::QueuedConnection);
}

//-------------------------------------------------------------------------------------------------
WorkerThread::~WorkerThread()
{
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
    mMutex.unlock();
}

//-------------------------------------------------------------------------------------------------
void WorkerThread::process()
{
    Q_ASSERT(QThread::currentThread() == this);

    QPixmap *p = nullptr;

    mMutex.lock();
    p = mFrames.isEmpty() ? nullptr : mFrames.takeFirst();
    mMutex.unlock();

    if (!p)
        return;

    QByteArray pngData;
    QBuffer    buffer(&pngData);

    p->save(&buffer,"PNG");
    emit processedPng(pngData);
    delete p;
}

