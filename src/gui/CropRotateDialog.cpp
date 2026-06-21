#include "CropRotateDialog.h"

#include <opencv2/imgproc.hpp>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <QTransform>

#include <cmath>

namespace PixelForge {

CropRotateDialog::CropRotateDialog(const Image& image, QWidget* parent)
    : QDialog(parent), sourceImage_(image.deepCopy()), result_(image.deepCopy())
{
    setupUi();
    updatePreview();
    setWindowTitle("Crop & Rotate");
    setMinimumSize(800, 600);
    resize(900, 700);
}

CropRotateDialog::~CropRotateDialog() = default;

Image CropRotateDialog::result() const {
    return result_;
}

void CropRotateDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // Graphics view for image preview
    scene_ = new QGraphicsScene(this);
    view_ = new QGraphicsView(scene_);
    view_->setRenderHint(QPainter::SmoothPixmapTransform);
    view_->setDragMode(QGraphicsView::ScrollHandDrag);
    view_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    mainLayout->addWidget(view_, 1);

    // Controls panel
    auto* controlsLayout = new QHBoxLayout;

    // Rotation controls
    auto* rotateGroup = new QGroupBox("Rotation");
    auto* rotateLayout = new QVBoxLayout(rotateGroup);

    auto* rotBtnLayout = new QHBoxLayout;
    auto* cwBtn = new QPushButton("90° CW");
    auto* ccwBtn = new QPushButton("90° CCW");
    auto* rot180Btn = new QPushButton("180°");
    rotBtnLayout->addWidget(ccwBtn);
    rotBtnLayout->addWidget(rot180Btn);
    rotBtnLayout->addWidget(cwBtn);
    rotateLayout->addLayout(rotBtnLayout);

    auto* flipBtnLayout = new QHBoxLayout;
    auto* flipHBtn = new QPushButton("Flip Horizontal");
    auto* flipVBtn = new QPushButton("Flip Vertical");
    flipBtnLayout->addWidget(flipHBtn);
    flipBtnLayout->addWidget(flipVBtn);
    rotateLayout->addLayout(flipBtnLayout);

    auto* angleLayout = new QHBoxLayout;
    angleLayout->addWidget(new QLabel("Fine Rotate:"));
    angleSpin_ = new QDoubleSpinBox;
    angleSpin_->setRange(-180.0, 180.0);
    angleSpin_->setValue(0.0);
    angleSpin_->setSuffix("°");
    angleSpin_->setSingleStep(0.5);
    angleLayout->addWidget(angleSpin_);
    rotateLayout->addLayout(angleLayout);

    controlsLayout->addWidget(rotateGroup);

    // Crop controls
    auto* cropGroup = new QGroupBox("Crop");
    auto* cropLayout = new QVBoxLayout(cropGroup);

    cropLayout->addWidget(new QLabel("Aspect Ratio:"));
    aspectCombo_ = new QComboBox;
    aspectCombo_->addItem("Free", 0);
    aspectCombo_->addItem("1:1 (Square)", 1);
    aspectCombo_->addItem("4:3", 2);
    aspectCombo_->addItem("3:2", 3);
    aspectCombo_->addItem("16:9", 4);
    aspectCombo_->addItem("2:3 (Portrait)", 5);
    cropLayout->addWidget(aspectCombo_);

    auto* resetCropBtn = new QPushButton("Reset Crop");
    cropLayout->addWidget(resetCropBtn);

    cropLayout->addStretch();

    controlsLayout->addWidget(cropGroup);

    // Info
    auto* infoGroup = new QGroupBox("Output");
    auto* infoLayout = new QVBoxLayout(infoGroup);
    dimensionsLabel_ = new QLabel("0 × 0");
    dimensionsLabel_->setStyleSheet("font-size: 14px; font-weight: bold;");
    infoLayout->addWidget(dimensionsLabel_);
    infoLayout->addStretch();
    controlsLayout->addWidget(infoGroup);

    mainLayout->addLayout(controlsLayout);

    // Dialog buttons
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Apply");
    mainLayout->addWidget(buttonBox);

    // Connections
    connect(cwBtn, &QPushButton::clicked, this, &CropRotateDialog::onRotateCW);
    connect(ccwBtn, &QPushButton::clicked, this, &CropRotateDialog::onRotateCCW);
    connect(rot180Btn, &QPushButton::clicked, this, &CropRotateDialog::onRotate180);
    connect(flipHBtn, &QPushButton::clicked, this, &CropRotateDialog::onFlipH);
    connect(flipVBtn, &QPushButton::clicked, this, &CropRotateDialog::onFlipV);
    connect(angleSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CropRotateDialog::onRotateAngleChanged);
    connect(aspectCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CropRotateDialog::onAspectRatioChanged);
    connect(resetCropBtn, &QPushButton::clicked, this, &CropRotateDialog::onResetCrop);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CropRotateDialog::onApply);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void CropRotateDialog::onRotateCW() {
    sourceImage_ = sourceImage_.rotated90(1);
    rotationAngle_ = 0;
    angleSpin_->setValue(0);
    result_ = sourceImage_;
    updatePreview();
}

void CropRotateDialog::onRotateCCW() {
    sourceImage_ = sourceImage_.rotated90(3);
    rotationAngle_ = 0;
    angleSpin_->setValue(0);
    result_ = sourceImage_;
    updatePreview();
}

void CropRotateDialog::onRotate180() {
    sourceImage_ = sourceImage_.rotated90(2);
    rotationAngle_ = 0;
    angleSpin_->setValue(0);
    result_ = sourceImage_;
    updatePreview();
}

void CropRotateDialog::onFlipH() {
    cv::Mat flipped;
    cv::flip(sourceImage_.mat(), flipped, 1);
    sourceImage_ = Image(flipped);
    result_ = sourceImage_;
    updatePreview();
}

void CropRotateDialog::onFlipV() {
    cv::Mat flipped;
    cv::flip(sourceImage_.mat(), flipped, 0);
    sourceImage_ = Image(flipped);
    result_ = sourceImage_;
    updatePreview();
}

void CropRotateDialog::onRotateAngleChanged(double angle) {
    rotationAngle_ = static_cast<float>(angle);
    if (std::abs(rotationAngle_) < 0.1f) {
        result_ = sourceImage_;
    } else {
        result_ = sourceImage_.rotated(rotationAngle_, {128, 128, 128});
    }
    updatePreview();
}

void CropRotateDialog::onAspectRatioChanged(int /*index*/) {
    // Aspect ratio crop selection would be applied interactively
    // For v1.1, we just store the preference
}

void CropRotateDialog::onResetCrop() {
    rotationAngle_ = 0;
    angleSpin_->setValue(0);
    result_ = sourceImage_;
    updatePreview();
}

void CropRotateDialog::onApply() {
    applied_ = true;
    accept();
}

void CropRotateDialog::updatePreview() {
    if (result_.isEmpty()) return;

    scene_->clear();

    const cv::Mat& mat = result_.mat();
    QImage qImg;
    if (mat.channels() == 3) {
        qImg = QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step), QImage::Format_RGB888).copy();
    } else if (mat.channels() == 4) {
        qImg = QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step), QImage::Format_RGBA8888).copy();
    } else {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
        qImg = QImage(rgb.data, rgb.cols, rgb.rows,
                      static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
    }

    pixmapItem_ = scene_->addPixmap(QPixmap::fromImage(qImg));
    scene_->setSceneRect(pixmapItem_->boundingRect());
    view_->fitInView(pixmapItem_, Qt::KeepAspectRatio);

    updateDimensionsLabel();
}

void CropRotateDialog::updateDimensionsLabel() {
    if (!result_.isEmpty()) {
        dimensionsLabel_->setText(
            QString("%1 × %2 px").arg(result_.width()).arg(result_.height()));
    }
}

} // namespace PixelForge