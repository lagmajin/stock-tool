module;

#include <functional>
#include <QString>
#include <QWidget>

export module Dock.ChartPane;

import Domain.FactorModel;

export namespace StockTool::Dock {

class ChartPane final : public QWidget {
private:
  class Impl;
  Impl *impl_;

public:
  explicit ChartPane(QWidget *parent = nullptr);
  ~ChartPane() override;

  void setSymbol(const QString &symbol);
  void setFactorModelResult(
      const QString &contextLabel,
      const StockTool::Domain::FactorModelResult &result);
  void clearModelResult();
};

} // namespace StockTool::Dock
