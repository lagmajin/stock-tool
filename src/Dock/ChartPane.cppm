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
#include <QRectF>
#include <QSizePolicy>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

module Dock.ChartPane;

import Dock.ChartPane;
import Domain.FactorModel;

namespace StockTool::Dock {
namespace {

QString exposureSummary(const StockTool::Domain::FactorModelResult &result) {
  if (!result.ok || result.exposures.empty()) {
    return "No exposure summary available.";
  }

  auto sorted = result.exposures;
  std::sort(sorted.begin(), sorted.end(), [](const auto &lhs, const auto &rhs) {
    return std::abs(lhs.beta) > std::abs(rhs.beta);
  });

  const int topCount = std::min<int>(3, static_cast<int>(sorted.size()));
  QStringList topFactors;
  for (int i = 0; i < topCount; ++i) {
    topFactors << QString::fromStdString(sorted[static_cast<std::size_t>(i)].name);
  }

  const QString lead = QString::fromStdString(sorted.front().name);
  const QString leadUpper = lead.toUpper();

  QString proxyLabel = "Composite risk proxy";
  if (leadUpper.contains("NASDAQ") || leadUpper.contains("SOXX")) {
    proxyLabel = "Degraded NASDAQ / theme proxy";
  } else if (leadUpper.contains("TOPIX") || leadUpper.contains("NIKKEI")) {
    proxyLabel = "Japan equity proxy";
  } else if (leadUpper.contains("CHINA")) {
    proxyLabel = "China cycle proxy";
  } else if (leadUpper.contains("USD") || leadUpper.contains("JPY")) {
    proxyLabel = "Currency proxy";
  } else if (leadUpper.contains("VIX")) {
    proxyLabel = "Risk-off proxy";
  }

  return QString("%1\nTop betas: %2\nFit: %3\nResidual drift: %4")
      .arg(proxyLabel, topFactors.join(", "),
           QString::number(result.rSquared * 100.0, 'f', 1),
           QString::number(result.residualMean * 100.0, 'f', 2));
}

class PlotCanvas final : public QWidget {
public:
  explicit PlotCanvas(QWidget *parent = nullptr) : QWidget(parent) {
    setMinimumHeight(170);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  }

  void setComparisonSeries(QString title, std::vector<double> primary,
                           std::vector<double> secondary) {
    title_ = std::move(title);
    primary_ = std::move(primary);
    secondary_ = std::move(secondary);
    update();
  }

  void setSingleSeries(QString title, std::vector<double> primary) {
    title_ = std::move(title);
    primary_ = std::move(primary);
    secondary_.clear();
    update();
  }

  void clearSeries() {
    title_.clear();
    primary_.clear();
    secondary_.clear();
    update();
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(22, 24, 28));

    const QRectF plotRect(54, 24, width() - 76, height() - 68);
    painter.setPen(QPen(QColor(64, 70, 82), 1));
    painter.drawRect(plotRect);

    painter.setPen(QPen(QColor(225, 230, 238), 1));
    QFont titleFont = painter.font();
    titleFont.setPointSize(10);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRectF(12, 8, width() - 24, 20), title_);

    if (primary_.empty()) {
      painter.setPen(QColor(160, 168, 180));
      painter.setFont(QFont(painter.font().family(), 9));
      painter.drawText(plotRect.adjusted(16, 16, -16, -16), Qt::AlignCenter,
                       "No series loaded yet.");
      return;
    }

    if (primary_.size() < 2) {
      painter.setPen(QColor(160, 168, 180));
      painter.drawText(plotRect.adjusted(16, 16, -16, -16), Qt::AlignCenter,
                       "Not enough data to plot.");
      return;
    }

    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();
    for (std::size_t i = 0; i < primary_.size(); ++i) {
      minValue = std::min(minValue, primary_[i]);
      maxValue = std::max(maxValue, primary_[i]);
      if (i < secondary_.size()) {
        minValue = std::min(minValue, secondary_[i]);
        maxValue = std::max(maxValue, secondary_[i]);
      }
    }

    if (std::abs(maxValue - minValue) < 1e-12) {
      maxValue += 0.0001;
      minValue -= 0.0001;
    }

    const auto mapPoint = [&](std::size_t index, double value) {
      const double x = plotRect.left() +
                       (static_cast<double>(index) /
                        static_cast<double>(primary_.size() - 1)) *
                           plotRect.width();
      const double ratio = (value - minValue) / (maxValue - minValue);
      const double y = plotRect.bottom() - ratio * plotRect.height();
      return QPointF(x, y);
    };

    painter.setPen(QPen(QColor(90, 98, 112), 1, Qt::DashLine));
    if (minValue < 0.0 && maxValue > 0.0) {
      const double zeroY = plotRect.bottom() -
                           ((0.0 - minValue) / (maxValue - minValue)) *
                               plotRect.height();
      painter.drawLine(QPointF(plotRect.left(), zeroY),
                       QPointF(plotRect.right(), zeroY));
    }

    painter.drawLine(QPointF(plotRect.left(), plotRect.top()),
                     QPointF(plotRect.left(), plotRect.bottom()));

    const auto drawPolyline = [&](const std::vector<double> &series,
                                  const QColor &color) {
      QPainterPath path;
      path.moveTo(mapPoint(0, series[0]));
      for (std::size_t i = 1; i < primary_.size(); ++i) {
        path.lineTo(mapPoint(i, series[i]));
      }
      painter.setPen(QPen(color, 2));
      painter.drawPath(path);
    };

    drawPolyline(primary_, QColor(66, 153, 225));
    if (!secondary_.empty()) {
      drawPolyline(secondary_, QColor(237, 137, 54));
    }

    painter.setPen(QColor(210, 214, 220));
    painter.setFont(QFont(painter.font().family(), 8));
    if (secondary_.empty()) {
      painter.drawText(QRectF(plotRect.left(), 2, 110, 16), "Residual");
      painter.fillRect(QRectF(plotRect.left() + 82, 6, 12, 12),
                       QColor(66, 153, 225));
    } else {
      painter.drawText(QRectF(plotRect.left(), 2, 110, 16), "Actual");
      painter.fillRect(QRectF(plotRect.left() + 114, 6, 12, 12),
                       QColor(66, 153, 225));
      painter.drawText(QRectF(plotRect.left() + 132, 2, 110, 16), "Fitted");
      painter.fillRect(QRectF(plotRect.left() + 190, 6, 12, 12),
                       QColor(237, 137, 54));
    }

    painter.drawText(QRectF(plotRect.right() - 128, 2, 128, 16),
                     QString("Range %1 .. %2")
                         .arg(QString::number(minValue * 100.0, 'f', 2),
                              QString::number(maxValue * 100.0, 'f', 2)));
  }

private:
  QString title_;
  std::vector<double> primary_;
  std::vector<double> secondary_;
};

QString defaultPlaceholder() {
  return "Chart placeholder\n\nCandlestick / volume / overlays later";
}

} // namespace

class ChartPane::Impl {
public:
  QLabel *symbolLabel = nullptr;
  QLabel *detailLabel = nullptr;
  PlotCanvas *comparisonCanvas = nullptr;
  PlotCanvas *residualCanvas = nullptr;
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
  surface->setMinimumHeight(420);

  auto *surfaceLayout = new QGridLayout(surface);
  surfaceLayout->setContentsMargins(10, 10, 10, 10);
  surfaceLayout->setSpacing(8);

  auto *symbolLabel = new QLabel("No symbol selected", surface);
  QFont symbolFont = symbolLabel->font();
  symbolFont.setPointSize(18);
  symbolFont.setBold(true);
  symbolLabel->setFont(symbolFont);
  impl_->symbolLabel = symbolLabel;

  auto *detailLabel = new QLabel(defaultPlaceholder(), surface);
  detailLabel->setWordWrap(true);
  detailLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  impl_->detailLabel = detailLabel;

  auto *comparisonCanvas = new PlotCanvas(surface);
  impl_->comparisonCanvas = comparisonCanvas;

  auto *residualCanvas = new PlotCanvas(surface);
  impl_->residualCanvas = residualCanvas;

  surfaceLayout->addWidget(symbolLabel, 0, 0);
  surfaceLayout->addWidget(detailLabel, 1, 0);
  surfaceLayout->addWidget(comparisonCanvas, 2, 0);
  surfaceLayout->addWidget(residualCanvas, 3, 0);
  surfaceLayout->setRowStretch(2, 1);
  surfaceLayout->setRowStretch(3, 1);

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
    impl_->detailLabel->setText(defaultPlaceholder());
    if (impl_->comparisonCanvas) {
      impl_->comparisonCanvas->clearSeries();
    }
    if (impl_->residualCanvas) {
      impl_->residualCanvas->clearSeries();
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
  if (!impl_->comparisonCanvas || !impl_->residualCanvas ||
      !impl_->detailLabel) {
    return;
  }

  impl_->comparisonCanvas->setComparisonSeries(
      QString("%1 actual vs fitted returns").arg(contextLabel),
      result.fitSeries.actualReturns, result.fitSeries.fittedReturns);
  impl_->residualCanvas->setSingleSeries(
      QString("%1 residuals").arg(contextLabel), result.fitSeries.residuals);

  QString details = QString("R squared: %1\nResidual alpha/day: %2\n"
                            "Residual volatility: %3\nVerdict: %4")
                        .arg(QString::number(result.rSquared * 100.0, 'f', 2),
                             QString::number(result.residualMean * 100.0, 'f', 2),
                             QString::number(result.residualStdDev * 100.0, 'f', 2),
                             QString::fromStdString(result.verdict));
  details += "\n\n";
  details += exposureSummary(result);
  impl_->detailLabel->setText(details);
}

void ChartPane::clearModelResult() {
  if (impl_->comparisonCanvas) {
    impl_->comparisonCanvas->clearSeries();
  }
  if (impl_->residualCanvas) {
    impl_->residualCanvas->clearSeries();
  }
  if (impl_->detailLabel) {
    impl_->detailLabel->setText(defaultPlaceholder());
  }
}

} // namespace StockTool::Dock
