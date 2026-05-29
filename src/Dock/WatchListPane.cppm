module;

#include <QFont>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

module Dock.WatchListPane;

import Dock.WatchListPane;

namespace StockTool::Dock {

class WatchListPane::Impl {
public:
  QListWidget *list = nullptr;
  std::function<void(const QString &)> openSymbolHandler;
};

WatchListPane::WatchListPane(QWidget *parent)
    : QWidget(parent), impl_(new Impl()) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(10);

  auto *title = new QLabel("Watch List", this);
  QFont titleFont = title->font();
  titleFont.setBold(true);
  title->setFont(titleFont);

  auto *list = new QListWidget(this);
  impl_->list = list;
  list->addItems({"AAPL", "MSFT", "NVDA", "7203.T", "9984.T"});

  auto *button = new QPushButton("Open Selected", this);

  layout->addWidget(title);
  layout->addWidget(list, 1);
  layout->addWidget(button);

  connect(button, &QPushButton::clicked, this, [this]() {
    const QString value = currentSymbol();
    if (!value.isEmpty() && impl_->openSymbolHandler) {
      impl_->openSymbolHandler(value);
    }
  });

  connect(list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
    const QString value = currentSymbol();
    if (!value.isEmpty() && impl_->openSymbolHandler) {
      impl_->openSymbolHandler(value);
    }
  });
}

WatchListPane::~WatchListPane() { delete impl_; }

void WatchListPane::setOpenSymbolHandler(
    std::function<void(const QString &)> handler) {
  impl_->openSymbolHandler = std::move(handler);
}

void WatchListPane::addSymbol(const QString &symbol) {
  const QString normalized = symbol.trimmed().toUpper();
  if (normalized.isEmpty() || !impl_->list) {
    return;
  }

  const auto items =
      impl_->list->findItems(normalized, Qt::MatchFixedString | Qt::MatchCaseSensitive);
  if (!items.isEmpty()) {
    impl_->list->setCurrentItem(items.front());
    return;
  }

  impl_->list->insertItem(0, normalized);
  impl_->list->setCurrentRow(0);
}

QString WatchListPane::currentSymbol() const {
  if (!impl_->list || !impl_->list->currentItem()) {
    return {};
  }
  return impl_->list->currentItem()->text().trimmed().toUpper();
}

} // namespace StockTool::Dock
