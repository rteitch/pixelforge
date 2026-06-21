#include "MainWindow.h"
#include "ImageCanvas.h"
#include "WapPanel.h"
#include "FilterPanel.h"
#include "BatchDialog.h"
#include "CropRotateDialog.h"
#include "ComparisonGrid.h"
#include "core/IoModule.h"
#include "core/PluginManager.h"
#include "core/AiStyleModule.h"

#include <filesystem>
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

    // Plugin management
    fileMenu->addAction("Load &Plugin...", this, &MainWindow::onLoadPlugin);
    fileMenu->addAction("Load Plugin &Directory...", this, &MainWindow::onLoadPluginDirectory);

    fileMenu->addSeparator();

    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence("Ctrl+Q"));

    // ---- Edit Menu ----
    auto* editMenu = menuBar()->addMenu("&Edit");

    auto* undoAction = editMenu->addAction("&Undo", this, &MainWindow::onUndo,
                                            QKeySequence("Ctrl+Z"));
    auto* redoAction = editMenu->addAction("&Redo", this, &MainWindow::onRedo,
                                            QKeySequence("Ctrl+Shift+Z"));

    editMenu->addSeparator();

    editMenu->addAction("Crop && &Rotate...", this, &MainWindow::onCropRotate,
                         QKeySequence("Ctrl+R"));

    editMenu->addAction("Toggle &Favorite Preset", this, &MainWindow::onToggleFavorite,
                         QKeySequence("Ctrl+D"));

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

    viewMenu->addSeparator();

    viewMenu->addAction("Comparison &Grid...", this, &MainWindow::onComparisonGrid,
                          QKeySequence("Ctrl+G"));

    // ---- AI Menu ----
    auto* aiMenu = menuBar()->addMenu("&AI Style");
    aiMenu->addAction("Apply &AI Style Transfer...", this, &MainWindow::onApplyAiStyle,
                       QKeySequence("Ctrl+I"));

    auto* aiStatusAction = aiMenu->addAction(
        AiStyleModule::isOnnxRuntimeAvailable()
            ? "ONNX Runtime: Available ✓"
            : "ONNX Runtime: Not Available ✗");
    aiStatusAction->setEnabled(false);

    aiMenu->addSeparator();
    auto models = AiStyleModule::availableModels();
    for (const auto& m : models) {
        aiMenu->addAction(QString::fromStdString(m.name + " — " + m.description));
    }

    // ---- Plugin Menu ----
    auto* pluginMenu = menuBar()->addMenu("&Plugins");
    pluginMenu->addAction("Load Plugin...", this, &MainWindow::onLoadPlugin);
    pluginMenu->addAction("Load Plugin Directory...", this, &MainWindow::onLoadPluginDirectory);

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

void MainWindow::onApplyFilter(const std::string& /*presetId*/) {
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

// ============================================================
// v1.1 Features
// ============================================================

void MainWindow::onCropRotate() {
    if (!project_.hasImage()) {
        QMessageBox::information(this, "No Image", "Please open an image first.");
        return;
    }

    CropRotateDialog dialog(project_.currentImage(), this);
    if (dialog.exec() == QDialog::Accepted) {
        Image result = dialog.result();
        if (!result.isEmpty()) {
            project_.setCurrentImage(result, "Crop & Rotate");
            canvas_->setImage(result);
            canvas_->setComparisonImage(project_.sourceImage());
            sizeLabel_->setText(QString("%1 × %2").arg(result.width()).arg(result.height()));
            statusLabel_->setText("Crop & Rotate applied");
        }
    }
}

void MainWindow::onComparisonGrid() {
    if (!project_.hasImage()) {
        QMessageBox::information(this, "No Image", "Please open an image first.");
        return;
    }

    ComparisonGrid dialog(project_.sourceImage(), gradingModule_, this);
    auto presetIds = gradingModule_.availablePresets();
    dialog.setPresetIds(presetIds);

    if (dialog.exec() == QDialog::Accepted) {
        std::string chosen = dialog.selectedPresetId();
        if (!chosen.empty()) {
            filterPanel_->setSelectedPreset(chosen);
            updatePreview();
            statusLabel_->setText("Applied preset from grid: " +
                                  QString::fromStdString(chosen));
        }
    }
}

void MainWindow::onToggleFavorite() {
    std::string presetId = filterPanel_->selectedPresetId();
    if (presetId.empty()) return;

    auto* info = gradingModule_.getPresetInfo(presetId);
    if (info) {
        // Toggle favorite status
        PresetInfo modified = *info;
        modified.isFavorite = !modified.isFavorite;
        gradingModule_.registerPreset(modified);

        // Refresh preset list
        auto ids = gradingModule_.availablePresets();
        std::unordered_map<std::string, PresetInfo> presetMap;
        for (const auto& id : ids) {
            const auto* i = gradingModule_.getPresetInfo(id);
            if (i) presetMap[id] = *i;
        }
        filterPanel_->setAvailablePresets(ids, presetMap);
        filterPanel_->setSelectedPreset(presetId);

        statusLabel_->setText(modified.isFavorite
            ? "Added to favorites: " + QString::fromStdString(info->name)
            : "Removed from favorites: " + QString::fromStdString(info->name));
    }
}

// ============================================================
// v1.3 — Plugin System
// ============================================================

void MainWindow::onLoadPlugin() {
    QString filePath = QFileDialog::getOpenFileName(
        this, "Load Plugin", "",
#ifdef _WIN32
        "Plugin Libraries (*.dll);;All Files (*)"
#elif __APPLE__
        "Plugin Libraries (*.dylib);;All Files (*)"
#else
        "Plugin Libraries (*.so);;All Files (*)"
#endif
    );

    if (filePath.isEmpty()) return;

    if (pluginManager_.loadPlugin(filePath.toStdString())) {
        auto info = pluginManager_.allPluginInfo();
        if (!info.empty()) {
            const auto& last = info.back();
            statusLabel_->setText(
                QString("Plugin loaded: %1 v%2")
                    .arg(QString::fromStdString(last.name))
                    .arg(QString::fromStdString(last.version)));
        }
    } else {
        QMessageBox::warning(this, "Plugin Error",
                             "Failed to load plugin. Check that the file is a valid "
                             "PixelForge plugin library.");
    }
}

void MainWindow::onLoadPluginDirectory() {
    QString dirPath = QFileDialog::getExistingDirectory(
        this, "Select Plugin Directory");

    if (dirPath.isEmpty()) return;

    int count = pluginManager_.loadPluginsFromDirectory(dirPath.toStdString());
    statusLabel_->setText(
        QString("Loaded %1 plugin(s) from: %2").arg(count).arg(dirPath));

    if (count == 0) {
        QMessageBox::information(this, "No Plugins Found",
                                 "No valid plugin libraries found in the selected directory.");
    }
}

// ============================================================
// v2.0 — AI Style Transfer
// ============================================================

void MainWindow::onApplyAiStyle() {
    if (!project_.hasImage()) {
        QMessageBox::information(this, "No Image", "Please open an image first.");
        return;
    }

    if (!AiStyleModule::isOnnxRuntimeAvailable()) {
        QMessageBox::information(this, "AI Style Transfer",
            "<h3>ONNX Runtime Not Available</h3>"
            "<p>AI Style Transfer requires the ONNX Runtime library.</p>"
            "<p>To enable this feature:</p>"
            "<ol>"
            "<li>Install ONNX Runtime: <code>vcpkg install onnxruntime</code></li>"
            "<li>Rebuild with: <code>cmake -DPIXELFORGE_HAS_ONNX=ON ..</code></li>"
            "<li>Download style transfer models (.onnx files) to the <code>models/</code> directory</li>"
            "</ol>"
            "<p>Recommended models: Mosaic, Candy, Starry Night, The Scream, Udnie</p>");
        return;
    }

    // Check if any models are loaded
    auto modelIds = aiStyleModule_.loadedModelIds();
    if (modelIds.empty()) {
        // Try to find models in the models/ directory
        QString modelPath = QFileDialog::getOpenFileName(
            this, "Load ONNX Style Model", "models/",
            "ONNX Models (*.onnx);;All Files (*)");

        if (modelPath.isEmpty()) return;

        // Load the model
        std::string modelId = std::filesystem::path(modelPath.toStdString())
                                  .stem().string();
        if (!aiStyleModule_.loadModel(modelPath.toStdString(), modelId)) {
            QMessageBox::warning(this, "Model Error",
                                 "Failed to load the ONNX model. "
                                 "Ensure ONNX Runtime is properly installed.");
            return;
        }
        modelIds = aiStyleModule_.loadedModelIds();
    }

    // Choose model if multiple
    std::string chosenModel = modelIds[0];
    if (modelIds.size() > 1) {
        QStringList items;
        for (const auto& id : modelIds) {
            items << QString::fromStdString(id);
        }
        bool ok;
        QString selected = QInputDialog::getItem(
            this, "Select AI Model", "Style model:", items, 0, false, &ok);
        if (!ok) return;
        chosenModel = selected.toStdString();
    }

    // Get strength
    bool ok;
    double strength = QInputDialog::getDouble(
        this, "AI Style Strength",
        "Blend strength (0.0 = original, 1.0 = full style):",
        1.0, 0.0, 1.0, 2, &ok);
    if (!ok) return;

    statusLabel_->setText("Applying AI style transfer...");
    QApplication::processEvents();

    Image result = aiStyleModule_.applyStyle(
        project_.sourceImage(), chosenModel, static_cast<float>(strength),
        [this](float progress, const std::string& msg) {
            statusLabel_->setText(
                QString::fromStdString(msg) + " (" +
                QString::number(static_cast<int>(progress * 100)) + "%)");
            QApplication::processEvents();
        });

    project_.setCurrentImage(result, "AI Style: " + chosenModel);
    canvas_->setImage(result);
    canvas_->setComparisonImage(project_.sourceImage());
    canvas_->setComparisonEnabled(true);

    statusLabel_->setText("AI style transfer complete: " +
                          QString::fromStdString(chosenModel));
}

} // namespace PixelForge
