#include "WapPanel.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

namespace PixelForge {

WapPanel::WapPanel(QWidget* parent) : QWidget(parent) {
    setupUi();
    connectSignals();
}

WapPanel::~WapPanel() = default;

void WapPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // WPAP Parameters group
    auto* paramsGroup = new QGroupBox("WPAP Parameters");
    auto* formLayout = new QFormLayout(paramsGroup);

    // Color count
    colorCountSlider_ = new QSlider(Qt::Horizontal);
    colorCountSlider_->setRange(6, 32);
    colorCountSlider_->setValue(16);
    colorCountLabel_ = new QLabel("16");
    auto* colorLayout = new QHBoxLayout;
    colorLayout->addWidget(colorCountSlider_);
    colorLayout->addWidget(colorCountLabel_);
    formLayout->addRow("Colors:", colorLayout);

    // Detail level
    detailCombo_ = new QComboBox;
    detailCombo_->addItem("Low", static_cast<int>(WapDetailLevel::Low));
    detailCombo_->addItem("Medium", static_cast<int>(WapDetailLevel::Medium));
    detailCombo_->addItem("High", static_cast<int>(WapDetailLevel::High));
    detailCombo_->addItem("Custom", static_cast<int>(WapDetailLevel::Custom));
    detailCombo_->setCurrentIndex(1);
    formLayout->addRow("Detail:", detailCombo_);

    // Custom point count
    customPointSpin_ = new QSpinBox;
    customPointSpin_->setRange(100, 8000);
    customPointSpin_->setValue(2000);
    customPointSpin_->setEnabled(false);
    formLayout->addRow("Points:", customPointSpin_);

    // Palette
    paletteCombo_ = new QComboBox;
    paletteCombo_->addItem("Vibrant", static_cast<int>(WapPalettePreset::Vibrant));
    paletteCombo_->addItem("Pastel", static_cast<int>(WapPalettePreset::Pastel));
    paletteCombo_->addItem("Mono + Accent", static_cast<int>(WapPalettePreset::MonochromeAccent));
    paletteCombo_->addItem("Auto (K-Means)", static_cast<int>(WapPalettePreset::Custom));
    formLayout->addRow("Palette:", paletteCombo_);

    // Face detection
    faceDetectionCheck_ = new QCheckBox("Enable face detection");
    faceDetectionCheck_->setChecked(true);
    formLayout->addRow(faceDetectionCheck_);

    // Face detail boost
    faceBoostSlider_ = new QSlider(Qt::Horizontal);
    faceBoostSlider_->setRange(100, 300);
    faceBoostSlider_->setValue(150);
    faceBoostLabel_ = new QLabel("1.5x");
    auto* boostLayout = new QHBoxLayout;
    boostLayout->addWidget(faceBoostSlider_);
    boostLayout->addWidget(faceBoostLabel_);
    formLayout->addRow("Face Boost:", boostLayout);

    mainLayout->addWidget(paramsGroup);

    // Buttons
    generateBtn_ = new QPushButton("Generate WPAP");
    generateBtn_->setStyleSheet(
        "QPushButton { background-color: #4a7dff; color: white; "
        "font-weight: bold; padding: 8px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3a6def; }");
    mainLayout->addWidget(generateBtn_);

    exportSvgBtn_ = new QPushButton("Export as SVG");
    mainLayout->addWidget(exportSvgBtn_);

    mainLayout->addStretch();
}

void WapPanel::connectSignals() {
    connect(colorCountSlider_, &QSlider::valueChanged, this, [this](int val) {
        colorCountLabel_->setText(QString::number(val));
        emit parametersChanged();
    });

    connect(detailCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        customPointSpin_->setEnabled(idx == 3); // Custom
        emit parametersChanged();
    });

    connect(customPointSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &WapPanel::parametersChanged);

    connect(paletteCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WapPanel::parametersChanged);

    connect(faceDetectionCheck_, &QCheckBox::toggled,
            this, &WapPanel::parametersChanged);

    connect(faceBoostSlider_, &QSlider::valueChanged, this, [this](int val) {
        faceBoostLabel_->setText(QString::number(val / 100.0f, 'f', 1) + "x");
        emit parametersChanged();
    });

    connect(generateBtn_, &QPushButton::clicked, this, &WapPanel::generateRequested);
    connect(exportSvgBtn_, &QPushButton::clicked, this, &WapPanel::exportRequested);
}

WapParameters WapPanel::currentParams() const {
    WapParameters params;
    params.colorCount = colorCountSlider_->value();
    params.detailLevel = static_cast<WapDetailLevel>(detailCombo_->currentData().toInt());
    params.customPointCount = customPointSpin_->value();
    params.palettePreset = static_cast<WapPalettePreset>(paletteCombo_->currentData().toInt());
    params.faceDetectionEnabled = faceDetectionCheck_->isChecked();
    params.faceDetailBoost = faceBoostSlider_->value() / 100.0f;
    return params;
}

void WapPanel::setParams(const WapParameters& params) {
    colorCountSlider_->setValue(params.colorCount);
    detailCombo_->setCurrentIndex(static_cast<int>(params.detailLevel));
    customPointSpin_->setValue(params.customPointCount);
    paletteCombo_->setCurrentIndex(static_cast<int>(params.palettePreset));
    faceDetectionCheck_->setChecked(params.faceDetectionEnabled);
    faceBoostSlider_->setValue(static_cast<int>(params.faceDetailBoost * 100));
}

} // namespace PixelForge