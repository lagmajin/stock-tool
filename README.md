# stock-tool

Qt Widgets + QADS + C++20 modules (`.ixx` / `.cppm`) based stock analysis tool.

The goal is to compare an asset's surface label with its real factor exposure:

- Is a "Japan stock" actually a USD/JPY and China cycle bet?
- Is a theme stock just degraded NASDAQ high beta?
- Is a diversified portfolio secretly one large risk-on position?

See [docs/PRODUCT_THESIS.md](docs/PRODUCT_THESIS.md).

## Current structure

- `src/main.cpp`
- `include/App/MainWindow.ixx`
- `src/App/MainWindow.cppm`
- `include/Domain/FactorModel.ixx`
- `include/Dock/*.ixx`
- `src/Dock/*.cppm`

## Planned panes

- Symbol Search
- Watch List
- Chart
- Factor Analysis

## Build notes

This project expects:

- `Qt6` with `Core`, `Gui`, `Widgets`
- `Qt Advanced Docking System` exposed as `ads::qtadvanceddocking-qt6`

Example configure command:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --build build
```

If QADS is installed through vcpkg, configure with your vcpkg toolchain and triplet.

The sample CSV used by the initial factor-analysis flow is
[data/sample_factor_history.csv](data/sample_factor_history.csv).
