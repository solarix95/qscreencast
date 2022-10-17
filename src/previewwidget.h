#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <QPixmap>
#include <QWidget>

namespace Ui {
class PreviewWidget;
}

class PreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    ~PreviewWidget();

    void setPreview(QPixmap previewImage);

private slots:
    void exportPreview();

private:
    Ui::PreviewWidget *ui;
};

#endif // PREVIEWWIDGET_H
