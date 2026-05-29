#include <QApplication>

import App.MainWindow;

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName("stock-tool");
  QApplication::setOrganizationName("lagmajin");

  StockTool::App::MainWindow window;
  window.resize(1440, 900);
  window.show();

  return app.exec();
}
