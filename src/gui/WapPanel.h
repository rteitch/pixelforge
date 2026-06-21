#pragma once

#include "core/CoreTypes.h"

#include <QWidget>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>

namespace PixelForge {

/// Left panel for WPAP parameter controls.
class WapPanel : public QWidget {
    Q_OBJECT

public:
    explicit WapPanel(QWidget* parent = nullptr);
    ~WapPanel() override;

    WapParameters currentParams() const;
    void setParams(const WapParameters& params);

signals:
    void parametersChanged();
    void generateRequested();
    void exportRequested();

private:
    QSlider* colorCountSlider_ = nullptr;
    QLabel* colorCountLabel_ = nullptr;
    QComboBox* detailCombo_ = nullptr;
    QSpinBox* customPointSpin_ = nullptr;
    QComboBox* paletteCombo_ = nullptr;
    QCheckBox* faceDetectionCheck_ = nullptr;
    QSlider* faceBoostSlider_ = nullptr;
    QLabel* faceBoostLabel_ = nullptr;
    QPushButton* generateBtn_ = nullptr;
    QPushButton* exportSvgBtn_ = nullptr;

    void setupUi();
    void connectSignals();
};

} // namespace PixelForge