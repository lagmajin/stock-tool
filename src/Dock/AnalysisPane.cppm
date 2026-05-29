module;

#include <QAbstractItemView>
#include <QComboBox>
#include <QFutureWatcher>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

module Dock.AnalysisPane;

import Dock.AnalysisPane;
import Domain.FactorModel;
import Domain.MarketData;
import Domain.PriceCsv;

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

QString formatPathLabel(const QString &path) {
  if (path.trimmed().isEmpty()) {
    return "No CSV loaded";
  }
  return QFileInfo(path).fileName();
}

QStringList splitSymbols(const QString &text) {
  return text.split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
}

struct WebFetchResult {
  StockTool::Domain::PriceCsvData data;
  QString statusMessage;
};

FactorModelResult runSampleFactorModel(const QString &symbol) {
  FactorSpec spec;
  spec.factorNames = {"TOPIX", "USDJPY", "NASDAQ100", "China", "VIX"};
  spec.includeIntercept = true;

  std::vector<FactorObservation> rows;
  rows.reserve(18);

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

  for (int i = 0; i < 18; ++i) {
    const double wave = static_cast<double>((i % 5) - 2) * 0.0018;
    const double topix = wave + static_cast<double>((i % 3) - 1) * 0.0011;
    const double usdJpy = static_cast<double>(((i + 1) % 4) - 1) * 0.0015;
    const double nasdaq = static_cast<double>(((i + 2) % 6) - 2) * 0.0022;
    const double china = static_cast<double>(((i + 3) % 5) - 2) * 0.0017;
    const double vix = -nasdaq * 0.7 + static_cast<double>((i % 2) - 1) * 0.0010;
    const double residual = static_cast<double>((i % 4) - 1) * 0.00025;

    const double target = 0.0003 + 0.42 * topix + currencyTilt * usdJpy +
                          techTilt * nasdaq + chinaTilt * china +
                          -0.26 * vix + residual;
    rows.push_back({target, {topix, usdJpy, nasdaq, china, vix}});
  }

  return StockTool::Domain::runFactorRegression(spec, rows);
}

FactorModelResult runCsvFactorModel(const StockTool::Domain::PriceCsvData &data) {
  const FactorSpec spec = StockTool::Domain::inferFactorSpec(data);
  const std::vector<FactorObservation> rows =
      StockTool::Domain::buildDailyReturnObservations(data);
  return StockTool::Domain::runFactorRegression(spec, rows);
}

StockTool::Domain::PriceCsvData fetchWebFactorCsv(
    StockTool::Domain::MarketProvider provider, const QString &targetSymbol,
    const QStringList &factorSymbols, QString *statusMessage) {
  using namespace StockTool::Domain;

  const QString normalizedTarget = normalizeMarketSymbol(provider, targetSymbol);
  if (normalizedTarget.trimmed().isEmpty()) {
    if (statusMessage) {
      *statusMessage = "Target symbol is empty";
    }
    return {};
  }

  MarketSeries targetSeries = fetchCloseSeries(provider, normalizedTarget);
  if (!targetSeries.ok) {
    if (statusMessage) {
      *statusMessage = targetSeries.error;
    }
    return {};
  }

  std::vector<MarketSeries> factorSeries;
  factorSeries.reserve(factorSymbols.size());
  QStringList normalizedFactorNames;
  for (const QString &factor : factorSymbols) {
    const QString normalizedFactor = normalizeMarketSymbol(provider, factor);
    if (normalizedFactor.trimmed().isEmpty()) {
      continue;
    }
    MarketSeries series = fetchCloseSeries(provider, normalizedFactor);
    if (!series.ok) {
      if (statusMessage) {
        *statusMessage = QString("%1: %2").arg(normalizedFactor, series.error);
      }
      return {};
    }
    normalizedFactorNames << normalizedFactor;
    factorSeries.push_back(std::move(series));
  }

  if (factorSeries.empty()) {
    if (statusMessage) {
      *statusMessage = "Please provide at least one factor symbol";
    }
    return {};
  }

  PriceCsvData data = buildPriceCsvFromSeries(
      QString("%1/%2").arg(marketProviderToString(provider), normalizedTarget),
      targetSeries, normalizedFactorNames, factorSeries);
  if (!data.ok && statusMessage) {
    *statusMessage = data.error;
  }
  return data;
}

WebFetchResult fetchWebFactorCsvAsync(StockTool::Domain::MarketProvider provider,
                                      QString targetSymbol,
                                      QStringList factorSymbols) {
  WebFetchResult result;
  result.data = fetchWebFactorCsv(provider, targetSymbol, factorSymbols,
                                  &result.statusMessage);
  return result;
}

} // namespace

class AnalysisPane::Impl {
public:
  std::function<void(const QString &,
                     const StockTool::Domain::FactorModelResult &)> resultHandler;
  QLabel *titleLabel = nullptr;
  QLabel *sourceLabel = nullptr;
  QComboBox *providerBox = nullptr;
  QLineEdit *targetEdit = nullptr;
  QLineEdit *factorsEdit = nullptr;
  QTableWidget *table = nullptr;
  QTextEdit *notes = nullptr;
  QToolButton *loadButton = nullptr;
  QToolButton *recalcButton = nullptr;
  QToolButton *webFetchButton = nullptr;
  QFutureWatcher<WebFetchResult> *webFetchWatcher = nullptr;
  StockTool::Domain::PriceCsvData loadedCsv;
  QString selectedSymbol;
  QString loadedPath;
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

  auto *toolbar = new QHBoxLayout();
  toolbar->setSpacing(8);

  auto *loadButton = new QToolButton(this);
  loadButton->setText("Load CSV");
  loadButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  impl_->loadButton = loadButton;

  auto *recalcButton = new QToolButton(this);
  recalcButton->setText("Recalculate");
  recalcButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  impl_->recalcButton = recalcButton;

  auto *sourceLabel = new QLabel("No CSV loaded", this);
  sourceLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  impl_->sourceLabel = sourceLabel;

  toolbar->addWidget(loadButton);
  toolbar->addWidget(recalcButton);
  toolbar->addWidget(sourceLabel, 1);

  auto *webRow = new QHBoxLayout();
  webRow->setSpacing(8);

  auto *providerBox = new QComboBox(this);
  providerBox->addItem("Stooq", QVariant::fromValue(0));
  providerBox->addItem("Yahoo Finance", QVariant::fromValue(1));
  impl_->providerBox = providerBox;

  auto *targetEdit = new QLineEdit(this);
  targetEdit->setPlaceholderText("Target symbol");
  targetEdit->setText("7203");
  impl_->targetEdit = targetEdit;

  auto *factorsEdit = new QLineEdit(this);
  factorsEdit->setPlaceholderText("Factor symbols separated by comma");
  factorsEdit->setText("^GSPC,^IXIC,USDJPY=X");
  impl_->factorsEdit = factorsEdit;

  auto *webFetchButton = new QToolButton(this);
  webFetchButton->setText("Fetch Web");
  impl_->webFetchButton = webFetchButton;

  webRow->addWidget(providerBox);
  webRow->addWidget(targetEdit, 1);
  webRow->addWidget(factorsEdit, 2);
  webRow->addWidget(webFetchButton);

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
      "Load a CSV with columns in this order:\n"
      "Date, Target, Factor1, Factor2, ...\n\n"
      "The app will calculate daily returns and run factor regression.");

  layout->addWidget(title);
  layout->addLayout(toolbar);
  layout->addLayout(webRow);
  layout->addWidget(table);
  layout->addWidget(notes, 1);

  connect(loadButton, &QToolButton::clicked, this, [this]() {
    const QString filePath = QFileDialog::getOpenFileName(
        this, "Load price CSV", QString(), "CSV Files (*.csv);;All Files (*.*)");
    if (filePath.isEmpty()) {
      return;
    }

    impl_->loadedCsv = StockTool::Domain::loadPriceCsv(filePath);
    impl_->loadedPath = filePath;
    if (!impl_->loadedCsv.ok) {
      impl_->sourceLabel->setText(formatPathLabel(filePath));
      impl_->notes->setPlainText(impl_->loadedCsv.error);
      impl_->table->setRowCount(0);
      return;
    }

    impl_->sourceLabel->setText(formatPathLabel(filePath));
    recalcFromCurrentContext();
  });

  connect(recalcButton, &QToolButton::clicked, this,
          [this]() { recalcFromCurrentContext(); });

  connect(webFetchButton, &QToolButton::clicked, this,
          [this]() { loadFromWebSource(); });

  impl_->webFetchWatcher = new QFutureWatcher<WebFetchResult>(this);
  connect(impl_->webFetchWatcher, &QFutureWatcher<WebFetchResult>::finished,
          this, [this]() {
            setWebFetchBusy(false);
            const WebFetchResult result = impl_->webFetchWatcher->result();
            if (!result.data.ok) {
              impl_->notes->setPlainText(result.statusMessage);
              impl_->table->setRowCount(0);
              impl_->sourceLabel->setText("Web fetch failed");
              return;
            }

            impl_->loadedCsv = result.data;
            impl_->loadedPath = result.data.sourcePath;
            impl_->sourceLabel->setText(result.data.sourcePath);
            recalcFromCurrentContext();
          });
}

AnalysisPane::~AnalysisPane() { delete impl_; }

void AnalysisPane::setResultHandler(
    std::function<void(const QString &,
                       const StockTool::Domain::FactorModelResult &)> handler) {
  impl_->resultHandler = std::move(handler);
}

void AnalysisPane::setSymbol(const QString &symbol) {
  impl_->selectedSymbol = symbol.trimmed().toUpper();
  if (impl_->selectedSymbol.isEmpty()) {
    impl_->titleLabel->setText("Factor Analysis");
    if (impl_->loadedCsv.ok) {
      recalcFromCurrentContext();
    }
    return;
  }

  impl_->titleLabel->setText(
      QString("Factor Analysis - %1").arg(impl_->selectedSymbol));
  if (impl_->loadedCsv.ok) {
    recalcFromCurrentContext();
    return;
  }

  const FactorModelResult result = runSampleFactorModel(impl_->selectedSymbol);
  renderResult(result, "Sample data");
}

void AnalysisPane::recalcFromCurrentContext() {
  if (impl_->loadedCsv.ok) {
    const FactorModelResult result = runCsvFactorModel(impl_->loadedCsv);
    renderResult(result, impl_->loadedPath);
    return;
  }

  if (!impl_->selectedSymbol.isEmpty()) {
    const FactorModelResult result = runSampleFactorModel(impl_->selectedSymbol);
    renderResult(result, "Sample data");
    return;
  }

  impl_->table->setRowCount(0);
  impl_->notes->setPlainText(
      "Load a CSV with columns in this order:\n"
      "Date, Target, Factor1, Factor2, ...\n\n"
      "The app will calculate daily returns and run factor regression.");
}

void AnalysisPane::loadFromWebSource() {
  if (!impl_->providerBox || !impl_->targetEdit || !impl_->factorsEdit) {
    return;
  }

  const auto provider = static_cast<StockTool::Domain::MarketProvider>(
      impl_->providerBox->currentIndex() == 1 ? 1 : 0);
  const QString targetInput = impl_->targetEdit->text();
  const QStringList factorInputs = splitSymbols(impl_->factorsEdit->text());

  if (impl_->webFetchWatcher && impl_->webFetchWatcher->isRunning()) {
    return;
  }

  setWebFetchBusy(true);
  impl_->notes->setPlainText(
      QString("Fetching %1 series for %2 ...")
          .arg(impl_->providerBox->currentText(), targetInput.trimmed()));
  impl_->sourceLabel->setText("Fetching web data...");

  const auto future = QtConcurrent::run(
      [provider, targetInput, factorInputs]() {
        return fetchWebFactorCsvAsync(provider, targetInput, factorInputs);
      });
  impl_->webFetchWatcher->setFuture(future);
}

void AnalysisPane::setWebFetchBusy(bool busy) {
  if (!impl_->loadButton || !impl_->recalcButton || !impl_->webFetchButton ||
      !impl_->providerBox || !impl_->targetEdit || !impl_->factorsEdit) {
    return;
  }

  impl_->loadButton->setEnabled(!busy);
  impl_->recalcButton->setEnabled(!busy);
  impl_->webFetchButton->setEnabled(!busy);
  impl_->providerBox->setEnabled(!busy);
  impl_->targetEdit->setEnabled(!busy);
  impl_->factorsEdit->setEnabled(!busy);
}

void AnalysisPane::renderResult(const FactorModelResult &result,
                                const QString &contextLabel) {
  if (!impl_->titleLabel || !impl_->table || !impl_->notes) {
    return;
  }

  impl_->table->setRowCount(0);

  if (!result.ok) {
    impl_->notes->setPlainText(QString::fromStdString(result.error));
    if (impl_->resultHandler) {
      impl_->resultHandler(contextLabel, result);
    }
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

  addRow("Intercept", formatPercent(result.intercept));
  addRow("R squared", formatPercent(result.rSquared));
  addRow("Residual alpha / day", formatPercent(result.residualMean));
  addRow("Residual volatility", formatPercent(result.residualStdDev));
  addRow("Verdict", QString::fromStdString(result.verdict));
  impl_->table->resizeColumnsToContents();

  QString leadFactor = "unknown";
  auto strongest = std::max_element(
      result.exposures.begin(), result.exposures.end(),
      [](const auto &lhs, const auto &rhs) {
        return std::abs(lhs.beta) < std::abs(rhs.beta);
      });
  if (strongest != result.exposures.end()) {
    leadFactor = QString::fromStdString(strongest->name);
  }

  const QString summary =
      impl_->loadedCsv.ok
          ? QString("%1\n\nRows of return data: %2\nLead factor: %3\nModel fit: %4\n"
                    "This is now driven by CSV input, so you can start feeding "
                    "real factor history into the shell.")
                .arg(contextLabel)
                .arg(StockTool::Domain::buildDailyReturnObservations(
                         impl_->loadedCsv)
                         .size())
                .arg(leadFactor)
                .arg(formatPercent(result.rSquared))
          : QString("Lead factor: %1\nModel fit: %2\n\n"
                    "This is still sample data until a CSV is loaded.")
                .arg(leadFactor, formatPercent(result.rSquared));

  impl_->notes->setPlainText(summary);

  if (impl_->resultHandler) {
    impl_->resultHandler(contextLabel, result);
  }
}

} // namespace StockTool::Dock
