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

    void queue(QPixmap *frame);
    void reset();

signals:
    void requestProcess();
    void processedPng(QByteArray pngData);

private slots:
    void process();

private:
    QMutex           mMutex;
    QList<QPixmap*>  mFrames;
};

#endif // WORKERTHREAD_H
