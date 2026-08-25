#pragma once

#include "core/CoreTypes.h"
#include "core/Image.h"
#include "core/WapModule.h"
#include "core/ColorGradingModule.h"

#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

namespace PixelForge {

/// Batch processing engine that applies a preset to multiple images
/// using multi-threaded parallel processing.
class BatchProcessor {
public:
    BatchProcessor();
    ~BatchProcessor();

    /// Start batch processing
    /// @param config Batch job configuration
    /// @param progress Per-file progress callback (0-1 range, file index)
    /// @param fileProgress Per-file internal progress callback
    /// @return Vector of results for each file
    std::vector<BatchJobResult> process(
        const BatchJobConfig& config,
        std::function<void(int fileIndex, int totalFiles, const std::string& fileName)> progress = nullptr,
        ProgressCallback fileProgress = nullptr
    );

    /// Cancel the current batch job
    void cancel();

    /// Check if a batch job is currently running
    bool isRunning() const;

    /// Get the number of completed files
    int completedCount() const;

    /// Get total file count
    int totalCount() const;

private:
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> running_{false};
    std::atomic<int> completed_{0};
    std::atomic<int> total_{0};
    std::mutex mutex_;

    WapModule wapModule_;
    ColorGradingModule gradingModule_;

    /// Process a single file
    BatchJobResult processSingle(
        const std::string& inputPath,
        const BatchJobConfig& config,
        ProgressCallback progress,
        int index
    );

    /// Generate output path from input path and config
    std::string generateOutputPath(
        const std::string& inputPath,
        const BatchJobConfig& config,
        int index
    );

    /// Process files in parallel using thread pool
    void processParallel(
        const BatchJobConfig& config,
        std::vector<BatchJobResult>& results,
        std::function<void(int, int, const std::string&)> progress,
        ProgressCallback fileProgress
    );
};

} // namespace PixelForge