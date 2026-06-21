#include "AiStyleModule.h"

#include <opencv2/imgproc.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_map>

// ONNX Runtime integration (conditional compile)
// When onnxruntime is available, define PIXELFORGE_HAS_ONNX
#ifdef PIXELFORGE_HAS_ONNX
#include <onnxruntime_cxx_api.h>
#endif

namespace PixelForge {

namespace fs = std::filesystem;

struct AiStyleModule::Impl {
    struct ModelEntry {
        AiStyleModel info;
#ifdef PIXELFORGE_HAS_ONNX
        std::unique_ptr<Ort::Session> session;
        Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "PixelForge"};
#endif
    };

    std::unordered_map<std::string, ModelEntry> models;
};

AiStyleModule::AiStyleModule() : impl_(std::make_unique<Impl>()) {}
AiStyleModule::~AiStyleModule() = default;

// ============================================================
// Static helpers
// ============================================================

bool AiStyleModule::isOnnxRuntimeAvailable() {
#ifdef PIXELFORGE_HAS_ONNX
    return true;
#else
    return false;
#endif
}

std::vector<AiStyleModule::AiStyleModel> AiStyleModule::availableModels() {
    // List of recommended pretrained style transfer models
    // These would be downloaded separately by the user
    return {
        {
            "mosaic",
            "Mosaic Style",
            "models/mosaic.onnx",
            "Mosaic art style transfer (fast, lightweight)",
            256, false
        },
        {
            "candy",
            "Candy Style",
            "models/candy.onnx",
            "Candy/neon art style transfer",
            256, false
        },
        {
            "starry_night",
            "Starry Night",
            "models/starry_night.onnx",
            "Van Gogh's Starry Night painting style",
            512, false
        },
        {
            "the_scream",
            "The Scream",
            "models/the_scream.onnx",
            "Edvard Munch's The Scream style",
            512, false
        },
        {
            "udnie",
            "Udnie Style",
            "models/udnie.onnx",
            "Abstract art style (Francis Picabia)",
            256, false
        }
    };
}

// ============================================================
// Model Management
// ============================================================

bool AiStyleModule::loadModel(const std::string& modelPath, const std::string& modelId) {
    if (!fs::exists(modelPath)) {
        std::cerr << "[AiStyle] Model file not found: " << modelPath << "\n";
        return false;
    }

#ifdef PIXELFORGE_HAS_ONNX
    try {
        Impl::ModelEntry entry;
        entry.info.id = modelId;
        entry.info.modelPath = modelPath;
        entry.info.loaded = true;

        // Try to determine input size from filename or use default
        entry.info.inputSize = 256;

        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(4);
        sessionOptions.SetGraphOptimizationLevel(
            GraphOptimizationLevel::ORT_ENABLE_ALL);

        entry.session = std::make_unique<Ort::Session>(
            impl_->env, modelPath.c_str(), sessionOptions);

        impl_->models[modelId] = std::move(entry);
        std::cout << "[AiStyle] Loaded model: " << modelId
                  << " from " << modelPath << "\n";
        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "[AiStyle] ONNX error loading model: " << e.what() << "\n";
        return false;
    }
#else
    // ONNX Runtime not available — store info but mark as not loaded
    Impl::ModelEntry entry;
    entry.info.id = modelId;
    entry.info.modelPath = modelPath;
    entry.info.loaded = false;
    impl_->models[modelId] = std::move(entry);

    std::cerr << "[AiStyle] ONNX Runtime not available. "
              << "Model '" << modelId << "' registered but cannot run inference. "
              << "Build with -DPIXELFORGE_HAS_ONNX=ON to enable AI style transfer.\n";
    return false;
#endif
}

std::vector<std::string> AiStyleModule::loadedModelIds() const {
    std::vector<std::string> ids;
    for (const auto& [id, entry] : impl_->models) {
#ifdef PIXELFORGE_HAS_ONNX
        if (entry.session) ids.push_back(id);
#else
        // Without ONNX, report none as truly loaded
#endif
    }
    return ids;
}

void AiStyleModule::unloadModel(const std::string& modelId) {
    impl_->models.erase(modelId);
}

// ============================================================
// Preprocessing / Postprocessing
// ============================================================

Image AiStyleModule::preprocess(const Image& input, int targetSize) {
    if (input.isEmpty()) return input;

    // Resize to model's expected input size
    Image resized = input.resized(targetSize, targetSize, cv::INTER_LINEAR);
    return resized;
}

Image AiStyleModule::postprocess(const std::vector<float>& output,
                                  int width, int height) {
    // Convert normalized float output back to uint8 RGB image
    cv::Mat result(height, width, CV_8UC3);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 3;
            uint8_t r = static_cast<uint8_t>(
                std::clamp((output[idx]     * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
            uint8_t g = static_cast<uint8_t>(
                std::clamp((output[idx + 1] * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
            uint8_t b = static_cast<uint8_t>(
                std::clamp((output[idx + 2] * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));

            uint8_t* p = result.ptr<uint8_t>(y) + x * 3;
            p[0] = r;
            p[1] = g;
            p[2] = b;
        }
    }

    return Image(result);
}

// ============================================================
// Inference
// ============================================================

std::vector<float> AiStyleModule::runInference(
    const std::string& modelId,
    const std::vector<float>& inputTensor,
    int inputH, int inputW)
{
#ifdef PIXELFORGE_HAS_ONNX
    auto it = impl_->models.find(modelId);
    if (it == impl_->models.end() || !it->second.session) {
        std::cerr << "[AiStyle] Model not loaded: " << modelId << "\n";
        return inputTensor;
    }

    try {
        auto& session = *it->second.session;

        // Create input tensor
        std::vector<int64_t> inputShape = {1, 3, inputH, inputW};
        Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(
            OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

        Ort::Value inputOrtTensor = Ort::Value::CreateTensor<float>(
            memInfo, const_cast<float*>(inputTensor.data()),
            inputTensor.size(), inputShape.data(), inputShape.size());

        // Get input/output names
        auto inputName = session.GetInputNameAllocated(0, Ort::AllocatorWithDefaultAllocator());
        auto outputName = session.GetOutputNameAllocated(0, Ort::AllocatorWithDefaultAllocator());

        const char* inputNames[] = {inputName.get()};
        const char* outputNames[] = {outputName.get()};

        // Run inference
        auto outputTensors = session.Run(
            Ort::RunOptions{nullptr},
            inputNames, &inputOrtTensor, 1,
            outputNames, 1);

        // Extract output
        auto& outputTensor = outputTensors[0];
        auto outputShape = outputTensor.GetTensorTypeAndShapeInfo().GetShape();
        float* outputData = outputTensor.GetTensorMutableData<float>();

        size_t outputSize = 1;
        for (auto dim : outputShape) outputSize *= dim;

        return std::vector<float>(outputData, outputData + outputSize);

    } catch (const Ort::Exception& e) {
        std::cerr << "[AiStyle] Inference error: " << e.what() << "\n";
        return inputTensor;
    }
#else
    // No ONNX Runtime — return input unchanged
    return inputTensor;
#endif
}

// ============================================================
// Main Style Application
// ============================================================

Image AiStyleModule::applyStyle(
    const Image& input,
    const std::string& modelId,
    float strength,
    ProgressCallback progress)
{
    if (input.isEmpty()) return input;

    if (progress) progress(0.1f, "Checking model...");

#ifdef PIXELFORGE_HAS_ONNX
    auto it = impl_->models.find(modelId);
    if (it == impl_->models.end() || !it->second.session) {
        std::cerr << "[AiStyle] Model not loaded: " << modelId << "\n";
        return input.deepCopy();
    }

    int inputSize = it->second.info.inputSize;

    if (progress) progress(0.2f, "Preprocessing...");
    Image processed = preprocess(input, inputSize);
    const cv::Mat& mat = processed.mat();
    int h = mat.rows, w = mat.cols;

    // Convert to float tensor [1, 3, H, W], normalized to [-1, 1]
    std::vector<float> tensor(3 * h * w);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const uint8_t* p = mat.ptr<uint8_t>(y) + x * 3;
            tensor[0 * h * w + y * w + x] = (p[0] / 255.0f - 0.5f) * 2.0f;
            tensor[1 * h * w + y * w + x] = (p[1] / 255.0f - 0.5f) * 2.0f;
            tensor[2 * h * w + y * w + x] = (p[2] / 255.0f - 0.5f) * 2.0f;
        }
    }

    if (progress) progress(0.4f, "Running AI inference...");
    auto output = runInference(modelId, tensor, h, w);

    if (progress) progress(0.8f, "Postprocessing...");
    Image styled = postprocess(output, w, h);

    // Resize back to original dimensions
    styled = styled.resized(input.width(), input.height(), cv::INTER_LINEAR);

    // Blend with original based on strength
    if (strength < 1.0f) {
        const cv::Mat& origMat = input.mat();
        cv::Mat& styledMat = styled.mat();
        cv::Mat blended;
        cv::addWeighted(origMat, 1.0 - strength, styledMat, strength, 0, blended);
        styled = Image(blended);
    }

    if (progress) progress(1.0f, "Done");
    return styled;
#else
    // ONNX Runtime not available — return original with a message
    (void)progress; // suppress unused warning
    std::cerr << "[AiStyle] ONNX Runtime not available. Cannot apply AI style.\n"
              << "Build with -DPIXELFORGE_HAS_ONNX=ON to enable this feature.\n";
    return input.deepCopy();
#endif
}

} // namespace PixelForge