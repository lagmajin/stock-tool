module;

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

module Domain.FactorModel;

import Domain.FactorModel;

namespace StockTool::Domain {
namespace {

constexpr double kPivotEpsilon = 1.0e-12;

FactorModelResult errorResult(std::string message) {
  FactorModelResult result;
  result.ok = false;
  result.error = std::move(message);
  return result;
}

double mean(const std::vector<double> &values) {
  if (values.empty()) {
    return 0.0;
  }
  const double sum = std::accumulate(values.begin(), values.end(), 0.0);
  return sum / static_cast<double>(values.size());
}

bool solveLinearSystem(std::vector<std::vector<double>> a,
                       std::vector<double> b,
                       std::vector<double> &solution) {
  const int n = static_cast<int>(b.size());
  solution.assign(n, 0.0);

  for (int col = 0; col < n; ++col) {
    int pivot = col;
    double best = std::abs(a[col][col]);
    for (int row = col + 1; row < n; ++row) {
      const double candidate = std::abs(a[row][col]);
      if (candidate > best) {
        best = candidate;
        pivot = row;
      }
    }

    if (best < kPivotEpsilon) {
      return false;
    }

    if (pivot != col) {
      std::swap(a[pivot], a[col]);
      std::swap(b[pivot], b[col]);
    }

    const double divisor = a[col][col];
    for (int j = col; j < n; ++j) {
      a[col][j] /= divisor;
    }
    b[col] /= divisor;

    for (int row = 0; row < n; ++row) {
      if (row == col) {
        continue;
      }

      const double factor = a[row][col];
      for (int j = col; j < n; ++j) {
        a[row][j] -= factor * a[col][j];
      }
      b[row] -= factor * b[col];
    }
  }

  solution = std::move(b);
  return true;
}

std::string verdictFor(const FactorModelResult &result) {
  if (!result.ok || result.exposures.empty()) {
    return {};
  }

  const auto strongest = std::max_element(
      result.exposures.begin(), result.exposures.end(),
      [](const FactorExposure &lhs, const FactorExposure &rhs) {
        return std::abs(lhs.beta) < std::abs(rhs.beta);
      });

  if (strongest == result.exposures.end()) {
    return {};
  }

  const std::string &name = strongest->name;
  const double beta = strongest->beta;

  if (result.rSquared >= 0.75 && std::abs(beta) >= 1.2) {
    return "High-beta proxy for " + name;
  }
  if (result.rSquared >= 0.65) {
    return "Composite exposure led by " + name;
  }
  if (std::abs(result.residualMean) > 0.0003) {
    return "Residual signal worth investigating";
  }
  return "Mixed exposure";
}

} // namespace

FactorModelResult runFactorRegression(
    const FactorSpec &spec, const std::vector<FactorObservation> &rows) {
  const std::size_t factorCount = spec.factorNames.size();
  if (factorCount == 0) {
    return errorResult("factor list is empty");
  }
  if (rows.size() <= factorCount + (spec.includeIntercept ? 1 : 0)) {
    return errorResult("not enough observations for regression");
  }

  for (const auto &row : rows) {
    if (row.factorReturns.size() != factorCount) {
      return errorResult("observation has mismatched factor count");
    }
  }

  const std::size_t parameterCount =
      factorCount + (spec.includeIntercept ? 1 : 0);
  std::vector<std::vector<double>> xtx(
      parameterCount, std::vector<double>(parameterCount, 0.0));
  std::vector<double> xty(parameterCount, 0.0);

  for (const auto &row : rows) {
    std::vector<double> x;
    x.reserve(parameterCount);
    if (spec.includeIntercept) {
      x.push_back(1.0);
    }
    x.insert(x.end(), row.factorReturns.begin(), row.factorReturns.end());

    for (std::size_t i = 0; i < parameterCount; ++i) {
      xty[i] += x[i] * row.targetReturn;
      for (std::size_t j = 0; j < parameterCount; ++j) {
        xtx[i][j] += x[i] * x[j];
      }
    }
  }

  std::vector<double> coefficients;
  if (!solveLinearSystem(std::move(xtx), std::move(xty), coefficients)) {
    return errorResult("factor matrix is singular");
  }

  FactorModelResult result;
  result.ok = true;
  result.intercept = spec.includeIntercept ? coefficients[0] : 0.0;

  const std::size_t betaOffset = spec.includeIntercept ? 1 : 0;
  result.exposures.reserve(factorCount);
  for (std::size_t i = 0; i < factorCount; ++i) {
    result.exposures.push_back({spec.factorNames[i], coefficients[i + betaOffset]});
  }

  std::vector<double> targetReturns;
  targetReturns.reserve(rows.size());
  result.fittedReturns.reserve(rows.size());
  result.residuals.reserve(rows.size());

  for (const auto &row : rows) {
    double fitted = result.intercept;
    for (std::size_t i = 0; i < factorCount; ++i) {
      fitted += row.factorReturns[i] * result.exposures[i].beta;
    }

    targetReturns.push_back(row.targetReturn);
    result.fittedReturns.push_back(fitted);
    result.residuals.push_back(row.targetReturn - fitted);
  }
  result.fitSeries.actualReturns = targetReturns;
  result.fitSeries.fittedReturns = result.fittedReturns;
  result.fitSeries.residuals = result.residuals;

  const double targetMean = mean(targetReturns);
  double sse = 0.0;
  double sst = 0.0;
  for (std::size_t i = 0; i < targetReturns.size(); ++i) {
    sse += result.residuals[i] * result.residuals[i];
    const double centered = targetReturns[i] - targetMean;
    sst += centered * centered;
  }

  result.rSquared = sst > std::numeric_limits<double>::epsilon()
                        ? std::max(0.0, 1.0 - (sse / sst))
                        : 0.0;
  result.residualMean = mean(result.residuals);

  double residualVariance = 0.0;
  for (const double residual : result.residuals) {
    const double centered = residual - result.residualMean;
    residualVariance += centered * centered;
  }
  residualVariance /= static_cast<double>(result.residuals.size());
  result.residualStdDev = std::sqrt(residualVariance);
  result.verdict = verdictFor(result);

  return result;
}

} // namespace StockTool::Domain
