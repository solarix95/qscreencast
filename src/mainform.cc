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
    ui->btnExport->setEnabled(false);

    restoreState();
    updateUi();
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
void MainForm::appendFrame(QByteArray pngData)
{
    mFrames << pngData;
    mRecorderSize += pngData.size();
    ui->lblBufferSize->setText(QString::number(mRecorderSize/1000000.0));
    updateUi();
}

//-------------------------------------------------------------------------------------------------
void MainForm::exportRecorder()
{
    QString export2Dir = QFileDialog::getExistingDirectory(this,"Export path",mLastExportDir);
    if (export2Dir.isEmpty())
        return;

    mLastExportDir = export2Dir;

    QString castName = ui->edtProjectName->text().trimmed().simplified();
    if (castName.isEmpty())
        castName  = QString("%1-%2")
                .arg("Screencast-")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-sss"));

    export2Dir = QString("%1/%2")
                    .arg(export2Dir)
                    .arg(castName);

    QDir d;
    d.mkpath(export2Dir);
    QStringList fileNames;
    QString fileExt = mFileformat.toLower();
    for(int i=0; i<mFrames.count(); i++) {
        QFile f(QString("%1/%2.%3").arg(export2Dir).arg(i+1,6,10,QChar('0')).arg(fileExt));
        if (!f.open(QIODevice::WriteOnly))
            return;
        f.write(mFrames[i]);
        fileNames << f.fileName();
    }

    if (ui->chkCreateToc->isChecked()) {
        createCinelerraToc(fileNames,export2Dir + "/" + castName + ".toc");
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
void MainForm::updateUi()
{
    ui->groupSection->setEnabled(ui->btnSection->isChecked());

    ui->lblFrames->setText(QString::number(mCaptureCount));
    ui->lblProcessed->setText(QString::number(mThread.processedFrames()));
    ui->lblExportFrames->setText(QString::number(mFrames.count()));

    ui->btnExport->setEnabled(!mRecorderTimer.isActive() && mFrames.count() > 0 && (mThread.processedFrames() == mCaptureCount));
    ui->cbxFormat->setEnabled(!mRecorderTimer.isActive());
    ui->chkStream->setEnabled(!mRecorderTimer.isActive());
}

//-------------------------------------------------------------------------------------------------
void MainForm::createCinelerraToc(const QStringList &fileNames, const QString &tocFilename) const
{
    if (fileNames.isEmpty())
        return;

    if (!fileNames.first().toLower().endsWith(".png")) // cinelerra TOC only with PNG
        return;

    QFile f(tocFilename);
    if (!f.open(QIODevice::WriteOnly))
        return;
    f.write("PNGLIST\n");
    f.write("#CREATED BY QScreenCast\n");
    f.write(QString("%1 #FPS\n").arg(ui->spbFrames->value()).toUtf8());
    f.write(QString("%1 #Width\n").arg(mRecordWidth).toUtf8());
    f.write(QString("%1 #Height\n").arg(mRecordHeight).toUtf8());
    f.write("#FILES\n");
    for (auto name: fileNames)
        f.write(QString("%1\n").arg(name).toUtf8());
    f.close();
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
}
