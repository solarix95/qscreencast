#ifndef MAINFORM_H
#define MAINFORM_H

#include <QWidget>
#include <QList>
#include <QTimer>
#include <QStringList>

#include "workerthread.h"
#include "imagecache.h"
#include "previewwidget.h"

namespace Ui {
class MainForm;
}

class MainForm : public QWidget
{
    Q_OBJECT

public:
    explicit MainForm(QWidget *parent = 0);
    ~MainForm();

private slots:
    void screenShot(bool preview);
    void appendFrame(QByteArray pngData);
    void exportRecorder();
    void resetRecorder();

protected:
    virtual void closeEvent(QCloseEvent *event);

private slots:
    void initRecorder();
    void prepareExport();
    void updateUi();

private:
    QString castName() const;
    QString prepareCast();

    Ui::MainForm      *ui;
    PreviewWidget     *mPreview;
    QScreen           *mScreen;
    QTimer             mRecorderTimer;
    WorkerThread       mThread;
    ImageCache         mCache;       // In-Memory-Cache and also Dump-to-Disk-Thread

    int                mRecordWidth;
    int                mRecordHeight;
    int                mRecordX;
    int                mRecordY;

    int                mCaptureCount;
    QString            mFileformat;
    QString            mLastExportDir;

    void storeState();
    void restoreState();
};

#endif // MAINFORM_H
