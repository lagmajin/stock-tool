module;

#include <QFormLayout>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

module Dock.SymbolSearchPane;

import Dock.SymbolSearchPane;

namespace StockTool::Dock {

namespace {

QString normalizedSymbol(const QString &raw) {
  return raw.trimmed().toUpper();
}

}

class SymbolSearchPane::Impl {
public:
  QLineEdit *tickerEdit = nullptr;
  std::function<void(const QString &)> submitHandler;
  std::function<void(const QString &)> addWatchHandler;
};

SymbolSearchPane::SymbolSearchPane(QWidget *parent)
    : QWidget(parent), impl_(new Impl()) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(10);

  auto *title = new QLabel("Symbol Search", this);
  QFont titleFont = title->font();
  titleFont.setBold(true);
  title->setFont(titleFont);

  auto *inputGroup = new QGroupBox("Query", this);
  auto *form = new QFormLayout(inputGroup);

  auto *tickerEdit = new QLineEdit(inputGroup);
  tickerEdit->setPlaceholderText("AAPL / 7203.T / NVDA");
  impl_->tickerEdit = tickerEdit;

  auto *marketEdit = new QLineEdit(inputGroup);
  marketEdit->setPlaceholderText("NASDAQ / TSE / NYSE");

  form->addRow("Ticker", tickerEdit);
  form->addRow("Market", marketEdit);

  auto *buttonRow = new QHBoxLayout();
  auto *fetchButton = new QPushButton("Fetch", this);
  auto *addWatchButton = new QPushButton("Add Watch", this);
  buttonRow->addWidget(fetchButton);
  buttonRow->addWidget(addWatchButton);

  layout->addWidget(title);
  layout->addWidget(inputGroup);
  layout->addLayout(buttonRow);
  layout->addStretch(1);

  const auto submitCurrent = [this]() {
    const QString value = symbol();
    if (!value.isEmpty() && impl_->submitHandler) {
      impl_->submitHandler(value);
    }
  };

  connect(fetchButton, &QPushButton::clicked, this, submitCurrent);
  connect(tickerEdit, &QLineEdit::returnPressed, this, submitCurrent);
  connect(addWatchButton, &QPushButton::clicked, this, [this]() {
    const QString value = symbol();
    if (!value.isEmpty() && impl_->addWatchHandler) {
      impl_->addWatchHandler(value);
    }
  });
}

SymbolSearchPane::~SymbolSearchPane() { delete impl_; }

void SymbolSearchPane::setSubmitHandler(
    std::function<void(const QString &)> handler) {
  impl_->submitHandler = std::move(handler);
}

void SymbolSearchPane::setAddWatchHandler(
    std::function<void(const QString &)> handler) {
  impl_->addWatchHandler = std::move(handler);
}

QString SymbolSearchPane::symbol() const {
  if (!impl_->tickerEdit) {
    return {};
  }
  return normalizedSymbol(impl_->tickerEdit->text());
}

} // namespace StockTool::Dock
