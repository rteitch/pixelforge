#pragma once

#include "core/CoreTypes.h"

#include <QDialog>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QThread>

namespace PixelForge {

class BatchProcessor;

/// Dialog for configuring and monitoring batch processing jobs.
class BatchDialog : public QDialog {
    Q_OBJECT

public:
    explicit BatchDialog(QWidget* parent = nullptr);
    ~BatchDialog() override;

    /// Set available presets for batch processing
    void setAvailablePresets(const std::vector<std::string>& presetIds,
                             const std::unordered_map<std::string, PresetInfo>& presets);

    /// Set files to process
    void setFiles(const std::vector<std::string>& filePaths);

private slots:
    void onSelectFiles();
    void onSelectOutputDir();
    void onStart();
    void onCancel();

private:
    // Config UI
    QTableWidget* fileList_ = nullptr;
    QLineEdit* outputDirEdit_ = nullptr;
    QComboBox* presetCombo_ = nullptr;
    QComboBox* formatCombo_ = nullptr;
    QLineEdit* namingPatternEdit_ = nullptr;
    QSlider* qualitySlider_ = nullptr;
    QLabel* qualityLabel_ = nullptr;

    // Progress UI
    QProgressBar* overallProgress_ = nullptr;
    QProgressBar* fileProgress_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* currentFileLabel_ = nullptr;

    // Buttons
    QPushButton* startBtn_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;
    QPushButton* closeBtn_ = nullptr;

    // State
    std::vector<std::string> files_;
    std::vector<std::string> presetIds_;
    std::unordered_map<std::string, PresetInfo> presets_;
    BatchProcessor* processor_ = nullptr;
    bool running_ = false;

    void setupUi();
    void connectSignals();
    void updateFileList();
    void setRunning(bool running);
};

} // namespace PixelForge