module;

#include <QString>
#include <QStringList>
#include <vector>

export module Domain.MarketData;

import Domain.PriceCsv;

export namespace StockTool::Domain {

enum class MarketProvider {
  Stooq,
  YahooFinance,
};

struct MarketSeries {
  bool ok = false;
  QString error;
  QString symbol;
  QStringList dates;
  std::vector<double> closes;
};

MarketProvider marketProviderFromString(const QString &value);
QString marketProviderToString(MarketProvider provider);
QString normalizeMarketSymbol(MarketProvider provider, const QString &symbol);
MarketSeries fetchCloseSeries(MarketProvider provider, const QString &symbol);
PriceCsvData buildPriceCsvFromSeries(const QString &sourceLabel,
                                     const MarketSeries &targetSeries,
                                     const QStringList &factorNames,
                                     const std::vector<MarketSeries> &factorSeries);

} // namespace StockTool::Domain
