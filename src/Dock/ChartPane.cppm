module;

#include <QColor>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>
#include <vector>

module Dock.ChartPane;

import Dock.ChartPane;

namespace StockTool::Dock {
namespace {

class PlotCanvas final : public QWidget {
public:
  explicit PlotCanvas(QWidget *parent = nullptr) : QWidget(parent) {
    setMinimumHeight(320);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  }

  void setSeries(QString title, std::vector<double> actual,
                 std::vector<double> fitted) {
    title_ = std::move(title);
    actual_ = std::move(actual);
    fitted_ = std::move(fitted);
    update();
  }

  void clearSeries() {
    title_.clear();
    actual_.clear();
    fitted_.clear();
    update();
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(22, 24, 28));

    const QRectF plotRect = QRectF(54, 24, width() - 76, height() - 68);
    painter.setPen(QPen(QColor(64, 70, 82), 1));
    painter.drawRect(plotRect);

    painter.setPen(QPen(QColor(225, 230, 238), 1));
    QFont titleFont = painter.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRectF(12, 8, width() - 24, 20), title_);

    if (actual_.empty() || fitted_.empty()) {
      painter.setPen(QColor(160, 168, 180));
      painter.setFont(QFont(painter.font().family(), 10));
      painter.drawText(plotRect.adjusted(16, 16, -16, -16),
                       Qt::AlignCenter, "Load a CSV or fetch web data to see\nactual vs fitted returns.");
      return;
    }

    const std::size_t count = std::min(actual_.size(), fitted_.size());
    if (count < 2) {
      painter.setPen(QColor(160, 168, 180));
      painter.drawText(plotRect.adjusted(16, 16, -16, -16),
                       Qt::AlignCenter, "Not enough data to plot.");
      return;
    }

    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();
    for (std::size_t i = 0; i < count; ++i) {
      minValue = std::min({minValue, actual_[i], fitted_[i]});
      maxValue = std::max({maxValue, actual_[i], fitted_[i]});
    }

    if (std::abs(maxValue - minValue) < 1e-9) {
      maxValue += 0.0001;
      minValue -= 0.0001;
    }

    const auto toPoint = [&](std::size_t index, double value) {
      const double x = plotRect.left() +
                       (static_cast<double>(index) / static_cast<double>(count - 1)) *
                           plotRect.width();
      const double t = (value - minValue) / (maxValue - minValue);
      const double y = plotRect.bottom() - t * plotRect.height();
      return QPointF(x, y);
    };

    painter.setPen(QPen(QColor(90, 98, 112), 1, Qt::DashLine));
    painter.drawLine(QPointF(plotRect.left(), plotRect.bottom()),
                     QPointF(plotRect.right(), plotRect.bottom()));
    painter.drawLine(QPointF(plotRect.left(), plotRect.top()),
                     QPointF(plotRect.left(), plotRect.bottom()));

    auto drawPolyline = [&](const std::vector<double> &series, const QColor &color) {
      QPainterPath path;
      path.moveTo(toPoint(0, series[0]));
      for (std::size_t i = 1; i < count; ++i) {
        path.lineTo(toPoint(i, series[i]));
      }
      painter.setPen(QPen(color, 2));
      painter.drawPath(path);
    };

    drawPolyline(actual_, QColor(66, 153, 225));
    drawPolyline(fitted_, QColor(237, 137, 54));

    painter.setPen(QColor(210, 214, 220));
    painter.setFont(QFont(painter.font().family(), 9));
    painter.drawText(QRectF(plotRect.left(), 2, 110, 18), "Actual");
    painter.fillRect(QRectF(plotRect.left() + 114, 6, 12, 12), QColor(66, 153, 225));
    painter.drawText(QRectF(plotRect.left() + 132, 2, 110, 18), "Fitted");
    painter.fillRect(QRectF(plotRect.left() + 190, 6, 12, 12), QColor(237, 137, 54));
    painter.drawText(QRectF(plotRect.right() - 128, 2, 128, 18),
                     QString("Range %1 .. %2")
                         .arg(QString::number(minValue * 100.0, 'f', 2),
                              QString::number(maxValue * 100.0, 'f', 2)));
  }

private:
  QString title_;
  std::vector<double> actual_;
  std::vector<double> fitted_;
};

} // namespace

class ChartPane::Impl {
public:
  QLabel *symbolLabel = nullptr;
  QLabel *detailLabel = nullptr;
  PlotCanvas *canvas = nullptr;
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
  surface->setMinimumHeight(360);

  auto *surfaceLayout = new QGridLayout(surface);
  surfaceLayout->setContentsMargins(10, 10, 10, 10);
  surfaceLayout->setSpacing(8);

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

  auto *canvas = new PlotCanvas(surface);
  impl_->canvas = canvas;

  surfaceLayout->addWidget(symbolLabel, 0, 0);
  surfaceLayout->addWidget(detailLabel, 1, 0);
  surfaceLayout->addWidget(canvas, 2, 0);
  surfaceLayout->setRowStretch(2, 1);

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
    if (impl_->canvas) {
      impl_->canvas->clearSeries();
    }
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

void ChartPane::setFactorModelResult(
    const QString &contextLabel,
    const StockTool::Domain::FactorModelResult &result) {
  if (!impl_->canvas || !impl_->detailLabel) {
    return;
  }

  impl_->canvas->setSeries(
      QString("%1 actual vs fitted returns").arg(contextLabel),
      result.fitSeries.actualReturns, result.fitSeries.fittedReturns);

  impl_->detailLabel->setText(
      QString("R squared: %1\nResidual alpha/day: %2\nResidual volatility: %3")
          .arg(QString::number(result.rSquared * 100.0, 'f', 2),
               QString::number(result.residualMean * 100.0, 'f', 2),
               QString::number(result.residualStdDev * 100.0, 'f', 2)));
}

void ChartPane::clearModelResult() {
  if (impl_->canvas) {
    impl_->canvas->clearSeries();
  }
  if (impl_->detailLabel) {
    impl_->detailLabel->setText(
        "Chart placeholder\n\nCandlestick / volume / overlays later");
  }
}

} // namespace StockTool::Dock
