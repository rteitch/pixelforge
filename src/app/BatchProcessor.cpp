#include "BatchProcessor.h"
#include "core/IoModule.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <sstream>

namespace PixelForge {

namespace fs = std::filesystem;

BatchProcessor::BatchProcessor() = default;
BatchProcessor::~BatchProcessor() = default;

// ============================================================
// Public Interface
// ============================================================

std::vector<BatchJobResult> BatchProcessor::process(
    const BatchJobConfig& config,
    std::function<void(int, int, const std::string&)> progress,
    ProgressCallback fileProgress)
{
    if (config.inputPaths.empty() || config.outputDirectory.empty()) return {};

    cancelled_ = false;
    running_ = true;
    completed_ = 0;
    total_ = static_cast<int>(config.inputPaths.size());

    // Ensure output directory exists
    std::error_code ec;
    fs::create_directories(config.outputDirectory, ec);

    std::vector<BatchJobResult> results(total_);

    // Initialize results
    for (int i = 0; i < total_; ++i) {
        results[i].inputPath = config.inputPaths[i];
        results[i].status = BatchJobStatus::Pending;
    }

    processParallel(config, results, progress, fileProgress);

    running_ = false;
    return results;
}

void BatchProcessor::cancel() {
    cancelled_ = true;
}

bool BatchProcessor::isRunning() const {
    return running_;
}

int BatchProcessor::completedCount() const {
    return completed_;
}

int BatchProcessor::totalCount() const {
    return total_;
}

// ============================================================
// Parallel Processing
// ============================================================

void BatchProcessor::processParallel(
    const BatchJobConfig& config,
    std::vector<BatchJobResult>& results,
    std::function<void(int, int, const std::string&)> progress,
    ProgressCallback fileProgress)
{
    unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    numThreads = std::min(numThreads, static_cast<unsigned int>(config.inputPaths.size()));

    if (numThreads <= 1) {
        // Single-threaded fallback
        for (int i = 0; i < total_; ++i) {
            if (cancelled_) {
                results[i].status = BatchJobStatus::Cancelled;
                continue;
            }

            if (progress) progress(i, total_, config.inputPaths[i]);
            results[i] = processSingle(config.inputPaths[i], config, fileProgress);
            completed_ = i + 1;
        }
        return;
    }

    // Multi-threaded: distribute files across threads
    std::atomic<int> fileIndex{0};

    auto worker = [&]() {
        while (!cancelled_) {
            int idx = fileIndex.fetch_add(1);
            if (idx >= total_) break;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (progress) progress(idx, total_, config.inputPaths[idx]);
            }

            results[idx] = processSingle(config.inputPaths[idx], config, fileProgress);
            completed_ = idx + 1;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (unsigned int t = 0; t < numThreads; ++t) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}

// ============================================================
// Single File Processing
// ============================================================

BatchJobResult BatchProcessor::processSingle(
    const std::string& inputPath,
    const BatchJobConfig& config,
    ProgressCallback progress)
{
    BatchJobResult result;
    result.inputPath = inputPath;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Load image
        if (progress) progress(0.1f, "Loading...");
        Image input = IoModule::loadImage(inputPath);

        // Check resolution limit
        if (!IoModule::checkResolutionLimit(input.width(), input.height(), 8000)) {
            input = input.resizedToFit(8000);
        }

        Image output;

        // Apply preset
        if (progress) progress(0.3f, "Processing...");
        if (config.useWapMode) {
            output = wapModule_.generate(input, config.wapParams, progress);
        } else {
            output = gradingModule_.applyPreset(input, config.presetId, config.filterParams, progress);
        }

        // Generate output path
        result.outputPath = generateOutputPath(inputPath, config,
                                                static_cast<int>(&result - &*(&result)[0]));

        // Fix index in output path name
        std::string namePattern = config.namingPattern;
        fs::path inputP(inputPath);
        std::string baseName = inputP.stem().string();

        // Simple pattern substitution
        auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
            size_t pos = 0;
            while ((pos = str.find(from, pos)) != std::string::npos) {
                str.replace(pos, from.length(), to);
                pos += to.length();
            }
        };

        replaceAll(namePattern, "{name}", baseName);
        replaceAll(namePattern, "{preset}", config.presetId.empty() ? "wap" : config.presetId);

        std::string ext = config.outputFormat;
        if (ext[0] != '.') ext = "." + ext;

        result.outputPath = (fs::path(config.outputDirectory) / (namePattern + ext)).string();

        // Save
        if (progress) progress(0.9f, "Saving...");
        int quality = config.jpegQuality;
        bool saved = output.save(result.outputPath, quality);

        if (saved) {
            result.status = BatchJobStatus::Completed;
        } else {
            result.status = BatchJobStatus::Failed;
            result.errorMessage = "Failed to save output file";
        }

    } catch (const std::exception& e) {
        result.status = BatchJobStatus::Failed;
        result.errorMessage = e.what();
    } catch (...) {
        result.status = BatchJobStatus::Failed;
        result.errorMessage = "Unknown error during processing";
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.processingTimeMs = std::chrono::duration<double, std::milli>(
        endTime - startTime).count();

    return result;
}

// ============================================================
// Output Path Generation
// ============================================================

std::string BatchProcessor::generateOutputPath(
    const std::string& inputPath,
    const BatchJobConfig& config,
    int index)
{
    fs::path inputP(inputPath);
    std::string baseName = inputP.stem().string();

    std::string namePattern = config.namingPattern;

    auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    replaceAll(namePattern, "{name}", baseName);
    replaceAll(namePattern, "{preset}", config.presetId.empty() ? "wap" : config.presetId);
    replaceAll(namePattern, "{index}", std::to_string(index));

    std::string ext = config.outputFormat;
    if (ext[0] != '.') ext = "." + ext;

    return (fs::path(config.outputDirectory) / (namePattern + ext)).string();
}

} // namespace PixelForge