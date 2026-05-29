module;

#include <string>
#include <vector>

export module Domain.FactorModel;

export namespace StockTool::Domain {

struct FactorObservation {
  double targetReturn = 0.0;
  std::vector<double> factorReturns;
};

struct FactorSpec {
  std::vector<std::string> factorNames;
  bool includeIntercept = true;
};

struct FactorFitSeries {
  std::vector<double> actualReturns;
  std::vector<double> fittedReturns;
  std::vector<double> residuals;
};

struct FactorExposure {
  std::string name;
  double beta = 0.0;
};

struct FactorModelResult {
  bool ok = false;
  std::string error;
  std::vector<FactorExposure> exposures;
  double intercept = 0.0;
  double rSquared = 0.0;
  double residualMean = 0.0;
  double residualStdDev = 0.0;
  std::vector<double> fittedReturns;
  std::vector<double> residuals;
  std::string verdict;
  FactorFitSeries fitSeries;
};

FactorModelResult runFactorRegression(const FactorSpec &spec,
                                      const std::vector<FactorObservation> &rows);

} // namespace StockTool::Domain
