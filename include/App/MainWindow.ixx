module;

#include <QMainWindow>
#include <QCloseEvent>

export module App.MainWindow;

export namespace StockTool::App {

class MainWindow final : public QMainWindow {
private:
  class Impl;
  Impl *impl_;

protected:
  void closeEvent(QCloseEvent *event) override;

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;
};

} // namespace StockTool::App
