#include <QPainter>
#include <QDebug>
#include "previewimage.h"

//-------------------------------------------------------------------------------------------------
PreviewImage::PreviewImage(QWidget *parent)
 : QWidget(parent)
{
}

//-------------------------------------------------------------------------------------------------
void PreviewImage::setImage(QPixmap img)
{
    mImage = img.toImage();
    update();
}

//-------------------------------------------------------------------------------------------------
QImage PreviewImage::image() const
{
    return mImage;
}

//-------------------------------------------------------------------------------------------------
void PreviewImage::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setPen(Qt::black);
    p.setBrush(Qt::black);
    p.drawRect(0,0,width(),height());

    if (mImage.isNull())
        return;

    QImage renderImg = mImage.scaled(width(),height(),Qt::KeepAspectRatio);
    p.drawImage((width()-renderImg.width())/2,(height()-renderImg.height())/2,renderImg);
}
