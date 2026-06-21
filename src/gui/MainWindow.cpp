#include "MainWindow.h"
#include "ImageCanvas.h"
#include "WapPanel.h"
#include "FilterPanel.h"
#include "BatchDialog.h"
#include "core/IoModule.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QAction>
#include <QActionGroup>
#include <QStyle>
#include <QStyleFactory>
#include <QSplitter>
#include <QScrollArea>
#include <QProgressDialog>
#include <QTimer>
#include <QInputDialog>

namespace PixelForge {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenus();
    setupToolbar();
    setupStatusBar();
    setupConnections();
    setupDarkTheme();
    updateTitle();

    setAcceptDrops(true);
    resize(1400, 900);
}

MainWindow::~MainWindow() = default;

// ============================================================
// UI Setup
// ============================================================

void MainWindow::setupUi() {
    // Central canvas
    canvas_ = new ImageCanvas;
    setCentralWidget(canvas_);

    // Left panel with tabs (WPAP / Filters)
    leftPanel_ = new QTabWidget;
    leftPanel_->setMinimumWidth(280);
    leftPanel_->setMaximumWidth(360);

    wapPanel_ = new WapPanel;
    filterPanel_ = new FilterPanel;

    leftPanel_->addTab(filterPanel_, "Filters");
    leftPanel_->addTab(wapPanel_, "WPAP");

    // Populate filter presets
    auto presetIds = gradingModule_.availablePresets();
    // Build a map for the panel
    std::unordered_map<std::string, PresetInfo> presetMap;
    for (const auto& id : presetIds) {
        const auto* info = gradingModule_.getPresetInfo(id);
        if (info) presetMap[id] = *info;
    }
    filterPanel_->setAvailablePresets(presetIds, presetMap);

    // Right dock for left panel
    auto* leftDock = new QDockWidget("Tools", this);
    leftDock->setWidget(leftPanel_);
    leftDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, leftDock);
}

void MainWindow::setupMenus() {
    // ---- File Menu ----
    auto* fileMenu = menuBar()->addMenu("&File");

    auto* openAction = fileMenu->addAction("&Open Image...", this, &MainWindow::onOpenFile,
                                            QKeySequence("Ctrl+O"));
    openAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));

    fileMenu->addSeparator();

    auto* saveProjectAction = fileMenu->addAction("Save &Project...", this,
                                                    &MainWindow::onSaveProject,
                                                    QKeySequence("Ctrl+Shift+S"));

    auto* openProjectAction = fileMenu->addAction("Open Pro&ject...", this,
                                                    &MainWindow::onOpenProject);

    fileMenu->addSeparator();

    auto* exportAction = fileMenu->addAction("&Export Image...", this,
                                               &MainWindow::onExportImage,
                                               QKeySequence("Ctrl+Shift+E"));

    auto* exportSvgAction = fileMenu->addAction("Export &SVG (WPAP)...", this,
                                                  &MainWindow::onExportSVG);

    fileMenu->addSeparator();

    fileMenu->addAction("&Batch Process...", this, &MainWindow::onBatchProcess,
                         QKeySequence("Ctrl+B"));

    fileMenu->addSeparator();

    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence("Ctrl+Q"));

    // ---- Edit Menu ----
    auto* editMenu = menuBar()->addMenu("&Edit");

    auto* undoAction = editMenu->addAction("&Undo", this, &MainWindow::onUndo,
                                            QKeySequence("Ctrl+Z"));
    auto* redoAction = editMenu->addAction("&Redo", this, &MainWindow::onRedo,
                                            QKeySequence("Ctrl+Shift+Z"));

    // ---- View Menu ----
    auto* viewMenu = menuBar()->addMenu("&View");

    viewMenu->addAction("Zoom &In", this, &MainWindow::onZoomIn, QKeySequence("Ctrl+="));
    viewMenu->addAction("Zoom &Out", this, &MainWindow::onZoomOut, QKeySequence("Ctrl+-"));
    viewMenu->addAction("&Fit to Window", this, &MainWindow::onZoomFit, QKeySequence("Ctrl+0"));
    viewMenu->addAction("&Actual Size", this, &MainWindow::onZoomActual, QKeySequence("Ctrl+1"));

    viewMenu->addSeparator();

    viewMenu->addAction("Toggle &Comparison", this, &MainWindow::onToggleComparison,
                         QKeySequence("Ctrl+C"));

    viewMenu->addSeparator();

    viewMenu->addAction("Toggle &Dark/Light Theme", this, &MainWindow::onThemeToggle);

    // ---- Help Menu ----
    auto* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About PixelForge", this, [this]() {
        QMessageBox::about(this, "About PixelForge",
            "<h2>PixelForge v1.0</h2>"
            "<p>Desktop Image Style & Filter Editor</p>"
            "<p>Features WPAP generation, cinematic filters, "
            "Japan-style presets, and batch processing.</p>"
            "<p>Built with C++20, Qt 6, and OpenCV.</p>");
    });
}

void MainWindow::setupToolbar() {
    auto* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(24, 24));

    toolbar->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton),
                        "Open", this, &MainWindow::onOpenFile);

    toolbar->addSeparator();

    toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowUndo),
                        "Undo", this, &MainWindow::onUndo);

    toolbar->addAction(style()->standardIcon(QStyle::SP_ArrowRedo),
                        "Redo", this, &MainWindow::onRedo);

    toolbar->addSeparator();

    toolbar->addAction("Zoom In", this, &MainWindow::onZoomIn);
    toolbar->addAction("Zoom Out", this, &MainWindow::onZoomOut);
    toolbar->addAction("Fit", this, &MainWindow::onZoomFit);

    toolbar->addSeparator();

    toolbar->addAction("Compare", this, &MainWindow::onToggleComparison);
}

void MainWindow::setupStatusBar() {
    statusLabel_ = new QLabel("Ready");
    zoomLabel_ = new QLabel("100%");
    sizeLabel_ = new QLabel("");

    statusBar()->addWidget(statusLabel_, 1);
    statusBar()->addPermanentWidget(sizeLabel_);
    statusBar()->addPermanentWidget(zoomLabel_);

    connect(canvas_, &ImageCanvas::zoomChanged, this, [this](float zoom) {
        zoomLabel_->setText(QString("%1%").arg(static_cast<int>(zoom * 100)));
    });
}

void MainWindow::setupConnections() {
    // Tab switching
    connect(leftPanel_, &QTabWidget::currentChanged, this, [this](int index) {
        currentMode_ = (index == 0) ? EditMode::Filter : EditMode::Wap;
    });

    // Filter panel connections
    connect(filterPanel_, &FilterPanel::presetSelected, this, &MainWindow::onApplyFilter);
    connect(filterPanel_, &FilterPanel::parametersChanged, this, &MainWindow::onFilterParamsChanged);
    connect(filterPanel_, &FilterPanel::saveAsPresetRequested, this, [this]() {
        bool ok;
        QString name = QInputDialog::getText(this, "Save Preset", "Preset name:",
                                               QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            std::string presetId = filterPanel_->selectedPresetId();
            FilterParameters params = filterPanel_->currentParams();
            gradingModule_.saveCustomPreset(name.toStdString(), presetId, params);

            // Refresh preset list
            auto presetIds = gradingModule_.availablePresets();
            std::unordered_map<std::string, PresetInfo> presetMap;
            for (const auto& id : presetIds) {
                const auto* info = gradingModule_.getPresetInfo(id);
                if (info) presetMap[id] = *info;
            }
            filterPanel_->setAvailablePresets(presetIds, presetMap);
        }
    });

    connect(filterPanel_, &FilterPanel::importLutRequested, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "Import LUT File", "",
                                                      "Cube LUT Files (*.cube);;All Files (*)");
        if (!file.isEmpty() && project_.hasImage()) {
            FilterParameters params = filterPanel_->currentParams();
            Image result = gradingModule_.applyCustomLut(
                project_.sourceImage(), file.toStdString(), params);
            project_.setCurrentImage(result, "Custom LUT");
            canvas_->setImage(result);
            canvas_->setComparisonImage(project_.sourceImage());
        }
    });

    // WPAP panel connections
    connect(wapPanel_, &WapPanel::generateRequested, this, &MainWindow::onGenerateWap);
    connect(wapPanel_, &WapPanel::exportRequested, this, &MainWindow::onExportSVG);
}

void MainWindow::setupDarkTheme() {
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));

    qApp->setPalette(darkPalette);
}

void MainWindow::setupLightTheme() {
    qApp->setStyle(QStyleFactory::create("Fusion"));
    qApp->setPalette(QApplication::style()->standardPalette());
}

// ============================================================
// File Operations
// ============================================================

void MainWindow::onOpenFile() {
    QString filePath = QFileDialog::getOpenFileName(
        this, "Open Image", "",
        IoModule::supportedImportFilter().c_str());

    if (!filePath.isEmpty()) {
        loadImageFromFile(filePath.toStdString());
    }
}

void MainWindow::loadImageFromFile(const std::string& path) {
    try {
        if (project_.openImage(path)) {
            canvas_->setImage(project_.currentImage());
            canvas_->setComparisonImage(project_.sourceImage());
            canvas_->setComparisonEnabled(false);

            // Update size label
            const auto& img = project_.currentImage();
            sizeLabel_->setText(QString("%1 × %2").arg(img.width()).arg(img.height()));

            updateTitle();
            statusLabel_->setText("Image loaded: " + QString::fromStdString(path));
        } else {
            QMessageBox::warning(this, "Error", "Failed to load image.");
        }
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Error",
                             QString("Failed to load image:\n%1").arg(e.what()));
    }
}

void MainWindow::onSaveProject() {
    if (!project_.hasImage()) return;

    QString filePath = QFileDialog::getSaveFileName(
        this, "Save Project", "",
        "PixelForge Project (*.pforge);;All Files (*)");

    if (!filePath.isEmpty()) {
        if (project_.saveProject(filePath.toStdString())) {
            statusLabel_->setText("Project saved");
            updateTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save project.");
        }
    }
}

void MainWindow::onOpenProject() {
    QString filePath = QFileDialog::getOpenFileName(
        this, "Open Project", "",
        "PixelForge Project (*.pforge);;All Files (*)");

    if (!filePath.isEmpty()) {
        if (project_.loadProject(filePath.toStdString())) {
            canvas_->setImage(project_.currentImage());
            canvas_->setComparisonImage(project_.sourceImage());
            updateTitle();
            statusLabel_->setText("Project loaded");
        } else {
            QMessageBox::warning(this, "Error", "Failed to load project.");
        }
    }
}

void MainWindow::onExportImage() {
    if (!project_.hasImage()) return;

    QString suggested = QString::fromStdString(
        project_.suggestedExportName(filterPanel_->selectedPresetId()));

    QString filePath = QFileDialog::getSaveFileName(
        this, "Export Image", suggested,
        IoModule::supportedExportFilter().c_str());

    if (!filePath.isEmpty()) {
        if (project_.exportImage(filePath.toStdString())) {
            statusLabel_->setText("Exported: " + filePath);
        } else {
            QMessageBox::warning(this, "Error", "Failed to export image.");
        }
    }
}

void MainWindow::onExportSVG() {
    if (!project_.hasImage()) return;

    // Export WPAP as SVG
    const auto& triangles = wapModule_.triangles();
    if (triangles.empty()) {
        QMessageBox::information(this, "No WPAP",
                                  "Generate a WPAP first before exporting SVG.");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "Export SVG", "",
        "SVG Files (*.svg);;All Files (*)");

    if (!filePath.isEmpty()) {
        const auto& img = project_.currentImage();
        if (wapModule_.exportSVG(filePath.toStdString(), img.width(), img.height())) {
            statusLabel_->setText("SVG exported: " + filePath);
        } else {
            QMessageBox::warning(this, "Error", "Failed to export SVG.");
        }
    }
}

void MainWindow::onBatchProcess() {
    auto presetIds = gradingModule_.availablePresets();
    std::unordered_map<std::string, PresetInfo> presetMap;
    for (const auto& id : presetIds) {
        const auto* info = gradingModule_.getPresetInfo(id);
        if (info) presetMap[id] = *info;
    }

    BatchDialog dialog(this);
    dialog.setAvailablePresets(presetIds, presetMap);
    dialog.exec();
}

// ============================================================
// Edit Operations
// ============================================================

void MainWindow::onUndo() {
    if (project_.history().canUndo()) {
        Image img = project_.history().undo();
        if (!img.isEmpty()) {
            canvas_->setImage(img);
            statusLabel_->setText("Undo: " +
                QString::fromStdString(project_.history().currentDescription()));
        }
    }
}

void MainWindow::onRedo() {
    if (project_.history().canRedo()) {
        Image img = project_.history().redo();
        if (!img.isEmpty()) {
            canvas_->setImage(img);
            statusLabel_->setText("Redo: " +
                QString::fromStdString(project_.history().currentDescription()));
        }
    }
}

// ============================================================
// View Operations
// ============================================================

void MainWindow::onZoomIn() {
    canvas_->zoomIn();
}

void MainWindow::onZoomOut() {
    canvas_->zoomOut();
}

void MainWindow::onZoomFit() {
    canvas_->zoomFit();
}

void MainWindow::onZoomActual() {
    canvas_->zoomActual();
}

void MainWindow::onToggleComparison() {
    bool enabled = !canvas_->isComparisonEnabled();
    canvas_->setComparisonEnabled(enabled);
    statusLabel_->setText(enabled ? "Comparison mode ON" : "Comparison mode OFF");
}

void MainWindow::onThemeToggle() {
    static bool isDark = true;
    if (isDark) {
        setupLightTheme();
    } else {
        setupDarkTheme();
    }
    isDark = !isDark;
}

// ============================================================
// Processing
// ============================================================

void MainWindow::onGenerateWap() {
    if (!project_.hasImage()) {
        QMessageBox::information(this, "No Image", "Please open an image first.");
        return;
    }

    statusLabel_->setText("Generating WPAP...");
    QApplication::processEvents();

    WapParameters params = wapPanel_->currentParams();

    Image result = wapModule_.generate(project_.sourceImage(), params,
        [this](float progress, const std::string& msg) {
            statusLabel_->setText(QString::fromStdString(msg) + " (" +
                                  QString::number(static_cast<int>(progress * 100)) + "%)");
            QApplication::processEvents();
        });

    project_.setCurrentImage(result, "WPAP Generation");
    canvas_->setImage(result);
    canvas_->setComparisonImage(project_.sourceImage());
    canvas_->setComparisonEnabled(true);

    statusLabel_->setText("WPAP generated successfully");
}

void MainWindow::onApplyFilter(const std::string& presetId) {
    if (!project_.hasImage()) return;

    currentMode_ = EditMode::Filter;
    updatePreview();
}

void MainWindow::onFilterParamsChanged() {
    if (!project_.hasImage() || currentMode_ != EditMode::Filter) return;
    updatePreview();
}

void MainWindow::updatePreview() {
    if (!project_.hasImage()) return;

    std::string presetId = filterPanel_->selectedPresetId();
    if (presetId.empty()) return;

    FilterParameters params = filterPanel_->currentParams();

    Image result = gradingModule_.applyPreset(project_.sourceImage(), presetId, params);

    canvas_->setImage(result);
    canvas_->setComparisonImage(project_.sourceImage());

    statusLabel_->setText("Filter applied: " + QString::fromStdString(presetId));
}

// ============================================================
// Window Events
// ============================================================

void MainWindow::updateTitle() {
    QString title = "PixelForge";
    if (project_.hasImage()) {
        title += " — " + QString::fromStdString(
            std::filesystem::path(project_.sourcePath()).filename().string());
        if (project_.hasUnsavedChanges()) title += " *";
    }
    setWindowTitle(title);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (project_.hasUnsavedChanges()) {
        auto reply = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved changes. Do you want to save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save) {
            onSaveProject();
        } else if (reply == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const auto& url : event->mimeData()->urls()) {
            std::string path = url.toLocalFile().toStdString();
            if (IoModule::isSupportedFormat(path) ||
                path.find(".pforge") != std::string::npos) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    for (const auto& url : event->mimeData()->urls()) {
        std::string path = url.toLocalFile().toStdString();
        if (path.find(".pforge") != std::string::npos) {
            if (project_.loadProject(path)) {
                canvas_->setImage(project_.currentImage());
                canvas_->setComparisonImage(project_.sourceImage());
                updateTitle();
            }
        } else if (IoModule::isSupportedFormat(path)) {
            loadImageFromFile(path);
        }
        break; // Process first valid file only
    }
}

} // namespace PixelForge