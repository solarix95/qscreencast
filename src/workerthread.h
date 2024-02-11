#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QThread>
#include <QByteArray>
#include <QPixmap>
#include <QList>
#include <QMutex>

class WorkerThread : public QThread
{
    Q_OBJECT
public:
    WorkerThread();
    virtual ~WorkerThread();


    void setImageFormat(const QString &formatName);
    void queue(QPixmap *frame);
    void reset();
    void shutdown();
    void setFilterFreezeFrames(bool filter);
    int  processedFrames() const;

signals:
    void requestProcess();
    void requestShutdown();
    void processedFrame(QByteArray pngData);
    void frameProcessed();

private slots:
    void process();

private:
    static bool isIdenticalFrame(const QPixmap &p1, const QPixmap &p2);

    bool             mFilterFreezeFrames;
    mutable QMutex   mMutex;
    QList<QPixmap*>  mFrames;
    QPixmap*         mLastFrame;
    int              mProcessedFrames;
    QString          mImageFormat;
};

#endif // WORKERTHREAD_H
