#pragma once

#include "CoreTypes.h"
#include "Image.h"

#include <string>
#include <vector>

namespace PixelForge {

/// Abstract interface for PixelForge filter plugins.
/// External dynamic libraries (.dll/.so/.dylib) must implement this interface.
///
/// Example plugin (myfilter.cpp):
///   class MyFilter : public IPlugin { ... };
///   PIXELFORGE_PLUGIN_EXPORT IPlugin* createPlugin() { return new MyFilter(); }
///   PIXELFORGE_PLUGIN_EXPORT void destroyPlugin(IPlugin* p) { delete p; }
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /// Unique plugin identifier (e.g., "com.example.myfilter")
    virtual const char* id() const = 0;

    /// Human-readable plugin name
    virtual const char* name() const = 0;

    /// Plugin version string
    virtual const char* version() const = 0;

    /// Plugin description
    virtual const char* description() const = 0;

    /// Plugin author
    virtual const char* author() const = 0;

    /// Category (e.g., "Cinematic", "Artistic", "Utility")
    virtual const char* category() const = 0;

    /// Apply the filter to an image
    /// @param input Source image (RGB, 8-bit)
    /// @param params Filter parameters (key-value pairs as strings)
    /// @return Processed image
    virtual Image apply(const Image& input,
                        const std::unordered_map<std::string, std::string>& params) = 0;

    /// Get list of parameter names this plugin accepts
    virtual std::vector<std::string> parameterNames() const = 0;

    /// Get default value for a parameter
    virtual std::string parameterDefault(const std::string& name) const = 0;

    /// Get description for a parameter
    virtual std::string parameterDescription(const std::string& name) const = 0;
};

// Plugin entry point function signatures
using CreatePluginFunc = IPlugin* (*)();
using DestroyPluginFunc = void (*)(IPlugin*);

} // namespace PixelForge

// Export macros for plugin authors
#ifdef _WIN32
    #define PIXELFORGE_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define PIXELFORGE_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif