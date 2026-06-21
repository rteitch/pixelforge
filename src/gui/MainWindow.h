#pragma once

#include "core/CoreTypes.h"
#include "core/WapModule.h"
#include "core/ColorGradingModule.h"
#include "app/ProjectManager.h"

#include <QMainWindow>
#include <QTabWidget>
#include <QDockWidget>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QLabel>
#include <QTimer>

namespace PixelForge {

class ImageCanvas;
class WapPanel;
class FilterPanel;
class BatchDialog;

/// Main application window with single-window workspace layout.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    // File menu
    void onOpenFile();
    void onSaveProject();
    void onOpenProject();
    void onExportImage();
    void onExportSVG();
    void onBatchProcess();

    // Edit menu
    void onUndo();
    void onRedo();

    // View menu
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onZoomActual();
    void onToggleComparison();

    // Processing
    void onGenerateWap();
    void onApplyFilter(const std::string& presetId);
    void onFilterParamsChanged();
    void onThemeToggle();

private:
    // Core modules
    ProjectManager project_;
    WapModule wapModule_;
    ColorGradingModule gradingModule_;

    // GUI components
    ImageCanvas* canvas_ = nullptr;
    QTabWidget* leftPanel_ = nullptr;
    WapPanel* wapPanel_ = nullptr;
    FilterPanel* filterPanel_ = nullptr;
    QDockWidget* rightDock_ = nullptr;

    // Status bar
    QLabel* statusLabel_ = nullptr;
    QLabel* zoomLabel_ = nullptr;
    QLabel* sizeLabel_ = nullptr;

    // Current mode
    enum class EditMode { Filter, Wap };
    EditMode currentMode_ = EditMode::Filter;

    void setupUi();
    void setupMenus();
    void setupToolbar();
    void setupStatusBar();
    void setupConnections();
    void setupDarkTheme();
    void setupLightTheme();
    void updateTitle();
    void updatePreview();
    void loadImageFromFile(const std::string& path);
};

} // namespace PixelForge