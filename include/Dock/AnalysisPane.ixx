module;

#include <QString>
#include <QWidget>

export module Dock.AnalysisPane;

export namespace StockTool::Dock {

class AnalysisPane final : public QWidget {
private:
  class Impl;
  Impl *impl_;

public:
  explicit AnalysisPane(QWidget *parent = nullptr);
  ~AnalysisPane() override;

  void setSymbol(const QString &symbol);
};

} // namespace StockTool::Dock
