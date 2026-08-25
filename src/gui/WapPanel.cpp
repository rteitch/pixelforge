#include "WapPanel.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QSignalBlocker>
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
    paletteCombo_->addItem("Sunset", static_cast<int>(WapPalettePreset::Sunset));
    paletteCombo_->addItem("Ocean", static_cast<int>(WapPalettePreset::Ocean));
    paletteCombo_->addItem("Auto (K-Means)", static_cast<int>(WapPalettePreset::Auto));
    paletteCombo_->addItem("Custom Palette", static_cast<int>(WapPalettePreset::Custom));
    formLayout->addRow("Palette:", paletteCombo_);

    editPaletteBtn_ = new QPushButton("Edit Custom Palette...");
    editPaletteBtn_->setEnabled(false);
    formLayout->addRow(editPaletteBtn_);

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
            this, [this](int index) {
                editPaletteBtn_->setEnabled(
                    paletteCombo_->itemData(index).toInt() ==
                    static_cast<int>(WapPalettePreset::Custom));
                emit parametersChanged();
            });

    connect(editPaletteBtn_, &QPushButton::clicked, this, [this]() {
        QDialog dialog(this);
        dialog.setWindowTitle("Custom WPAP Palette");
        dialog.setMinimumSize(360, 300);
        auto* layout = new QVBoxLayout(&dialog);
        auto* list = new QListWidget;
        std::vector<Color3u8> workingPalette = customPalette_;
        auto refresh = [&workingPalette, list]() {
            list->clear();
            for (const auto& color : workingPalette) {
                auto* item = new QListWidgetItem(
                    QString("#%1%2%3")
                        .arg(color.r, 2, 16, QChar('0'))
                        .arg(color.g, 2, 16, QChar('0'))
                        .arg(color.b, 2, 16, QChar('0')).toUpper());
                item->setBackground(QColor(color.r, color.g, color.b));
                item->setForeground(QColor(
                    color.r + color.g + color.b > 360 ? Qt::black : Qt::white));
                list->addItem(item);
            }
        };
        refresh();
        layout->addWidget(list);

        auto* paletteButtons = new QHBoxLayout;
        auto* add = new QPushButton("Add");
        auto* edit = new QPushButton("Edit");
        auto* remove = new QPushButton("Remove");
        paletteButtons->addWidget(add);
        paletteButtons->addWidget(edit);
        paletteButtons->addWidget(remove);
        paletteButtons->addStretch();
        layout->addLayout(paletteButtons);

        connect(add, &QPushButton::clicked, &dialog,
            [this, &workingPalette, refresh]() {
            QColor color = QColorDialog::getColor(Qt::white, this, "Add Palette Color");
            if (color.isValid() && workingPalette.size() < 32) {
                workingPalette.push_back({static_cast<uint8_t>(color.red()),
                                          static_cast<uint8_t>(color.green()),
                                          static_cast<uint8_t>(color.blue())});
                refresh();
            }
        });
        connect(edit, &QPushButton::clicked, &dialog,
            [this, &workingPalette, list, refresh]() {
            int row = list->currentRow();
            if (row < 0 || row >= static_cast<int>(workingPalette.size())) return;
            const auto& old = workingPalette[row];
            QColor color = QColorDialog::getColor(
                QColor(old.r, old.g, old.b), this, "Edit Palette Color");
            if (color.isValid()) {
                workingPalette[row] = {static_cast<uint8_t>(color.red()),
                                       static_cast<uint8_t>(color.green()),
                                       static_cast<uint8_t>(color.blue())};
                refresh();
            }
        });
        connect(remove, &QPushButton::clicked, &dialog,
                [&workingPalette, list, refresh]() {
            int row = list->currentRow();
            if (row >= 0 && row < static_cast<int>(workingPalette.size())) {
                workingPalette.erase(workingPalette.begin() + row);
                refresh();
            }
        });

        auto* dialogButtons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout->addWidget(dialogButtons);
        connect(dialogButtons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(dialogButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() == QDialog::Accepted) {
            customPalette_ = std::move(workingPalette);
            paletteCombo_->setCurrentIndex(paletteCombo_->findData(
                static_cast<int>(WapPalettePreset::Custom)));
            emit parametersChanged();
        }
    });

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
    params.customPalette = customPalette_;
    return params;
}

void WapPanel::setParams(const WapParameters& params) {
    QSignalBlocker blockers[] = {
        QSignalBlocker(colorCountSlider_), QSignalBlocker(detailCombo_),
        QSignalBlocker(customPointSpin_), QSignalBlocker(paletteCombo_),
        QSignalBlocker(faceDetectionCheck_), QSignalBlocker(faceBoostSlider_)
    };
    colorCountSlider_->setValue(params.colorCount);
    detailCombo_->setCurrentIndex(static_cast<int>(params.detailLevel));
    customPointSpin_->setValue(params.customPointCount);
    paletteCombo_->setCurrentIndex(static_cast<int>(params.palettePreset));
    faceDetectionCheck_->setChecked(params.faceDetectionEnabled);
    faceBoostSlider_->setValue(static_cast<int>(params.faceDetailBoost * 100));
    customPalette_ = params.customPalette;
    int paletteIndex = paletteCombo_->findData(static_cast<int>(params.palettePreset));
    if (paletteIndex >= 0) paletteCombo_->setCurrentIndex(paletteIndex);
}

} // namespace PixelForge