#include <QWidget>

#include <memory>

namespace Ui
{
class ExampleWidget;
}

class ExampleWidget : public QWidget
{
    using ParentType = QWidget;

    Q_OBJECT

public:
    explicit ExampleWidget(QWidget *parent = nullptr);
    virtual ~ExampleWidget();

private:
    std::unique_ptr<Ui::ExampleWidget> ui;
};
