#include <QPushButton>
#include <QFileDialog>
#include "previewwidget.h"
#include "ui_previewwidget.h"

//-------------------------------------------------------------------------------------------------
PreviewWidget::PreviewWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PreviewWidget)
{
    ui->setupUi(this);

    connect(ui->btnSave, &QPushButton::clicked, this, &PreviewWidget::exportPreview);
}

//-------------------------------------------------------------------------------------------------
PreviewWidget::~PreviewWidget()
{
    delete ui;
}

//-------------------------------------------------------------------------------------------------
void PreviewWidget::setPreview(QPixmap previewImage)
{
    ui->preview->setImage(previewImage);
    ui->btnSave->setEnabled(!previewImage.isNull());
    show();
    raise();
}

//-------------------------------------------------------------------------------------------------
void PreviewWidget::exportPreview()
{
    if (ui->preview->image().isNull())
        return;

    static QString lastExportDir;
    QString selectedFileName = QFileDialog::getSaveFileName(this,"Preview Export", lastExportDir,  trUtf8("Images (*.png *.jpg)"));
    if (selectedFileName.isEmpty())
        return;

    if (!selectedFileName.contains("."))
        selectedFileName += ".png";

    ui->preview->image().save(selectedFileName);
    lastExportDir = selectedFileName;
}
