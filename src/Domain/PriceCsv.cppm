module;

#include <QIODevice>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

module Domain.PriceCsv;

import Domain.PriceCsv;
import Domain.FactorModel;

namespace StockTool::Domain {
namespace {

QString trimmedCell(QString cell) {
  cell = cell.trimmed();
  if (cell.startsWith('"') && cell.endsWith('"') && cell.size() >= 2) {
    cell = cell.mid(1, cell.size() - 2);
  }
  return cell.trimmed();
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
      cells.push_back(trimmedCell(current));
      current.clear();
      continue;
    }
    current.push_back(ch);
  }

  cells.push_back(trimmedCell(current));
  return cells;
}

bool parseDoubleCell(const QString &cell, double &value) {
  bool ok = false;
  value = cell.toDouble(&ok);
  return ok;
}

QString invalidColumnsMessage(const QString &reason) {
  return QString("CSV structure error: %1").arg(reason);
}

} // namespace

PriceCsvData loadPriceCsv(const QString &filePath) {
  PriceCsvData data;
  data.sourcePath = filePath;

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    data.error = QString("Failed to open CSV: %1").arg(file.errorString());
    return data;
  }

  QTextStream stream(&file);
  const QString headerLine = stream.readLine();
  if (headerLine.trimmed().isEmpty()) {
    data.error = "CSV file is empty";
    return data;
  }

  data.headers = splitCsvLine(headerLine);
  if (data.headers.size() < 3) {
    data.error = invalidColumnsMessage(
        "expected Date + target + at least one factor column");
    return data;
  }

  while (!stream.atEnd()) {
    const QString line = stream.readLine();
    if (line.trimmed().isEmpty()) {
      continue;
    }

    const QStringList cells = splitCsvLine(line);
    if (cells.size() != data.headers.size()) {
      data.error = invalidColumnsMessage(
          QString("row has %1 cells but header has %2")
              .arg(cells.size())
              .arg(data.headers.size()));
      data.ok = false;
      return data;
    }

    data.dates.push_back(cells.front());
    if (data.pricesByColumn.empty()) {
      data.pricesByColumn.resize(static_cast<std::size_t>(cells.size()));
    }

    for (int i = 1; i < cells.size(); ++i) {
      double value = 0.0;
      if (!parseDoubleCell(cells[i], value)) {
        data.error = QString("Failed to parse numeric value at column %1")
                         .arg(data.headers[i]);
        return data;
      }
      data.pricesByColumn[static_cast<std::size_t>(i)].push_back(value);
    }
  }

  if (data.dates.size() < 2) {
    data.error = "CSV must contain at least two rows of price history";
    return data;
  }

  data.ok = true;
  return data;
}

std::vector<FactorObservation>
buildDailyReturnObservations(const PriceCsvData &data) {
  std::vector<FactorObservation> observations;
  if (!data.ok || data.pricesByColumn.size() < 3) {
    return observations;
  }

  const std::size_t rowCount = data.pricesByColumn[1].size();
  const std::size_t factorCount = data.pricesByColumn.size() - 2;
  observations.reserve(rowCount > 1 ? rowCount - 1 : 0);

  for (std::size_t row = 1; row < rowCount; ++row) {
    const double targetPrev = data.pricesByColumn[1][row - 1];
    const double targetNow = data.pricesByColumn[1][row];
    if (targetPrev == 0.0) {
      continue;
    }

    std::vector<double> factors;
    factors.reserve(factorCount);
    bool valid = true;

    for (std::size_t column = 2; column < data.pricesByColumn.size(); ++column) {
      const double prev = data.pricesByColumn[column][row - 1];
      const double now = data.pricesByColumn[column][row];
      if (prev == 0.0) {
        valid = false;
        break;
      }
      factors.push_back((now / prev) - 1.0);
    }

    if (!valid) {
      continue;
    }

    observations.push_back({(targetNow / targetPrev) - 1.0, std::move(factors)});
  }

  return observations;
}

FactorSpec inferFactorSpec(const PriceCsvData &data) {
  FactorSpec spec;
  if (!data.ok || data.headers.size() < 3) {
    return spec;
  }

  for (int i = 2; i < data.headers.size(); ++i) {
    spec.factorNames.push_back(data.headers[i].toStdString());
  }
  spec.includeIntercept = true;
  return spec;
}

QStringList priceCsvSummary(const PriceCsvData &data) {
  QStringList lines;
  if (!data.ok) {
    lines << data.error;
    return lines;
  }

  lines << QString("Source: %1").arg(QFileInfo(data.sourcePath).fileName());
  lines << QString("Rows: %1").arg(data.dates.size());
  lines << QString("Target column: %1").arg(data.headers.value(1));
  lines << QString("Factor columns: %1")
               .arg(data.headers.mid(2).join(", "));
  return lines;
}

} // namespace StockTool::Domain
