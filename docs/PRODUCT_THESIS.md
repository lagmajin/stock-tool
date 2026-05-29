# Product Thesis

`stock-tool` is not just a price viewer. The core idea is to compare the label on an asset with the real economic bet underneath it.

## Surface Name vs Real Position

Common labels:

- Toyota stock
- Japan equities
- AI stocks
- High dividend stocks

Possible real exposures:

- USD/JPY long
- China growth bet
- NASDAQ high beta
- Semiconductor cycle
- Lower-rate bet
- Risk-on asset
- Short credit spread

The tool should make this mismatch visible.

## MVP

1. Load price CSV files.
2. Calculate daily returns.
3. Select one target asset and multiple factors.
4. Run multi-factor regression.
5. Show beta, R squared, residual mean, and residual series.
6. Plot actual return vs estimated return.

## First Factor Set

- Nikkei 225
- TOPIX
- USD/JPY
- S&P 500
- NASDAQ 100
- SOXX
- China index
- VIX
- US 10Y yield
- crude oil
- copper
- gold

## Long-Term Features

- Degraded index checker
- Hidden beta detector
- Residual alpha tracker
- Residual clustering for new themes
- Portfolio compression by true risk exposure
