#include <QDebug>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>
#include <QWindow>
#include <QPixmap>
#include <QTimer>
#include <QElapsedTimer>
#include <QTime>
#include <QFileDialog>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>

#include "mainform.h"
#include "ui_mainform.h"

//-------------------------------------------------------------------------------------------------
MainForm::MainForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainForm)
{
    ui->setupUi(this);
    mPreview = new PreviewWidget();

    connect(ui->btnTest, &QPushButton::clicked, this, [&](){
        mRecordWidth  = ui->spbWidth->value();
        mRecordHeight = ui->spbHeight->value();
        QTimer::singleShot(1000,this, [&](){
            screenShot(true);
        });
        mPreview->setPreview(QPixmap());
    });

    connect(ui->btnStart, &QPushButton::clicked, this, [&](){
        resetRecorder();
        mRecordWidth  = ui->spbWidth->value();
        mRecordHeight = ui->spbHeight->value();
        mRecorderTimer.start(1000/ui->spbFrames->value());
    });

    connect(ui->btnStop, &QPushButton::clicked, this, [&](){
        if (mRecorderTimer.isActive()) {
            mRecorderTimer.stop();
            return;
        }
        resetRecorder();
    });

    connect(ui->btnExport, &QPushButton::clicked, this, &MainForm::exportRecorder);

    connect(&mRecorderTimer, &QTimer::timeout, this,[&](){
        screenShot(false);
    });

    connect(&mThread, &WorkerThread::processedPng, this, &MainForm::appendFrame);
    mThread.start();
    ui->btnExport->setEnabled(false);

    restoreState();
}

//-------------------------------------------------------------------------------------------------
MainForm::~MainForm()
{
    mThread.shutdown();
    mThread.wait();
    delete ui;
}

//-------------------------------------------------------------------------------------------------
void MainForm::screenShot(bool preview)
{
    int screenIndex = ui->spbScreen->value();
    if (screenIndex >= QGuiApplication::screens().count())
        screenIndex = -1;

    QScreen *screen = screenIndex < 0 ? QGuiApplication::primaryScreen() : QGuiApplication::screens()[screenIndex];
     if (!screen)
         screen = windowHandle()->screen();

     if (!screen)
         return;

     QElapsedTimer t;
     t.start();

     int w = mRecordWidth;
     int h = mRecordHeight;
     QSize s = screen->size();

     QPixmap *originalPixmap = new QPixmap(screen->grabWindow(0, (s.width()-w)/2,(s.height()-h)/2,w,h));
     mCaptureCount++;

     if (preview) {
        ui->lblTime->setText(QString::number(t.elapsed()));
        mPreview->setPreview(*originalPixmap);
        delete originalPixmap;
        return;
     }

     if (t.elapsed() > mRecorderTimer.interval()) {
         ui->lblSkipped->setText(QString::number(ui->lblSkipped->text().toInt() + 1));
         ui->lblDebug->setText(QString("%1 [ms]").arg(t.elapsed()));
     }

     ui->lblFrames->setText(QString::number(mCaptureCount));
     mThread.queue(originalPixmap);
}

//-------------------------------------------------------------------------------------------------
void MainForm::appendFrame(QByteArray pngData)
{
    mFrames << pngData;
    mRecorderSize += pngData.size();
    ui->lblBufferSize->setText(QString::number(mRecorderSize/1000000.0));
    ui->lblProcessed->setText(QString::number(mFrames.count()));
    ui->btnExport->setEnabled(mFrames.count() == mCaptureCount);
}

//-------------------------------------------------------------------------------------------------
void MainForm::exportRecorder()
{
    QString export2Dir = QFileDialog::getExistingDirectory(this,"Export path",mLastExportDir);
    if (export2Dir.isEmpty())
        return;

    mLastExportDir = export2Dir;

    export2Dir = QString("%1/%2-%3")
                    .arg(export2Dir)
                    .arg("Screencast-")
                    .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-sss"));
    QDir d;
    d.mkpath(export2Dir);
    for(int i=0; i<mFrames.count(); i++) {
        QFile f(QString("%1/%2.png").arg(export2Dir).arg(i+1,6,10,QChar('0')));
        if (!f.open(QIODevice::WriteOnly))
            return;
        f.write(mFrames[i]);
    }

    QMessageBox::information(this,"Export..", "..done!");
}

//-------------------------------------------------------------------------------------------------
void MainForm::resetRecorder()
{
    mFrames.clear();
    ui->lblSkipped->setText("0");
    ui->lblFrames->setText("0");
    ui->lblBufferSize->setText("0");
    ui->lblFrames->setText("0");
    ui->lblProcessed->setText("0");

    mRecorderSize = 0;
    mCaptureCount = 0;
    mThread.reset();
}

//-------------------------------------------------------------------------------------------------
void MainForm::closeEvent(QCloseEvent *event)
{
    mPreview->hide();
    storeState();
    QWidget::closeEvent(event);
}

//-------------------------------------------------------------------------------------------------
void MainForm::storeState()
{
    QSettings settings;
    settings.setValue("source/width", ui->spbWidth->value());
    settings.setValue("source/height", ui->spbHeight->value());
    settings.setValue("source/screen", ui->spbScreen->value());
    settings.setValue("source/fps",    ui->spbFrames->value());

    settings.setValue("export/path", mLastExportDir);
}

//-------------------------------------------------------------------------------------------------
void MainForm::restoreState()
{
    QSettings settings;

    ui->spbWidth->setValue(settings.value("source/width",800).toInt());
    ui->spbHeight->setValue(settings.value("source/height",600).toInt());
    ui->spbScreen->setValue(settings.value("source/screen",-1).toInt());
    ui->spbFrames->setValue(settings.value("source/fps",25).toInt());

    mLastExportDir = settings.value("export/path").toString();
}
