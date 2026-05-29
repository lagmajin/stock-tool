module;

#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

module Dock.AnalysisPane;

import Dock.AnalysisPane;
import Domain.FactorModel;

namespace StockTool::Dock {
namespace {

using StockTool::Domain::FactorModelResult;
using StockTool::Domain::FactorObservation;
using StockTool::Domain::FactorSpec;

QString formatBeta(double value) {
  return QString::number(value, 'f', 2);
}

QString formatPercent(double value) {
  return QString("%1%").arg(QString::number(value * 100.0, 'f', 2));
}

std::vector<FactorObservation> sampleRowsFor(const QString &symbol) {
  const QString normalized = symbol.trimmed().toUpper();
  const double techTilt =
      (normalized.contains("8035") || normalized.contains("NVDA") ||
       normalized.contains("9984"))
          ? 0.55
          : 0.10;
  const double chinaTilt =
      (normalized.contains("7203") || normalized.contains("6301"))
          ? 0.28
          : 0.08;
  const double currencyTilt =
      (normalized.contains("7203") || normalized.contains("8058"))
          ? 0.34
          : 0.12;

  const double topixBeta = 0.42;
  const double usdJpyBeta = currencyTilt;
  const double nasdaqBeta = techTilt;
  const double chinaBeta = chinaTilt;
  const double vixBeta = -0.26;

  std::vector<FactorObservation> rows;
  rows.reserve(18);

  for (int i = 0; i < 18; ++i) {
    const double wave = static_cast<double>((i % 5) - 2) * 0.0018;
    const double topix = wave + static_cast<double>((i % 3) - 1) * 0.0011;
    const double usdJpy = static_cast<double>(((i + 1) % 4) - 1) * 0.0015;
    const double nasdaq = static_cast<double>(((i + 2) % 6) - 2) * 0.0022;
    const double china = static_cast<double>(((i + 3) % 5) - 2) * 0.0017;
    const double vix = -nasdaq * 0.7 + static_cast<double>((i % 2) - 1) * 0.0010;
    const double residual = static_cast<double>((i % 4) - 1) * 0.00025;

    const double target = 0.0003 + topixBeta * topix + usdJpyBeta * usdJpy +
                          nasdaqBeta * nasdaq + chinaBeta * china +
                          vixBeta * vix + residual;

    rows.push_back({target, {topix, usdJpy, nasdaq, china, vix}});
  }

  return rows;
}

FactorModelResult runSampleFactorModel(const QString &symbol) {
  FactorSpec spec;
  spec.factorNames = {"TOPIX", "USDJPY", "NASDAQ100", "China", "VIX"};
  spec.includeIntercept = true;
  return StockTool::Domain::runFactorRegression(spec, sampleRowsFor(symbol));
}

} // namespace

class AnalysisPane::Impl {
public:
  QLabel *titleLabel = nullptr;
  QTableWidget *table = nullptr;
  QTextEdit *notes = nullptr;
};

AnalysisPane::AnalysisPane(QWidget *parent)
    : QWidget(parent), impl_(new Impl()) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(10);

  auto *title = new QLabel("Factor Analysis", this);
  QFont titleFont = title->font();
  titleFont.setBold(true);
  title->setFont(titleFont);
  impl_->titleLabel = title;

  auto *table = new QTableWidget(0, 2, this);
  impl_->table = table;
  table->setHorizontalHeaderLabels({"Metric", "Value"});
  table->verticalHeader()->setVisible(false);
  table->horizontalHeader()->setStretchLastSection(true);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);

  auto *notes = new QTextEdit(this);
  impl_->notes = notes;
  notes->setReadOnly(true);
  notes->setPlainText(
      "Select a symbol to estimate visible and hidden factor exposures.");

  layout->addWidget(title);
  layout->addWidget(table);
  layout->addWidget(notes, 1);
}

AnalysisPane::~AnalysisPane() { delete impl_; }

void AnalysisPane::setSymbol(const QString &symbol) {
  const QString normalized = symbol.trimmed().toUpper();
  if (!impl_->titleLabel || !impl_->table || !impl_->notes) {
    return;
  }

  impl_->table->setRowCount(0);

  if (normalized.isEmpty()) {
    impl_->titleLabel->setText("Factor Analysis");
    impl_->notes->setPlainText(
        "Select a symbol to estimate visible and hidden factor exposures.");
    return;
  }

  const FactorModelResult result = runSampleFactorModel(normalized);
  impl_->titleLabel->setText(QString("Factor Analysis - %1").arg(normalized));

  if (!result.ok) {
    impl_->notes->setPlainText(QString::fromStdString(result.error));
    return;
  }

  auto addRow = [this](const QString &metric, const QString &value) {
    const int row = impl_->table->rowCount();
    impl_->table->insertRow(row);
    impl_->table->setItem(row, 0, new QTableWidgetItem(metric));
    impl_->table->setItem(row, 1, new QTableWidgetItem(value));
  };

  for (const auto &exposure : result.exposures) {
    addRow(QString::fromStdString(exposure.name) + " beta",
           formatBeta(exposure.beta));
  }

  addRow("R squared", formatPercent(result.rSquared));
  addRow("Residual alpha / day", formatPercent(result.residualMean));
  addRow("Residual volatility", formatPercent(result.residualStdDev));
  addRow("Verdict", QString::fromStdString(result.verdict));
  impl_->table->resizeColumnsToContents();

  auto strongest = std::max_element(
      result.exposures.begin(), result.exposures.end(),
      [](const auto &lhs, const auto &rhs) {
        return std::abs(lhs.beta) < std::abs(rhs.beta);
      });

  QString leadFactor = "unknown";
  if (strongest != result.exposures.end()) {
    leadFactor = QString::fromStdString(strongest->name);
  }

  impl_->notes->setPlainText(
      QString("%1 is currently modeled as a composite position.\n\n"
              "Lead factor: %2\n"
              "Model fit: %3\n\n"
              "This is sample data for the MVP shell. The next step is CSV "
              "return loading so these betas come from real price history.")
          .arg(normalized, leadFactor, formatPercent(result.rSquared)));
}

} // namespace StockTool::Dock
