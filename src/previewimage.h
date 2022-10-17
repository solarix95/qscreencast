#ifndef PREVIEWIMAGE_H
#define PREVIEWIMAGE_H

#include <QWidget>
#include <QPixmap>
#include <QImage>

class PreviewImage : public QWidget
{
public:
    PreviewImage(QWidget *parent);

    void   setImage(QPixmap img);
    QImage image() const;

protected:
    virtual void paintEvent(QPaintEvent *event);

private:
    QImage mImage;
};

#endif // PREVIEWIMAGE_H
