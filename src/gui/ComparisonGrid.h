#pragma once

#include "core/CoreTypes.h"
#include "core/Image.h"
#include "core/ColorGradingModule.h"

#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <vector>
#include <string>

namespace PixelForge {

/// Dialog that shows one image with multiple filter presets applied
/// in a grid layout for easy comparison.
class ComparisonGrid : public QDialog {
    Q_OBJECT

public:
    explicit ComparisonGrid(const Image& sourceImage,
                            ColorGradingModule& gradingModule,
                            QWidget* parent = nullptr);
    ~ComparisonGrid() override;

    /// Set which presets to compare
    void setPresetIds(const std::vector<std::string>& ids);

    /// Get the selected preset ID (after user clicks one)
    std::string selectedPresetId() const;

signals:
    void presetChosen(const std::string& presetId);

private:
    Image sourceImage_;
    ColorGradingModule& gradingModule_;
    std::vector<std::string> presetIds_;
    std::string selectedId_;

    QGridLayout* gridLayout_ = nullptr;
    QScrollArea* scrollArea_ = nullptr;

    void setupUi();
    void generateThumbnails();
    QPixmap createThumbnail(const Image& img, int maxSize = 200);
};

} // namespace PixelForge