#pragma once

#include "core/CoreTypes.h"
#include "core/Image.h"

#include <QDialog>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>

namespace PixelForge {

/// Dialog for basic crop and rotate operations.
class CropRotateDialog : public QDialog {
    Q_OBJECT

public:
    explicit CropRotateDialog(const Image& image, QWidget* parent = nullptr);
    ~CropRotateDialog() override;

    /// Get the processed result image
    Image result() const;

private slots:
    void onRotateCW();
    void onRotateCCW();
    void onRotate180();
    void onFlipH();
    void onFlipV();
    void onRotateAngleChanged(double angle);
    void onAspectRatioChanged(int index);
    void onResetCrop();
    void onApply();

private:
    Image sourceImage_;
    Image result_;
    QGraphicsScene* scene_ = nullptr;
    QGraphicsView* view_ = nullptr;
    QGraphicsPixmapItem* pixmapItem_ = nullptr;
    QGraphicsRectItem* cropRect_ = nullptr;

    QDoubleSpinBox* angleSpin_ = nullptr;
    QComboBox* aspectCombo_ = nullptr;
    QLabel* dimensionsLabel_ = nullptr;

    float rotationAngle_ = 0.0f;
    bool applied_ = false;

    void setupUi();
    void updatePreview();
    void updateDimensionsLabel();
};

} // namespace PixelForge