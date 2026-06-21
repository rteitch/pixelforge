#pragma once

#include "CoreTypes.h"
#include "Image.h"

#include <string>
#include <vector>
#include <memory>

namespace PixelForge {

/// AI Style Transfer module using ONNX Runtime.
/// Experimental v2.0 feature — applies neural style transfer
/// using lightweight ONNX models (runs on CPU).
///
/// This is a scaffold for future development. The actual ONNX Runtime
/// integration requires the onnxruntime library to be installed.
class AiStyleModule {
public:
    AiStyleModule();
    ~AiStyleModule();

    /// Available AI style model info
    struct AiStyleModel {
        std::string id;
        std::string name;
        std::string modelPath;    // Path to .onnx model file
        std::string description;
        int inputSize = 256;      // Model expects this input dimension
        bool loaded = false;
    };

    /// Load an ONNX model for style transfer
    /// @param modelPath Path to .onnx file
    /// @param modelId Unique identifier for this model
    /// @return true if model loaded successfully
    bool loadModel(const std::string& modelPath, const std::string& modelId);

    /// Check if ONNX Runtime is available
    static bool isOnnxRuntimeAvailable();

    /// Get list of available/recommended style models
    static std::vector<AiStyleModel> availableModels();

    /// Apply AI style transfer
    /// @param input Source image
    /// @param modelId Which loaded model to use
    /// @param strength Blend strength (0.0-1.0, 1.0 = full effect)
    /// @param progress Progress callback
    /// @return Styled image
    Image applyStyle(const Image& input,
                     const std::string& modelId,
                     float strength = 1.0f,
                     ProgressCallback progress = nullptr);

    /// Get loaded model IDs
    std::vector<std::string> loadedModelIds() const;

    /// Unload a model to free memory
    void unloadModel(const std::string& modelId);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    /// Preprocess image for model input (resize, normalize)
    Image preprocess(const Image& input, int targetSize);

    /// Postprocess model output back to image
    Image postprocess(const std::vector<float>& output, int width, int height);

    /// Run ONNX inference (stub — returns original if ONNX not available)
    std::vector<float> runInference(const std::string& modelId,
                                     const std::vector<float>& inputTensor,
                                     int inputH, int inputW);
};

} // namespace PixelForge