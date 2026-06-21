#include "FilterPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>

namespace PixelForge {

FilterPanel::FilterPanel(QWidget* parent) : QWidget(parent) {
    setupUi();
    connectSignals();
}

FilterPanel::~FilterPanel() = default;

QHBoxLayout* FilterPanel::createSliderRow(
    QSlider*& slider, QLabel*& label,
    const QString& labelText, int min, int max, int defaultVal,
    const QString& suffix)
{
    slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(defaultVal);
    label = new QLabel(QString::number(defaultVal) + suffix);
    label->setMinimumWidth(45);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* layout = new QHBoxLayout;
    layout->addWidget(new QLabel(labelText), 1);
    layout->addWidget(slider, 3);
    layout->addWidget(label, 1);
    return layout;
}

void FilterPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // Scroll area for the entire panel
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* scrollWidget = new QWidget;
    auto* scrollLayout = new QVBoxLayout(scrollWidget);

    // Preset selection group
    auto* presetGroup = new QGroupBox("Presets");
    auto* presetLayout = new QVBoxLayout(presetGroup);

    searchBox_ = new QLineEdit;
    searchBox_->setPlaceholderText("Search presets...");
    searchBox_->setClearButtonEnabled(true);
    presetLayout->addWidget(searchBox_);

    presetList_ = new QListWidget;
    presetList_->setMinimumHeight(150);
    presetList_->setAlternatingRowColors(true);
    presetLayout->addWidget(presetList_);

    importLutBtn_ = new QPushButton("Import Custom LUT (.cube)");
    presetLayout->addWidget(importLutBtn_);

    scrollLayout->addWidget(presetGroup);

    // Adjustments group
    auto* adjustGroup = new QGroupBox("Adjustments");
    auto* adjustLayout = new QVBoxLayout(adjustGroup);

    adjustLayout->addLayout(createSliderRow(intensitySlider_, intensityLabel_,
                                             "Intensity", 0, 100, 100, "%"));
    adjustLayout->addLayout(createSliderRow(grainSlider_, grainLabel_,
                                             "Grain", 0, 100, 0, "%"));
    adjustLayout->addLayout(createSliderRow(vignetteSlider_, vignetteLabel_,
                                             "Vignette", 0, 100, 0, "%"));
    adjustLayout->addLayout(createSliderRow(temperatureSlider_, temperatureLabel_,
                                             "Temperature", -100, 100, 0));
    adjustLayout->addLayout(createSliderRow(tintSlider_, tintLabel_,
                                             "Tint", -100, 100, 0));
    adjustLayout->addLayout(createSliderRow(contrastSlider_, contrastLabel_,
                                             "Contrast", -100, 100, 0));
    adjustLayout->addLayout(createSliderRow(brightnessSlider_, brightnessLabel_,
                                             "Brightness", -100, 100, 0));
    adjustLayout->addLayout(createSliderRow(saturationSlider_, saturationLabel_,
                                             "Saturation", -100, 100, 0));
    adjustLayout->addLayout(createSliderRow(highlightsSlider_, highlightsLabel_,
                                             "Highlights", -100, 100, 0));
    adjustLayout->addLayout(createSliderRow(shadowsSlider_, shadowsLabel_,
                                             "Shadows", -100, 100, 0));

    scrollLayout->addWidget(adjustGroup);

    // Save preset button
    savePresetBtn_ = new QPushButton("Save as New Preset");
    scrollLayout->addWidget(savePresetBtn_);

    scrollLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);
}

void FilterPanel::connectSignals() {
    // Preset selection
    connect(presetList_, &QListWidget::currentRowChanged, this, [this](int) {
        auto* item = presetList_->currentItem();
        if (item) {
            emit presetSelected(item->data(Qt::UserRole).toString().toStdString());
        }
    });

    // Search filter
    connect(searchBox_, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (int i = 0; i < presetList_->count(); ++i) {
            auto* item = presetList_->item(i);
            bool match = text.isEmpty() ||
                         item->text().contains(text, Qt::CaseInsensitive);
            item->setHidden(!match);
        }
    });

    // Parameter sliders
    auto connectSlider = [this](QSlider* slider, QLabel* label,
                                 const QString& suffix, bool emitSignal = true) {
        connect(slider, &QSlider::valueChanged, this, [label, suffix, this, emitSignal](int val) {
            label->setText(QString::number(val) + suffix);
            if (emitSignal) emit parametersChanged();
        });
    };

    connectSlider(intensitySlider_, intensityLabel_, "%");
    connectSlider(grainSlider_, grainLabel_, "%");
    connectSlider(vignetteSlider_, vignetteLabel_, "%");
    connectSlider(temperatureSlider_, temperatureLabel_, "");
    connectSlider(tintSlider_, tintLabel_, "");
    connectSlider(contrastSlider_, contrastLabel_, "");
    connectSlider(brightnessSlider_, brightnessLabel_, "");
    connectSlider(saturationSlider_, saturationLabel_, "");
    connectSlider(highlightsSlider_, highlightsLabel_, "");
    connectSlider(shadowsSlider_, shadowsLabel_, "");

    connect(savePresetBtn_, &QPushButton::clicked, this, &FilterPanel::saveAsPresetRequested);
    connect(importLutBtn_, &QPushButton::clicked, this, &FilterPanel::importLutRequested);
}

void FilterPanel::setAvailablePresets(
    const std::vector<std::string>& presetIds,
    const std::unordered_map<std::string, PresetInfo>& presets)
{
    presetList_->clear();
    for (const auto& id : presetIds) {
        auto it = presets.find(id);
        if (it != presets.end()) {
            auto* item = new QListWidgetItem;
            item->setText(QString::fromStdString(it->second.name));
            item->setData(Qt::UserRole, QString::fromStdString(id));

            // Category color coding
            switch (it->second.category) {
                case FilterCategory::Cinematic:
                    item->setForeground(QColor("#ff9800"));
                    break;
                case FilterCategory::JapanStyle:
                    item->setForeground(QColor("#e91e63"));
                    break;
                case FilterCategory::VintageRetro:
                    item->setForeground(QColor("#8bc34a"));
                    break;
                case FilterCategory::Monochrome:
                    item->setForeground(QColor("#9e9e9e"));
                    break;
                default:
                    item->setForeground(QColor("#03a9f4"));
                    break;
            }

            presetList_->addItem(item);
        }
    }
}

FilterParameters FilterPanel::currentParams() const {
    FilterParameters p;
    p.intensity = static_cast<float>(intensitySlider_->value());
    p.grainAmount = static_cast<float>(grainSlider_->value());
    p.vignetteStrength = static_cast<float>(vignetteSlider_->value());
    p.temperature = static_cast<float>(temperatureSlider_->value());
    p.tint = static_cast<float>(tintSlider_->value());
    p.contrast = static_cast<float>(contrastSlider_->value());
    p.brightness = static_cast<float>(brightnessSlider_->value());
    p.saturation = static_cast<float>(saturationSlider_->value());
    p.highlights = static_cast<float>(highlightsSlider_->value());
    p.shadows = static_cast<float>(shadowsSlider_->value());
    return p;
}

void FilterPanel::setParams(const FilterParameters& p) {
    intensitySlider_->setValue(static_cast<int>(p.intensity));
    grainSlider_->setValue(static_cast<int>(p.grainAmount));
    vignetteSlider_->setValue(static_cast<int>(p.vignetteStrength));
    temperatureSlider_->setValue(static_cast<int>(p.temperature));
    tintSlider_->setValue(static_cast<int>(p.tint));
    contrastSlider_->setValue(static_cast<int>(p.contrast));
    brightnessSlider_->setValue(static_cast<int>(p.brightness));
    saturationSlider_->setValue(static_cast<int>(p.saturation));
    highlightsSlider_->setValue(static_cast<int>(p.highlights));
    shadowsSlider_->setValue(static_cast<int>(p.shadows));
}

std::string FilterPanel::selectedPresetId() const {
    auto* item = presetList_->currentItem();
    if (item) return item->data(Qt::UserRole).toString().toStdString();
    return "";
}

void FilterPanel::setSelectedPreset(const std::string& presetId) {
    QString qid = QString::fromStdString(presetId);
    for (int i = 0; i < presetList_->count(); ++i) {
        if (presetList_->item(i)->data(Qt::UserRole).toString() == qid) {
            presetList_->setCurrentRow(i);
            return;
        }
    }
}

} // namespace PixelForge