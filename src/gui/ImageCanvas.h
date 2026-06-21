#pragma once

#include "core/CoreTypes.h"
#include "core/Image.h"

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QPointF>
#include <QSize>

namespace PixelForge {

/// Widget that displays an image with zoom/pan capabilities.
/// Supports split-view (before/after) comparison mode.
class ImageCanvas : public QWidget {
    Q_OBJECT

public:
    explicit ImageCanvas(QWidget* parent = nullptr);
    ~ImageCanvas() override;

    /// Set the displayed image
    void setImage(const Image& image);

    /// Set the "before" image for comparison
    void setComparisonImage(const Image& beforeImage);

    /// Toggle comparison mode
    void setComparisonEnabled(bool enabled);
    bool isComparisonEnabled() const;

    /// Set comparison split position (0.0 - 1.0)
    void setSplitPosition(float position);

    /// Zoom controls
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoomActual();

    float zoomFactor() const;

    /// Check if an image is loaded
    bool hasImage() const;

    /// Get current image as QPixmap
    QPixmap currentPixmap() const;

signals:
    void imageChanged();
    void zoomChanged(float zoom);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    void updateTransform();
    QImage matToQImage(const cv::Mat& mat) const;
    QPointF widgetToImage(const QPointF& widgetPos) const;
};

} // namespace PixelForge