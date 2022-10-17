#ifndef MAINFORM_H
#define MAINFORM_H

#include <QWidget>
#include <QList>
#include <QTimer>
#include "workerthread.h"
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

private:
    Ui::MainForm      *ui;
    PreviewWidget     *mPreview;
    QTimer             mRecorderTimer;
    WorkerThread       mThread;
    int                mRecordWidth;
    int                mRecordHeight;

    int                mCaptureCount;
    QList<QByteArray>  mFrames;
    int                mRecorderSize;
    QString            mLastExportDir;


    void storeState();
    void restoreState();
};

#endif // MAINFORM_H
