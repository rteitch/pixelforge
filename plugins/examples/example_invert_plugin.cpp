/// Example PixelForge Plugin: Color Invert
/// 
/// Build instructions:
///   mkdir build && cd build
///   cmake -DCMAKE_BUILD_TYPE=Release ..
///   cmake --build .
///
/// The resulting .dll/.so/.dylib should be placed in the plugins/ directory.

#include "../../src/core/PluginInterface.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

using namespace PixelForge;

class InvertPlugin : public IPlugin {
public:
    const char* id() const override { return "com.pixelforge.example.invert"; }
    const char* name() const override { return "Color Invert"; }
    const char* version() const override { return "1.0.0"; }
    const char* description() const override { return "Inverts all colors in the image. Example plugin for PixelForge."; }
    const char* author() const override { return "PixelForge Team"; }
    const char* category() const override { return "Utility"; }

    Image apply(const Image& input,
                const std::unordered_map<std::string, std::string>& params) override {
        if (input.isEmpty()) return input;

        Image result = input.deepCopy();
        cv::Mat& mat = result.mat();
        int total = mat.rows * mat.cols;
        int ch = mat.channels();

        float strength = 1.0f;
        auto it = params.find("strength");
        if (it != params.end()) {
            try { strength = std::stof(it->second); } catch (...) {}
        }
        strength = std::clamp(strength, 0.0f, 1.0f);

        for (int i = 0; i < total; ++i) {
            uint8_t* p = mat.data + i * ch;
            for (int c = 0; c < std::min(ch, 3); ++c) {
                uint8_t inverted = 255 - p[c];
                p[c] = static_cast<uint8_t>(p[c] * (1.0f - strength) + inverted * strength);
            }
        }

        return result;
    }

    std::vector<std::string> parameterNames() const override {
        return {"strength"};
    }

    std::string parameterDefault(const std::string& name) const override {
        if (name == "strength") return "1.0";
        return "";
    }

    std::string parameterDescription(const std::string& name) const override {
        if (name == "strength") return "Inversion strength (0.0 to 1.0)";
        return "";
    }
};

// Plugin entry points
PIXELFORGE_PLUGIN_EXPORT IPlugin* createPlugin() {
    return new InvertPlugin();
}

PIXELFORGE_PLUGIN_EXPORT void destroyPlugin(IPlugin* p) {
    delete p;
}