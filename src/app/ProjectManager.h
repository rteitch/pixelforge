#pragma once

#include "core/CoreTypes.h"
#include "core/Image.h"
#include "HistoryManager.h"

#include <string>
#include <memory>

namespace PixelForge {

/// Manages the current editing session: source image, adjustment layers,
/// project save/load (.pforge format).
class ProjectManager {
public:
    ProjectManager();
    ~ProjectManager();

    // ---- Image Management ----
    bool openImage(const std::string& filePath);
    void closeImage();
    bool hasImage() const;
    const Image& sourceImage() const;
    const Image& currentImage() const;
    void setCurrentImage(const Image& image, const std::string& description);

    // ---- Adjustment Layers ----
    void addLayer(const AdjustmentLayer& layer);
    void removeLayer(const std::string& layerId);
    void toggleLayer(const std::string& layerId, bool enabled);
    void updateLayerParams(const std::string& layerId, const FilterParameters& params);
    void reorderLayer(const std::string& layerId, int newIndex);
    const std::vector<AdjustmentLayer>& layers() const;
    std::vector<AdjustmentLayer>& layers();
    AdjustmentLayer* findLayer(const std::string& layerId);
    void clearLayers();

    // ---- History ----
    HistoryManager& history();
    const HistoryManager& history() const;

    // ---- Project Save/Load ----
    bool saveProject(const std::string& filePath);
    bool loadProject(const std::string& filePath);
    bool hasUnsavedChanges() const;
    void markSaved();

    // ---- Export ----
    bool exportImage(const std::string& filePath, int jpegQuality = 95);

    // ---- File paths ----
    const std::string& sourcePath() const;
    const std::string& projectPath() const;
    std::string suggestedExportName(const std::string& presetName = "") const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    static std::string generateLayerId();
};

} // namespace PixelForge