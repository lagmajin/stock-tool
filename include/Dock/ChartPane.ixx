module;

#include <QString>
#include <QWidget>

export module Dock.ChartPane;

export namespace StockTool::Dock {

class ChartPane final : public QWidget {
private:
  class Impl;
  Impl *impl_;

public:
  explicit ChartPane(QWidget *parent = nullptr);
  ~ChartPane() override;

  void setSymbol(const QString &symbol);
};

} // namespace StockTool::Dock
