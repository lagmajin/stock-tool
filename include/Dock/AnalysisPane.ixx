module;

#include <QComboBox>
#include <QFutureWatcher>
#include <functional>
#include <QString>
#include <QLineEdit>
#include <QWidget>

export module Dock.AnalysisPane;

import Domain.FactorModel;

export namespace StockTool::Dock {

class AnalysisPane final : public QWidget {
private:
  class Impl;
  Impl *impl_;

public:
  explicit AnalysisPane(QWidget *parent = nullptr);
  ~AnalysisPane() override;

  void setSymbol(const QString &symbol);
  void setResultHandler(
      std::function<void(const QString &label,
                         const StockTool::Domain::FactorModelResult &result)>
          handler);

private:
  void recalcFromCurrentContext();
  void loadFromWebSource();
  void setWebFetchBusy(bool busy);
  void renderResult(const StockTool::Domain::FactorModelResult &result,
                    const QString &contextLabel);
};

} // namespace StockTool::Dock
