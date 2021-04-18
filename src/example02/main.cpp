#include "ExampleWidget.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ExampleWidget widget;
    widget.show();

    return app.exec();
}
