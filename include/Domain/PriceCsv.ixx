module;

#include <QString>
#include <QStringList>
#include <vector>

export module Domain.PriceCsv;

import Domain.FactorModel;

export namespace StockTool::Domain {

struct PriceCsvData {
  bool ok = false;
  QString error;
  QString sourcePath;
  QStringList headers;
  QStringList dates;
  std::vector<std::vector<double>> pricesByColumn;
};

PriceCsvData loadPriceCsv(const QString &filePath);

std::vector<FactorObservation>
buildDailyReturnObservations(const PriceCsvData &data);

FactorSpec inferFactorSpec(const PriceCsvData &data);

QStringList priceCsvSummary(const PriceCsvData &data);

} // namespace StockTool::Domain
