#include "ImageCanvas.h"

#include <opencv2/imgproc.hpp>

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPainterPath>

#include <cmath>

namespace PixelForge {

struct ImageCanvas::Impl {
    QPixmap currentPixmap;
    QPixmap comparisonPixmap;
    bool comparisonEnabled = false;
    float splitPosition = 0.5f;

    float zoom = 1.0f;
    QPointF panOffset{0, 0};
    QPointF lastMousePos{0, 0};
    bool panning = false;

    QPointF imageOffset{0, 0}; // top-left of image in widget coords
    QSizeF scaledSize{0, 0};
};

ImageCanvas::ImageCanvas(QWidget* parent)
    : QWidget(parent), impl_(std::make_unique<Impl>())
{
    setMinimumSize(200, 200);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet("background-color: #2b2b2b;");
}

ImageCanvas::~ImageCanvas() = default;

// ============================================================
// Image Management
// ============================================================

void ImageCanvas::setImage(const Image& image) {
    if (image.isEmpty()) {
        impl_->currentPixmap = QPixmap();
        update();
        emit imageChanged();
        return;
    }

    // Convert to QImage (already in RGB)
    const cv::Mat& mat = image.mat();
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

    impl_->currentPixmap = QPixmap::fromImage(qImg);

    if (!impl_->comparisonEnabled) {
        zoomFit();
    }

    update();
    emit imageChanged();
}

void ImageCanvas::setComparisonImage(const Image& beforeImage) {
    if (beforeImage.isEmpty()) {
        impl_->comparisonPixmap = QPixmap();
        update();
        return;
    }

    const cv::Mat& mat = beforeImage.mat();
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

    impl_->comparisonPixmap = QPixmap::fromImage(qImg);
    update();
}

void ImageCanvas::setComparisonEnabled(bool enabled) {
    impl_->comparisonEnabled = enabled;
    update();
}

bool ImageCanvas::isComparisonEnabled() const {
    return impl_->comparisonEnabled;
}

void ImageCanvas::setSplitPosition(float position) {
    impl_->splitPosition = std::clamp(position, 0.0f, 1.0f);
    update();
}

bool ImageCanvas::hasImage() const {
    return !impl_->currentPixmap.isNull();
}

QPixmap ImageCanvas::currentPixmap() const {
    return impl_->currentPixmap;
}

// ============================================================
// Zoom Controls
// ============================================================

void ImageCanvas::zoomIn() {
    impl_->zoom = std::min(impl_->zoom * 1.25f, 20.0f);
    updateTransform();
    update();
    emit zoomChanged(impl_->zoom);
}

void ImageCanvas::zoomOut() {
    impl_->zoom = std::max(impl_->zoom / 1.25f, 0.05f);
    updateTransform();
    update();
    emit zoomChanged(impl_->zoom);
}

void ImageCanvas::zoomFit() {
    if (impl_->currentPixmap.isNull()) return;

    float wRatio = static_cast<float>(width()) / impl_->currentPixmap.width();
    float hRatio = static_cast<float>(height()) / impl_->currentPixmap.height();
    impl_->zoom = std::min(wRatio, hRatio) * 0.95f;

    impl_->panOffset = QPointF(0, 0);
    updateTransform();
    update();
    emit zoomChanged(impl_->zoom);
}

void ImageCanvas::zoomActual() {
    impl_->zoom = 1.0f;
    impl_->panOffset = QPointF(0, 0);
    updateTransform();
    update();
    emit zoomChanged(impl_->zoom);
}

float ImageCanvas::zoomFactor() const {
    return impl_->zoom;
}

// ============================================================
// Transform
// ============================================================

void ImageCanvas::updateTransform() {
    if (impl_->currentPixmap.isNull()) return;

    float scaledW = impl_->currentPixmap.width() * impl_->zoom;
    float scaledH = impl_->currentPixmap.height() * impl_->zoom;
    impl_->scaledSize = QSizeF(scaledW, scaledH);

    float offsetX = (width() - scaledW) / 2.0f + impl_->panOffset.x();
    float offsetY = (height() - scaledH) / 2.0f + impl_->panOffset.y();
    impl_->imageOffset = QPointF(offsetX, offsetY);
}

QPointF ImageCanvas::widgetToImage(const QPointF& widgetPos) const {
    float x = (widgetPos.x() - impl_->imageOffset.x()) / impl_->zoom;
    float y = (widgetPos.y() - impl_->imageOffset.y()) / impl_->zoom;
    return QPointF(x, y);
}

// ============================================================
// Painting
// ============================================================

void ImageCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Dark background
    painter.fillRect(rect(), QColor(43, 43, 43));

    if (impl_->currentPixmap.isNull()) {
        // Draw placeholder text
        painter.setPen(QColor(120, 120, 120));
        painter.setFont(QFont("Segoe UI", 12));
        painter.drawText(rect(), Qt::AlignCenter, "Drag & drop an image here\nor use File > Open");
        return;
    }

    updateTransform();

    QRectF targetRect(impl_->imageOffset, impl_->scaledSize);

    if (impl_->comparisonEnabled && !impl_->comparisonPixmap.isNull()) {
        // Split view: left = before, right = after
        float splitX = impl_->imageOffset.x() + impl_->scaledSize.width() * impl_->splitPosition;

        // Draw "before" on left
        painter.save();
        QPainterPath leftClip;
        leftClip.addRect(0, 0, splitX, height());
        painter.setClipPath(leftClip);
        painter.drawPixmap(targetRect, impl_->comparisonPixmap,
                           QRectF(0, 0, impl_->comparisonPixmap.width(),
                                  impl_->comparisonPixmap.height()));
        painter.restore();

        // Draw "after" on right
        painter.save();
        QPainterPath rightClip;
        rightClip.addRect(splitX, 0, width() - splitX, height());
        painter.setClipPath(rightClip);
        painter.drawPixmap(targetRect, impl_->currentPixmap,
                           QRectF(0, 0, impl_->currentPixmap.width(),
                                  impl_->currentPixmap.height()));
        painter.restore();

        // Draw split line
        painter.setPen(QPen(Qt::white, 2));
        painter.drawLine(QPointF(splitX, 0), QPointF(splitX, height()));

        // Labels
        painter.setPen(Qt::white);
        painter.setFont(QFont("Segoe UI", 9, QFont::Bold));
        painter.drawText(QRectF(5, 5, 60, 20), "BEFORE");
        painter.drawText(QRectF(width() - 65, 5, 60, 20), "AFTER");

    } else {
        // Normal single image display
        painter.drawPixmap(targetRect, impl_->currentPixmap,
                           QRectF(0, 0, impl_->currentPixmap.width(),
                                  impl_->currentPixmap.height()));
    }

    // Draw zoom indicator
    painter.setPen(QColor(180, 180, 180));
    painter.setFont(QFont("Segoe UI", 8));
    painter.drawText(QRectF(width() - 80, height() - 25, 75, 20),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QString::number(static_cast<int>(impl_->zoom * 100)) + "%");

    // Draw image dimensions
    if (!impl_->currentPixmap.isNull()) {
        QString dims = QString("%1 × %2")
            .arg(impl_->currentPixmap.width())
            .arg(impl_->currentPixmap.height());
        painter.drawText(QRectF(5, height() - 25, 120, 20),
                         Qt::AlignLeft | Qt::AlignVCenter, dims);
    }
}

void ImageCanvas::resizeEvent(QResizeEvent*) {
    updateTransform();
}

// ============================================================
// Mouse / Wheel
// ============================================================

void ImageCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton) {
        impl_->panning = true;
        impl_->lastMousePos = event->position();
        setCursor(Qt::ClosedHandCursor);
    }
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (impl_->panning) {
        QPointF delta = event->position() - impl_->lastMousePos;
        impl_->panOffset += delta;
        impl_->lastMousePos = event->position();
        update();
    }
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton) {
        impl_->panning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void ImageCanvas::wheelEvent(QWheelEvent* event) {
    if (impl_->currentPixmap.isNull()) return;

    float delta = event->angleDelta().y() / 120.0f;
    float factor = std::pow(1.15f, delta);

    float newZoom = impl_->zoom * factor;
    newZoom = std::clamp(newZoom, 0.05f, 20.0f);

    // Zoom toward mouse position
    QPointF mousePos = event->position();
    QPointF imagePos = widgetToImage(mousePos);

    impl_->zoom = newZoom;
    updateTransform();

    // Adjust pan to keep point under mouse
    QPointF newWidgetPos(
        imagePos.x() * impl_->zoom + impl_->imageOffset.x(),
        imagePos.y() * impl_->zoom + impl_->imageOffset.y()
    );
    impl_->panOffset += mousePos - newWidgetPos;

    updateTransform();
    update();
    emit zoomChanged(impl_->zoom);
}

QImage ImageCanvas::matToQImage(const cv::Mat& mat) const {
    if (mat.channels() == 3) {
        return QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step), QImage::Format_RGB888).copy();
    } else if (mat.channels() == 4) {
        return QImage(mat.data, mat.cols, mat.rows,
                      static_cast<int>(mat.step), QImage::Format_RGBA8888).copy();
    }
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows,
                  static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
}

} // namespace PixelForge