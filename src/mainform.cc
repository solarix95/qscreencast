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
#include <QComboBox>
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
    , mCaptureCount(0)
{
    ui->setupUi(this);
    mPreview = new PreviewWidget();
    mScreen  = nullptr;

    connect(ui->btnTest, &QPushButton::clicked, this, [&](){
        initRecorder();
        QTimer::singleShot(1000,this, [&](){
            screenShot(true);
        });
        mPreview->setPreview(QPixmap());
    });

    connect(ui->btnStart, &QPushButton::clicked, this, [&](){
        initRecorder();
        if (ui->chkStream->isChecked()) {
            if (prepareCast().isEmpty())
                return;
            mCache.startStream();
        }
        mRecorderTimer.start(1000/ui->spbFrames->value());
    });

    connect(ui->btnStop, &QPushButton::clicked, this, [&](){
        if (mRecorderTimer.isActive()) {
            mRecorderTimer.stop();
            updateUi();
            return;
        }
        resetRecorder();
    });

    connect(ui->btnFullscreen, &QRadioButton::clicked, this, [&]() {
       updateUi();
    });
    connect(ui->btnSection, &QRadioButton::clicked, this, [&]() {
       updateUi();
    });

    connect(ui->btnExport, &QPushButton::clicked, this, &MainForm::exportRecorder);

    connect(&mRecorderTimer, &QTimer::timeout, this,[&](){
        screenShot(false);
    });

    connect(&mThread, &WorkerThread::processedFrame, this, &MainForm::appendFrame);
    connect(&mThread, &WorkerThread::frameProcessed, this, [this]() {
         updateUi();
    });

    mThread.start();
    mCache.start();

    ui->btnExport->setEnabled(false);

    restoreState();
    updateUi();
}

//-------------------------------------------------------------------------------------------------
MainForm::~MainForm()
{
    mThread.shutdown();
    mThread.wait();

    mCache.shutdown();
    mCache.wait();

    delete ui;
}

//-------------------------------------------------------------------------------------------------
void MainForm::screenShot(bool preview)
{
    if (!mScreen)
        return;

     QElapsedTimer t;
     t.start();

     QPixmap *originalPixmap = new QPixmap(mScreen->grabWindow(0, mRecordX,mRecordY,mRecordWidth, mRecordHeight));
     mCaptureCount++;

     if (preview) {
        ui->lblTime->setText(QString("%1 [ms]").arg(t.elapsed()));
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
void MainForm::appendFrame(QByteArray imgData)
{
    mCache.enqueue(imgData);
    ui->lblBufferSize->setText(QString::number(mCache.cacheSize()/1000000.0));
    updateUi();
}

//-------------------------------------------------------------------------------------------------
void MainForm::exportRecorder()
{
    QString newCastName = prepareCast();
    if (newCastName.isEmpty())
        return;

    if (ui->chkCreateToc->isChecked())
        mCache.dumpToDisk(newCastName,ui->spbFrames->value(),mRecordWidth,mRecordHeight);
    else
        mCache.dumpToDisk();

    QMessageBox::information(this,"Export..", "..done!");
}

//-------------------------------------------------------------------------------------------------
void MainForm::resetRecorder()
{
    mCache.reset();
    ui->lblSkipped->setText("0");
    ui->lblFrames->setText("0");
    ui->lblBufferSize->setText("0");
    ui->lblFrames->setText("0");
    ui->lblProcessed->setText("0");

    mCaptureCount = 0;
    mThread.reset();
    mThread.setFilterFreezeFrames(ui->chkFilterDoubles->isChecked());
    updateUi();
}

//-------------------------------------------------------------------------------------------------
void MainForm::closeEvent(QCloseEvent *event)
{
    mPreview->hide();
    storeState();
    QWidget::closeEvent(event);
}

//-------------------------------------------------------------------------------------------------
void MainForm::initRecorder()
{
    resetRecorder();

    int screenIndex = ui->spbScreen->value();
    if (screenIndex >= QGuiApplication::screens().count())
        screenIndex = -1;

    mScreen = screenIndex < 0 ? QGuiApplication::primaryScreen() : QGuiApplication::screens()[screenIndex];
     if (!mScreen)
         mScreen = windowHandle()->screen();

     if (!mScreen)
         return;

     QSize screenSize = mScreen->availableSize();
    if (ui->btnFullscreen->isChecked()) {
        mRecordX = 0;
        mRecordY = 0;
        mRecordWidth = screenSize.width();
        mRecordHeight= screenSize.height();
    } else {
        mRecordX = ui->spbX->value();
        mRecordY = ui->spbY->value();
        mRecordWidth = ui->spbWidth->value();
        mRecordHeight= ui->spbHeight->value();

        // center screen section
        if (mRecordX < 0)
            mRecordX = (screenSize.width() - mRecordWidth)/2;
        if (mRecordY < 0)
            mRecordY = (screenSize.height() - mRecordHeight)/2;
    }

    // Validate Screen Size
    if ((mRecordWidth + mRecordX) > screenSize.width())
        mRecordWidth = screenSize.width() - mRecordX;
    if ((mRecordHeight + mRecordY) > screenSize.height())
        mRecordHeight = screenSize.height() - mRecordY;

    // Update Export Fileformat Infos
    mThread.setImageFormat(ui->cbxFormat->currentText());
    mFileformat = ui->cbxFormat->currentText();
    if (mFileformat.contains("-"))
        mFileformat = mFileformat.split("-").first(); // "JPG-70" -> "JPG";
    ui->lblFormat->setText(mFileformat);
}

//-------------------------------------------------------------------------------------------------
void MainForm::prepareExport()
{

}

//-------------------------------------------------------------------------------------------------
void MainForm::updateUi()
{
    ui->groupSection->setEnabled(ui->btnSection->isChecked());

    ui->lblFrames->setText(QString::number(mCaptureCount));
    ui->lblProcessed->setText(QString::number(mThread.processedFrames()));
    ui->lblExportFrames->setText(QString::number(mCache.count()));

    ui->btnExport->setEnabled(!mRecorderTimer.isActive() && mCache.count() > 0 && (mThread.processedFrames() == mCaptureCount));
    ui->cbxFormat->setEnabled(!mRecorderTimer.isActive());
    ui->chkStream->setEnabled(!mRecorderTimer.isActive());
}

//-------------------------------------------------------------------------------------------------
QString MainForm::castName() const
{
    QString newCastName = ui->edtProjectName->text().trimmed().simplified();
    if (newCastName.isEmpty()) {
        newCastName  = QString("%1-%2")
                .arg("Screencast-")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-sss"));
    }
    return newCastName;
}

//-------------------------------------------------------------------------------------------------
QString MainForm::prepareCast()
{
    QString export2Dir = QFileDialog::getExistingDirectory(this,"Export path",mLastExportDir);
    if (export2Dir.isEmpty())
        return "";

    mLastExportDir = export2Dir;

    QString newCastName = castName();

    export2Dir = QString("%1/%2")
                    .arg(export2Dir)
                    .arg(newCastName);

    QDir d;
    d.mkpath(export2Dir);

    QString fileExt = mFileformat.toLower();
    mCache.setup(fileExt,export2Dir);
    return newCastName;
}

//-------------------------------------------------------------------------------------------------
void MainForm::storeState()
{
    QSettings settings;

    settings.setValue("source/x", ui->spbX->value());
    settings.setValue("source/y", ui->spbY->value());
    settings.setValue("source/width", ui->spbWidth->value());
    settings.setValue("source/height", ui->spbHeight->value());
    settings.setValue("source/screen", ui->spbScreen->value());
    settings.setValue("source/fps",    ui->spbFrames->value());
    settings.setValue("source/fullscreen", ui->btnFullscreen->isChecked());

    settings.setValue("export/path", mLastExportDir);
    settings.setValue("export/castname", ui->edtProjectName->text().trimmed());

    // Flags
    settings.setValue("options/filterfreeze", ui->chkFilterDoubles->isChecked());
    settings.setValue("options/createtoc", ui->chkCreateToc->isChecked());
    settings.setValue("options/streamtohd", ui->chkStream->isChecked());
}

//-------------------------------------------------------------------------------------------------
void MainForm::restoreState()
{
    QSettings settings;

    ui->spbX->setValue(settings.value("source/x",-1).toInt());
    ui->spbY->setValue(settings.value("source/y",-1).toInt());
    ui->spbWidth->setValue(settings.value("source/width",800).toInt());
    ui->spbHeight->setValue(settings.value("source/height",600).toInt());
    ui->spbScreen->setValue(settings.value("source/screen",-1).toInt());
    ui->btnFullscreen->setChecked(settings.value("source/fullscreen",true).toBool());
    ui->btnSection->setChecked(!settings.value("source/fullscreen",true).toBool());
    ui->spbFrames->setValue(settings.value("source/fps",25).toInt());

    mLastExportDir = settings.value("export/path").toString();
    ui->edtProjectName->setText(settings.value("source/castname","").toString());


    // Stream
    ui->chkFilterDoubles->setChecked(settings.value("options/filterfreeze",false).toBool());
    ui->chkCreateToc->setChecked(settings.value("options/createtoc",false).toBool());
    ui->chkStream->setChecked(settings.value("options/streamtohd",false).toBool());
}
