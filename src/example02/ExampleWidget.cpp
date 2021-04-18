#include "ExampleWidget.hpp"

#include <ui_ExampleWidget.h>

#include <QDebug>

ExampleWidget::ExampleWidget(QWidget *parent)
    : ParentType(parent)
    , ui(std::make_unique<Ui::ExampleWidget>())
{
    ui->setupUi(this);

    connect(ui->pushButtonTest, &QPushButton::clicked, this, []() { qDebug() << tr("Test"); });
    connect(ui->pushButtonExit, &QPushButton::clicked, this, &ExampleWidget::close);
}

ExampleWidget::~ExampleWidget()
{
}