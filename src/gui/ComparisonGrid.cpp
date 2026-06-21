#include "ComparisonGrid.h"

#include <opencv2/imgproc.hpp>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QApplication>

#include <cmath>

namespace PixelForge {

ComparisonGrid::ComparisonGrid(const Image& sourceImage,
                               ColorGradingModule& gradingModule,
                               QWidget* parent)
    : QDialog(parent), sourceImage_(sourceImage), gradingModule_(gradingModule)
{
    setupUi();
    setWindowTitle("Comparison Grid");
    setMinimumSize(900, 700);
    resize(1000, 800);
}

ComparisonGrid::~ComparisonGrid() = default;

void ComparisonGrid::setPresetIds(const std::vector<std::string>& ids) {
    presetIds_ = ids;
    generateThumbnails();
}

std::string ComparisonGrid::selectedPresetId() const {
    return selectedId_;
}

void ComparisonGrid::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    QLabel* header = new QLabel("Click a preset to apply it. Scroll to see all presets.");
    header->setStyleSheet("font-size: 12px; color: #aaa; padding: 5px;");
    mainLayout->addWidget(header);

    scrollArea_ = new QScrollArea;
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* gridWidget = new QWidget;
    gridLayout_ = new QGridLayout(gridWidget);
    gridLayout_->setSpacing(8);
    gridLayout_->setContentsMargins(8, 8, 8, 8);

    scrollArea_->setWidget(gridWidget);
    mainLayout->addWidget(scrollArea_);

    auto* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignRight);
}

QPixmap ComparisonGrid::createThumbnail(const Image& img, int maxSize) {
    if (img.isEmpty()) return QPixmap();

    const cv::Mat& mat = img.mat();
    int w = mat.cols, h = mat.rows;
    float scale = static_cast<float>(maxSize) / static_cast<float>(std::max(w, h));
    int newW = std::max(1, static_cast<int>(w * scale));
    int newH = std::max(1, static_cast<int>(h * scale));

    cv::Mat resized;
    cv::resize(mat, resized, cv::Size(newW, newH), 0, 0, cv::INTER_AREA);

    QImage qImg;
    if (resized.channels() == 3) {
        qImg = QImage(resized.data, resized.cols, resized.rows,
                      static_cast<int>(resized.step), QImage::Format_RGB888).copy();
    } else {
        cv::Mat rgb;
        cv::cvtColor(resized, rgb, cv::COLOR_GRAY2RGB);
        qImg = QImage(rgb.data, rgb.cols, rgb.rows,
                      static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
    }

    return QPixmap::fromImage(qImg);
}

void ComparisonGrid::generateThumbnails() {
    // Clear existing
    QLayoutItem* item;
    while ((item = gridLayout_->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    // Determine grid columns based on dialog width
    int cols = 4;
    int row = 0, col = 0;

    // Generate a thumbnail for each preset
    // Use a small preview image for speed
    Image previewSource = sourceImage_.downsampledForPreview(400000);

    for (const auto& presetId : presetIds_) {
        // Apply filter
        FilterParameters params;
        params.intensity = 100.0f;
        Image result = gradingModule_.applyPreset(previewSource, presetId, params);
        QPixmap thumb = createThumbnail(result, 200);

        // Create clickable frame
        auto* frame = new QFrame;
        frame->setFrameStyle(QFrame::Box);
        frame->setLineWidth(2);
        frame->setStyleSheet(
            "QFrame { border: 2px solid #444; border-radius: 4px; background: #2a2a2a; }"
            "QFrame:hover { border: 2px solid #4a7dff; }");
        frame->setCursor(Qt::PointingHandCursor);
        frame->setFixedSize(220, 260);

        auto* frameLayout = new QVBoxLayout(frame);
        frameLayout->setContentsMargins(8, 8, 8, 8);

        auto* imgLabel = new QLabel;
        imgLabel->setPixmap(thumb);
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setFixedSize(200, 200);
        frameLayout->addWidget(imgLabel);

        const PresetInfo* info = gradingModule_.getPresetInfo(presetId);
        QString name = info ? QString::fromStdString(info->name) : QString::fromStdString(presetId);

        auto* nameLabel = new QLabel(name);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setStyleSheet("font-weight: bold; font-size: 11px; color: #ddd;");
        nameLabel->setWordWrap(true);
        frameLayout->addWidget(nameLabel);

        // Category label
        if (info) {
            QString catStr;
            QColor catColor;
            switch (info->category) {
                case FilterCategory::Cinematic:
                    catStr = "Cinematic"; catColor = QColor("#ff9800"); break;
                case FilterCategory::JapanStyle:
                    catStr = "Japan Style"; catColor = QColor("#e91e63"); break;
                case FilterCategory::VintageRetro:
                    catStr = "Vintage"; catColor = QColor("#8bc34a"); break;
                case FilterCategory::Monochrome:
                    catStr = "Mono"; catColor = QColor("#9e9e9e"); break;
                default:
                    catStr = "Other"; catColor = QColor("#03a9f4"); break;
            }
            auto* catLabel = new QLabel(catStr);
            catLabel->setAlignment(Qt::AlignCenter);
            catLabel->setStyleSheet(
                QString("font-size: 9px; color: %1;").arg(catColor.name()));
            frameLayout->addWidget(catLabel);
        }

        // Make the frame clickable
        std::string capturedId = presetId;
        frame->installEventFilter(this);

        // Use a signal mapper approach with lambda
        auto* clickHandler = new QPushButton(frame);
        clickHandler->setFlat(true);
        clickHandler->setStyleSheet("QPushButton { border: none; background: transparent; }");
        clickHandler->setFixedSize(220, 260);
        clickHandler->setCursor(Qt::PointingHandCursor);

        // Re-parent labels to the button so it's clickable
        // Actually, use a simpler approach: overlay transparent button
        connect(clickHandler, &QPushButton::clicked, this, [this, capturedId]() {
            selectedId_ = capturedId;
            emit presetChosen(capturedId);
            accept();
        });

        gridLayout_->addWidget(frame, row, col);

        col++;
        if (col >= cols) {
            col = 0;
            row++;
        }
    }
}

} // namespace PixelForge