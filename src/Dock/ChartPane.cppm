module;

#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

module Dock.ChartPane;

import Dock.ChartPane;

namespace StockTool::Dock {

class ChartPane::Impl {
public:
  QLabel *symbolLabel = nullptr;
  QLabel *detailLabel = nullptr;
};

ChartPane::ChartPane(QWidget *parent) : QWidget(parent), impl_(new Impl()) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(10);

  auto *title = new QLabel("Chart", this);
  QFont titleFont = title->font();
  titleFont.setBold(true);
  title->setFont(titleFont);

  auto *surface = new QFrame(this);
  surface->setFrameShape(QFrame::StyledPanel);
  surface->setMinimumHeight(320);

  auto *surfaceLayout = new QGridLayout(surface);
  auto *symbolLabel = new QLabel("No symbol selected", surface);
  QFont symbolFont = symbolLabel->font();
  symbolFont.setPointSize(18);
  symbolFont.setBold(true);
  symbolLabel->setFont(symbolFont);
  impl_->symbolLabel = symbolLabel;

  auto *detailLabel = new QLabel(
      "Chart placeholder\n\nCandlestick / volume / overlays later", surface);
  detailLabel->setWordWrap(true);
  impl_->detailLabel = detailLabel;

  surfaceLayout->addWidget(symbolLabel, 0, 0);
  surfaceLayout->addWidget(detailLabel, 1, 0);

  layout->addWidget(title);
  layout->addWidget(surface, 1);
}

ChartPane::~ChartPane() { delete impl_; }

void ChartPane::setSymbol(const QString &symbol) {
  const QString normalized = symbol.trimmed().toUpper();
  if (!impl_->symbolLabel || !impl_->detailLabel) {
    return;
  }

  if (normalized.isEmpty()) {
    impl_->symbolLabel->setText("No symbol selected");
    impl_->detailLabel->setText(
        "Chart placeholder\n\nCandlestick / volume / overlays later");
    return;
  }

  impl_->symbolLabel->setText(normalized);
  impl_->detailLabel->setText(
      QString("Ready to load chart data for %1.\n\n"
              "Next step:\n"
              "- price history\n"
              "- OHLC candlesticks\n"
              "- volume overlay")
          .arg(normalized));
}

} // namespace StockTool::Dock
