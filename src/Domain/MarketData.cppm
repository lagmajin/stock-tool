module;

#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QTextStream>
#include <QTimeZone>
#include <algorithm>
#include <cctype>
#include <functional>
#include <limits>
#include <memory>
#include <utility>

module Domain.MarketData;

import Domain.MarketData;
import Domain.PriceCsv;

namespace StockTool::Domain {
namespace {

bool isDigitsOnly(const QString &value) {
  if (value.isEmpty()) {
    return false;
  }
  return std::all_of(value.begin(), value.end(),
                     [](QChar ch) { return ch.isDigit(); });
}

QString normalizedProviderSymbol(const QString &symbol) {
  return symbol.trimmed();
}

QString stooqSymbolFromInput(const QString &raw) {
  const QString trimmed = normalizedProviderSymbol(raw);
  if (trimmed.isEmpty()) {
    return trimmed;
  }
  if (trimmed.startsWith('^')) {
    return trimmed.toLower();
  }

  const QString lowered = trimmed.toLower();
  if (lowered.contains('.')) {
    return lowered;
  }
  if (isDigitsOnly(lowered) && lowered.size() <= 5) {
    return lowered + ".jp";
  }
  return lowered + ".us";
}

QString yahooSymbolFromInput(const QString &raw) {
  const QString trimmed = normalizedProviderSymbol(raw);
  if (trimmed.isEmpty()) {
    return trimmed;
  }

  if (trimmed.startsWith('^')) {
    return trimmed.toUpper();
  }

  const QString upper = trimmed.toUpper();
  if (upper.endsWith(".JP")) {
    return upper.left(upper.size() - 3) + ".T";
  }
  if (upper.endsWith(".US")) {
    return upper.left(upper.size() - 3);
  }
  if (isDigitsOnly(upper)) {
    return upper + ".T";
  }
  return trimmed;
}

QByteArray httpGet(const QUrl &url, QString *errorOut) {
  QNetworkAccessManager manager;
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QStringLiteral("stock-tool/0.1"));

  QNetworkReply *reply = manager.get(request);
  QObject::connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (reply->error() != QNetworkReply::NoError) {
    if (errorOut) {
      *errorOut = reply->errorString();
    }
    return {};
  }

  return reply->readAll();
}

QStringList splitCsvLine(const QString &line) {
  QStringList cells;
  QString current;
  bool inQuotes = false;
  for (const QChar ch : line) {
    if (ch == '"') {
      inQuotes = !inQuotes;
      continue;
    }
    if (ch == ',' && !inQuotes) {
      cells.push_back(current.trimmed());
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  cells.push_back(current.trimmed());
  return cells;
}

bool parseDoubleCell(const QString &cell, double &value) {
  bool ok = false;
  value = cell.toDouble(&ok);
  return ok;
}

MarketSeries parseStooqCsv(const QString &symbol, const QByteArray &csvBytes) {
  MarketSeries series;
  series.symbol = symbol;

  QString csvText = QString::fromUtf8(csvBytes);
  QTextStream stream(&csvText);
  const QString headerLine = stream.readLine();
  if (headerLine.trimmed().isEmpty()) {
    series.error = "Stooq response is empty";
    return series;
  }

  const QStringList headers = splitCsvLine(headerLine);
  const int closeColumn = headers.indexOf("Close");
  if (closeColumn < 0) {
    series.error = "Stooq CSV does not contain a Close column";
    return series;
  }

  while (!stream.atEnd()) {
    const QString line = stream.readLine();
    if (line.trimmed().isEmpty()) {
      continue;
    }
    const QStringList cells = splitCsvLine(line);
    if (cells.size() <= closeColumn) {
      continue;
    }

    double closeValue = 0.0;
    if (!parseDoubleCell(cells[closeColumn], closeValue)) {
      continue;
    }

    series.dates.push_back(cells.value(0));
    series.closes.push_back(closeValue);
  }

  if (series.closes.size() < 2) {
    series.error = "Stooq CSV did not contain enough rows";
    return series;
  }

  series.ok = true;
  return series;
}

MarketSeries parseYahooChart(const QString &symbol, const QByteArray &jsonBytes) {
  MarketSeries series;
  series.symbol = symbol;

  const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
  if (!doc.isObject()) {
    series.error = "Yahoo Finance response was not JSON";
    return series;
  }

  const QJsonObject chart = doc.object().value("chart").toObject();
  const QJsonArray results = chart.value("result").toArray();
  if (results.isEmpty()) {
    const QJsonObject errorObj = chart.value("error").toObject();
    series.error = errorObj.value("description").toString("Yahoo Finance returned no data");
    return series;
  }

  const QJsonObject result = results.first().toObject();
  const QJsonArray timestamps = result.value("timestamp").toArray();
  const QJsonObject indicators = result.value("indicators").toObject();
  const QJsonArray quoteArray = indicators.value("quote").toArray();
  if (timestamps.isEmpty() || quoteArray.isEmpty()) {
    series.error = "Yahoo Finance payload missing series data";
    return series;
  }

  const QJsonArray closes = quoteArray.first().toObject().value("close").toArray();
  const QJsonArray adjCloseArray = indicators.value("adjclose").toArray();
  QJsonArray selectedCloses = closes;
  if (!adjCloseArray.isEmpty()) {
    const QJsonArray adjClose = adjCloseArray.first().toObject().value("adjclose").toArray();
    if (!adjClose.isEmpty()) {
      selectedCloses = adjClose;
    }
  }

  const int count = std::min(timestamps.size(), selectedCloses.size());
  for (int i = 0; i < count; ++i) {
    if (selectedCloses[i].isNull()) {
      continue;
    }
    const double closeValue = selectedCloses[i].toDouble();
    const qint64 ts = static_cast<qint64>(timestamps[i].toDouble());
    const QDate date =
        QDateTime::fromSecsSinceEpoch(ts, QTimeZone::utc()).date();
    series.dates.push_back(date.toString(Qt::ISODate));
    series.closes.push_back(closeValue);
  }

  if (series.closes.size() < 2) {
    series.error = "Yahoo Finance payload did not contain enough rows";
    return series;
  }

  series.ok = true;
  return series;
}

} // namespace

MarketProvider marketProviderFromString(const QString &value) {
  const QString normalized = value.trimmed().toLower();
  if (normalized.contains("yahoo")) {
    return MarketProvider::YahooFinance;
  }
  return MarketProvider::Stooq;
}

QString marketProviderToString(MarketProvider provider) {
  switch (provider) {
  case MarketProvider::Stooq:
    return "Stooq";
  case MarketProvider::YahooFinance:
    return "Yahoo Finance";
  }
  return "Stooq";
}

QString normalizeMarketSymbol(MarketProvider provider, const QString &symbol) {
  switch (provider) {
  case MarketProvider::Stooq:
    return stooqSymbolFromInput(symbol);
  case MarketProvider::YahooFinance:
    return yahooSymbolFromInput(symbol);
  }
  return symbol.trimmed();
}

MarketSeries fetchCloseSeries(MarketProvider provider, const QString &symbol) {
  MarketSeries series;
  series.symbol = normalizeMarketSymbol(provider, symbol);
  if (series.symbol.trimmed().isEmpty()) {
    series.error = "Symbol is empty";
    return series;
  }

  QString error;
  QUrl url;
  switch (provider) {
  case MarketProvider::Stooq:
    url = QUrl(QStringLiteral("https://stooq.com/q/d/l/"));
    {
      QUrlQuery query;
      query.addQueryItem("s", series.symbol);
      query.addQueryItem("i", "d");
      url.setQuery(query);
    }
    break;
  case MarketProvider::YahooFinance:
    url = QUrl(QStringLiteral("https://query1.finance.yahoo.com/v8/finance/chart/%1")
                   .arg(QString::fromLatin1(QUrl::toPercentEncoding(series.symbol))));
    {
      QUrlQuery query;
      query.addQueryItem("interval", "1d");
      query.addQueryItem("range", "5y");
      query.addQueryItem("includePrePost", "false");
      query.addQueryItem("events", "div,splits");
      url.setQuery(query);
    }
    break;
  }

  const QByteArray body = httpGet(url, &error);
  if (body.isEmpty()) {
    series.error = QString("Failed to download %1 data for %2: %3")
                       .arg(marketProviderToString(provider), series.symbol, error);
    return series;
  }

  switch (provider) {
  case MarketProvider::Stooq:
    series = parseStooqCsv(series.symbol, body);
    break;
  case MarketProvider::YahooFinance:
    series = parseYahooChart(series.symbol, body);
    break;
  }

  if (!series.ok && series.error.isEmpty()) {
    series.error = QString("No usable rows were returned for %1").arg(series.symbol);
  }
  return series;
}

PriceCsvData buildPriceCsvFromSeries(const QString &sourceLabel,
                                     const MarketSeries &targetSeries,
                                     const QStringList &factorNames,
                                     const std::vector<MarketSeries> &factorSeries) {
  PriceCsvData data;
  data.sourcePath = sourceLabel;
  data.headers.clear();
  data.headers << "Date" << targetSeries.symbol;
  for (const QString &factorName : factorNames) {
    data.headers << factorName;
  }

  if (!targetSeries.ok) {
    data.error = targetSeries.error;
    return data;
  }

  const int factorCount = static_cast<int>(factorSeries.size());
  if (factorCount != factorNames.size()) {
    data.error = "Factor name count does not match fetched factor series count";
    return data;
  }

  std::vector<QHash<QString, double>> factorLookups;
  factorLookups.reserve(factorSeries.size());
  for (const auto &factor : factorSeries) {
    if (!factor.ok) {
      data.error = factor.error;
      return data;
    }

    QHash<QString, double> lookup;
    for (int i = 0; i < factor.dates.size() && i < factor.closes.size(); ++i) {
      lookup.insert(factor.dates[i], factor.closes[static_cast<std::size_t>(i)]);
    }
    factorLookups.push_back(std::move(lookup));
  }

  QHash<QString, double> targetLookup;
  for (int i = 0; i < targetSeries.dates.size() && i < targetSeries.closes.size(); ++i) {
    targetLookup.insert(targetSeries.dates[i],
                        targetSeries.closes[static_cast<std::size_t>(i)]);
  }

  data.pricesByColumn.resize(static_cast<std::size_t>(factorCount + 2));
  for (const QString &date : targetSeries.dates) {
    if (!targetLookup.contains(date)) {
      continue;
    }

    bool allPresent = true;
    std::vector<double> factorValues;
    factorValues.reserve(static_cast<std::size_t>(factorCount));
    for (const auto &lookup : factorLookups) {
      if (!lookup.contains(date)) {
        allPresent = false;
        break;
      }
      factorValues.push_back(lookup.value(date));
    }

    if (!allPresent) {
      continue;
    }

    data.dates.push_back(date);
    data.pricesByColumn[0].push_back(0.0);
    data.pricesByColumn[1].push_back(targetLookup.value(date));
    for (int factorIndex = 0; factorIndex < factorCount; ++factorIndex) {
      data.pricesByColumn[static_cast<std::size_t>(factorIndex + 2)].push_back(
          factorValues[static_cast<std::size_t>(factorIndex)]);
    }
  }

  if (data.dates.size() < 2) {
    data.error = "Fetched series did not overlap enough to build a factor dataset";
    return data;
  }

  data.ok = true;
  return data;
}

} // namespace StockTool::Domain
