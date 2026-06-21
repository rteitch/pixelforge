#pragma once

#include "PluginInterface.h"
#include "CoreTypes.h"
#include "Image.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace PixelForge {

/// Manages loading, unloading, and execution of filter plugins.
/// Plugins are dynamic libraries (.dll/.so/.dylib) that implement IPlugin.
class PluginManager {
public:
    PluginManager();
    ~PluginManager();

    /// Load a plugin from a dynamic library file
    /// @param filePath Path to the .dll/.so/.dylib file
    /// @return true if loaded successfully
    bool loadPlugin(const std::string& filePath);

    /// Load all plugins from a directory
    /// @param directoryPath Path to directory containing plugins
    /// @return Number of plugins successfully loaded
    int loadPluginsFromDirectory(const std::string& directoryPath);

    /// Unload a specific plugin by ID
    void unloadPlugin(const std::string& pluginId);

    /// Unload all plugins
    void unloadAll();

    /// Get list of all loaded plugin IDs
    std::vector<std::string> loadedPluginIds() const;

    /// Get plugin info by ID
    const IPlugin* getPlugin(const std::string& pluginId) const;

    /// Apply a plugin filter to an image
    Image applyPlugin(const std::string& pluginId,
                      const Image& input,
                      const std::unordered_map<std::string, std::string>& params = {});

    /// Get all plugin names and categories for UI display
    struct PluginInfo {
        std::string id;
        std::string name;
        std::string version;
        std::string description;
        std::string author;
        std::string category;
        std::vector<std::string> paramNames;
    };
    std::vector<PluginInfo> allPluginInfo() const;

    /// Check if any plugins are loaded
    bool hasPlugins() const;

    /// Get the default plugin search directory
    static std::string defaultPluginDirectory();

private:
    struct LoadedPlugin {
        void* handle = nullptr;       // Platform library handle
        IPlugin* instance = nullptr;
        DestroyPluginFunc destroyFunc = nullptr;
        std::string filePath;
    };

    std::unordered_map<std::string, LoadedPlugin> plugins_;

    // Platform-specific dynamic library functions
    void* loadLibrary(const std::string& path);
    void* getSymbol(void* handle, const char* symbol);
    void unloadLibrary(void* handle);
    std::string libraryExtension() const;
    std::string lastError() const;
};

} // namespace PixelForge