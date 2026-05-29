module;

#include <functional>
#include <QString>
#include <QWidget>

export module Dock.SymbolSearchPane;

export namespace StockTool::Dock {

class SymbolSearchPane final : public QWidget {
private:
  class Impl;
  Impl *impl_;

public:
  explicit SymbolSearchPane(QWidget *parent = nullptr);
  ~SymbolSearchPane() override;

  void setSubmitHandler(std::function<void(const QString &)> handler);
  void setAddWatchHandler(std::function<void(const QString &)> handler);
  QString symbol() const;
};

} // namespace StockTool::Dock
