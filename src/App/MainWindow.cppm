module;

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <QByteArray>
#include <QCloseEvent>
#include <QFrame>
#include <QFont>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QSettings>
#include <QStatusBar>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

module App.MainWindow;

import App.MainWindow;
import Dock.AnalysisPane;
import Dock.ChartPane;
import Dock.SymbolSearchPane;
import Dock.WatchListPane;

namespace StockTool::App {

class MainWindow::Impl {
public:
  ads::CDockManager *dockManager = nullptr;
  StockTool::Dock::SymbolSearchPane *searchPane = nullptr;
  StockTool::Dock::WatchListPane *watchListPane = nullptr;
  StockTool::Dock::ChartPane *chartPane = nullptr;
  StockTool::Dock::AnalysisPane *analysisPane = nullptr;
  QLabel *overviewSymbolLabel = nullptr;
  QTextEdit *overviewNotes = nullptr;
  QString currentSymbol;
};

namespace {

constexpr auto kSettingsGroup = "workspace";
constexpr auto kLayoutKey = "dockLayout";
constexpr int kLayoutVersion = 1;

QString normalizedSymbol(const QString &raw) {
  return raw.trimmed().toUpper();
}

QWidget *createOverviewWidget(QLabel **overviewSymbolLabelOut,
                              QTextEdit **overviewNotesOut,
                              QWidget *parent) {
  auto *root = new QFrame(parent);
  root->setObjectName("overviewWidget");

  auto *layout = new QVBoxLayout(root);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(12);

  auto *title = new QLabel("Market Overview", root);
  QFont titleFont = title->font();
  titleFont.setPointSize(16);
  titleFont.setBold(true);
  title->setFont(titleFont);

  auto *summary = new QLabel(
      "Start with a simple multi-pane workspace: search a symbol, inspect a "
      "chart, and review indicator output.",
      root);
  summary->setWordWrap(true);

  auto *overviewSymbolLabel = new QLabel("Selected Symbol: none", root);
  QFont symbolFont = overviewSymbolLabel->font();
  symbolFont.setPointSize(13);
  symbolFont.setBold(true);
  overviewSymbolLabel->setFont(symbolFont);
  if (overviewSymbolLabelOut) {
    *overviewSymbolLabelOut = overviewSymbolLabel;
  }

  auto *highlights = new QListWidget(root);
  highlights->addItem("Top pane target: daily price snapshot");
  highlights->addItem("Next step: wire market data provider");
  highlights->addItem("Later: add candlestick chart and screening rules");

  auto *notes = new QTextEdit(root);
  notes->setReadOnly(true);
  notes->setPlainText(
      "This center panel can later become a dashboard or a multi-chart "
      "workspace. Keeping it simple now makes the dock layout easier to "
      "stabilize.");
  if (overviewNotesOut) {
    *overviewNotesOut = notes;
  }

  layout->addWidget(title);
  layout->addWidget(summary);
  layout->addWidget(overviewSymbolLabel);
  layout->addWidget(highlights, 1);
  layout->addWidget(notes, 2);

  return root;
}

void configureDockManager(ads::CDockManager *dockManager) {
  ads::CDockManager::setConfigFlag(ads::CDockManager::DefaultOpaqueConfig, true);
  ads::CDockManager::setConfigFlag(ads::CDockManager::AlwaysShowTabs, true);
  ads::CDockManager::setConfigFlag(
      ads::CDockManager::RetainTabSizeWhenCloseButtonHidden, true);

  dockManager->setStyleSheet(
      "ads--CDockWidgetTab { padding-left: 10px; padding-right: 10px; }");
}

ads::CDockWidget *makeDock(ads::CDockManager *manager, const QString &title,
                           QWidget *content, QWidget *parent) {
  auto *dock = new ads::CDockWidget(manager, title, parent);
  dock->setWidget(content, ads::CDockWidget::ForceNoScrollArea);
  dock->setFeature(ads::CDockWidget::DockWidgetFocusable, true);
  return dock;
}

void saveLayout(ads::CDockManager *dockManager) {
  if (!dockManager) {
    return;
  }

  QSettings settings;
  settings.beginGroup(kSettingsGroup);
  settings.setValue(kLayoutKey, dockManager->saveState(kLayoutVersion));
  settings.endGroup();
}

bool restoreLayout(ads::CDockManager *dockManager) {
  if (!dockManager) {
    return false;
  }

  QSettings settings;
  settings.beginGroup(kSettingsGroup);
  const QByteArray state = settings.value(kLayoutKey).toByteArray();
  settings.endGroup();
  return !state.isEmpty() && dockManager->restoreState(state, kLayoutVersion);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), impl_(new Impl()) {
  setWindowTitle("stock-tool");

  auto *fileMenu = menuBar()->addMenu("&File");
  fileMenu->addAction("Save Layout", [this]() {
    saveLayout(impl_->dockManager);
    statusBar()->showMessage("Layout saved", 2000);
  });
  fileMenu->addSeparator();
  fileMenu->addAction("Exit", this, &QWidget::close);

  auto *viewMenu = menuBar()->addMenu("&View");
  viewMenu->addAction("Restore Layout", [this]() {
    const bool restored = restoreLayout(impl_->dockManager);
    statusBar()->showMessage(restored ? "Layout restored"
                                      : "No saved layout found",
                              3000);
  });
  viewMenu->addAction("Reset Layout", [this]() {
    statusBar()->showMessage(
        "Reset layout is not wired yet. Use Restore Layout after saving a "
        "preferred workspace.",
        3000);
  });

  auto *mainToolBar = addToolBar("Main");
  mainToolBar->addAction("Refresh", [this]() {
    const QString label =
        impl_->currentSymbol.isEmpty() ? "workspace" : impl_->currentSymbol;
    statusBar()->showMessage(QString("Refresh requested for %1").arg(label),
                             3000);
  });
  mainToolBar->addAction("Save Layout", [this]() {
    saveLayout(impl_->dockManager);
    statusBar()->showMessage("Layout saved", 2000);
  });

  impl_->dockManager = new ads::CDockManager(this);
  configureDockManager(impl_->dockManager);
  setCentralWidget(impl_->dockManager);

  auto *overviewDock = makeDock(impl_->dockManager, "Overview",
                                createOverviewWidget(&impl_->overviewSymbolLabel,
                                                     &impl_->overviewNotes,
                                                     this),
                                impl_->dockManager);
  impl_->dockManager->setCentralWidget(overviewDock);

  impl_->searchPane = new StockTool::Dock::SymbolSearchPane(this);
  auto *searchDock = makeDock(impl_->dockManager, "Symbol Search",
                              impl_->searchPane, impl_->dockManager);
  auto *leftArea =
      impl_->dockManager->addDockWidget(ads::LeftDockWidgetArea, searchDock);

  impl_->watchListPane = new StockTool::Dock::WatchListPane(this);
  auto *watchListDock = makeDock(impl_->dockManager, "Watch List",
                                 impl_->watchListPane, impl_->dockManager);
  impl_->dockManager->addDockWidgetTabToArea(watchListDock, leftArea);

  impl_->chartPane = new StockTool::Dock::ChartPane(this);
  auto *chartDock = makeDock(impl_->dockManager, "Chart", impl_->chartPane,
                             impl_->dockManager);
  auto *rightArea =
      impl_->dockManager->addDockWidget(ads::RightDockWidgetArea, chartDock);

  impl_->analysisPane = new StockTool::Dock::AnalysisPane(this);
  auto *analysisDock = makeDock(impl_->dockManager, "Analysis",
                                impl_->analysisPane, impl_->dockManager);
  impl_->dockManager->addDockWidgetTabToArea(analysisDock, rightArea);

  const auto openSymbol = [this](const QString &rawSymbol) {
    const QString symbol = normalizedSymbol(rawSymbol);
    if (symbol.isEmpty()) {
      return;
    }

    impl_->currentSymbol = symbol;
    impl_->chartPane->setSymbol(symbol);
    impl_->analysisPane->setSymbol(symbol);

    if (impl_->overviewSymbolLabel) {
      impl_->overviewSymbolLabel->setText(
          QString("Selected Symbol: %1").arg(symbol));
    }
    if (impl_->overviewNotes) {
      impl_->overviewNotes->setPlainText(
          QString("%1 workspace focus\n\n"
                  "- Update chart data source\n"
                  "- Show price summary and range\n"
                  "- Add real indicators to Analysis pane")
              .arg(symbol));
    }

    statusBar()->showMessage(QString("Loaded %1").arg(symbol), 2500);
  };

  impl_->searchPane->setSubmitHandler(openSymbol);
  impl_->searchPane->setAddWatchHandler([this](const QString &rawSymbol) {
    const QString symbol = normalizedSymbol(rawSymbol);
    if (symbol.isEmpty()) {
      return;
    }

    impl_->watchListPane->addSymbol(symbol);
    statusBar()->showMessage(QString("%1 added to watch list").arg(symbol),
                             2500);
  });
  impl_->watchListPane->setOpenSymbolHandler(openSymbol);
  impl_->analysisPane->setResultHandler(
      [this](const QString &contextLabel,
             const StockTool::Domain::FactorModelResult &result) {
        if (!impl_->chartPane) {
          return;
        }
        if (result.ok) {
          impl_->chartPane->setFactorModelResult(contextLabel, result);
        } else {
          impl_->chartPane->clearModelResult();
        }
      });

  QTimer::singleShot(0, this, [this]() {
    if (!restoreLayout(impl_->dockManager)) {
      statusBar()->showMessage("Workspace ready", 2000);
    }
  });
}

MainWindow::~MainWindow() {
  delete impl_;
}

void MainWindow::closeEvent(QCloseEvent *event) {
  saveLayout(impl_->dockManager);
  QMainWindow::closeEvent(event);
}

} // namespace StockTool::App
