#include <QApplication>
#include "mainform.h"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("solarix-oss");
    QCoreApplication::setApplicationName("quickscreencast");

    MainForm form;
    form.show();

    return app.exec();
}
