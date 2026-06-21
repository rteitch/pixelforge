#pragma once

#include "core/CoreTypes.h"

#include <QWidget>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>

namespace PixelForge {

/// Right panel for filter selection and parameter adjustment.
class FilterPanel : public QWidget {
    Q_OBJECT

public:
    explicit FilterPanel(QWidget* parent = nullptr);
    ~FilterPanel() override;

    /// Populate preset list from grading module
    void setAvailablePresets(const std::vector<std::string>& presetIds,
                             const std::unordered_map<std::string, PresetInfo>& presets);

    /// Get current filter parameters
    FilterParameters currentParams() const;
    void setParams(const FilterParameters& params);

    /// Get selected preset ID
    std::string selectedPresetId() const;
    void setSelectedPreset(const std::string& presetId);

signals:
    void presetSelected(const std::string& presetId);
    void parametersChanged();
    void saveAsPresetRequested();
    void importLutRequested();

private:
    QListWidget* presetList_ = nullptr;
    QLineEdit* searchBox_ = nullptr;

    QSlider* intensitySlider_ = nullptr;
    QLabel* intensityLabel_ = nullptr;

    QSlider* grainSlider_ = nullptr;
    QLabel* grainLabel_ = nullptr;

    QSlider* vignetteSlider_ = nullptr;
    QLabel* vignetteLabel_ = nullptr;

    QSlider* temperatureSlider_ = nullptr;
    QLabel* temperatureLabel_ = nullptr;

    QSlider* tintSlider_ = nullptr;
    QLabel* tintLabel_ = nullptr;

    QSlider* contrastSlider_ = nullptr;
    QLabel* contrastLabel_ = nullptr;

    QSlider* brightnessSlider_ = nullptr;
    QLabel* brightnessLabel_ = nullptr;

    QSlider* saturationSlider_ = nullptr;
    QLabel* saturationLabel_ = nullptr;

    QSlider* highlightsSlider_ = nullptr;
    QLabel* highlightsLabel_ = nullptr;

    QSlider* shadowsSlider_ = nullptr;
    QLabel* shadowsLabel_ = nullptr;

    QPushButton* savePresetBtn_ = nullptr;
    QPushButton* importLutBtn_ = nullptr;

    void setupUi();
    void connectSignals();

    /// Helper to create a slider row
    QHBoxLayout* createSliderRow(QSlider*& slider, QLabel*& label,
                                  const QString& labelText,
                                  int min, int max, int defaultVal,
                                  const QString& suffix = "");
};

} // namespace PixelForge