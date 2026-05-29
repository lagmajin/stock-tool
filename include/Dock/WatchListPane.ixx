module;

#include <functional>
#include <QString>
#include <QWidget>

export module Dock.WatchListPane;

export namespace StockTool::Dock {

class WatchListPane final : public QWidget {
private:
  class Impl;
  Impl *impl_;

public:
  explicit WatchListPane(QWidget *parent = nullptr);
  ~WatchListPane() override;

  void setOpenSymbolHandler(std::function<void(const QString &)> handler);
  void addSymbol(const QString &symbol);
  QString currentSymbol() const;
};

} // namespace StockTool::Dock
