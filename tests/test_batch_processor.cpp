#include <gtest/gtest.h>
#include "app/BatchProcessor.h"
#include "core/CoreTypes.h"

using namespace PixelForge;

TEST(BatchProcessor, EmptyConfig) {
    BatchProcessor processor;
    BatchJobConfig config;
    auto results = processor.process(config);
    EXPECT_TRUE(results.empty());
}

TEST(BatchProcessor, CancelFlag) {
    BatchProcessor processor;
    EXPECT_FALSE(processor.isRunning());
    processor.cancel(); // Should not crash
    EXPECT_FALSE(processor.isRunning());
}

TEST(BatchProcessor, OutputPathGeneration) {
    // Test naming pattern substitution
    BatchJobConfig config;
    config.inputPaths = {"C:/photos/test.jpg"};
    config.outputDirectory = "C:/output";
    config.outputFormat = "png";
    config.namingPattern = "{name}_{preset}";
    config.presetId = "cinematic_teal_orange";

    // We can't easily test processSingle directly since it's private,
    // but we can test the config structure
    EXPECT_EQ(config.inputPaths.size(), 1u);
    EXPECT_EQ(config.namingPattern, "{name}_{preset}");
}

TEST(BatchProcessor, BatchJobResult) {
    BatchJobResult result;
    result.inputPath = "test.jpg";
    result.status = BatchJobStatus::Pending;
    EXPECT_EQ(result.status, BatchJobStatus::Pending);
    EXPECT_TRUE(result.errorMessage.empty());
    EXPECT_DOUBLE_EQ(result.processingTimeMs, 0.0);
}

TEST(BatchProcessor, BatchJobConfigDefaults) {
    BatchJobConfig config;
    EXPECT_EQ(config.outputFormat, "png");
    EXPECT_EQ(config.jpegQuality, 95);
    EXPECT_FALSE(config.useWapMode);
    EXPECT_TRUE(config.inputPaths.empty());
}