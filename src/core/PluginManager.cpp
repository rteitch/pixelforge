#include "PluginManager.h"

#include <filesystem>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace PixelForge {

namespace fs = std::filesystem;

PluginManager::PluginManager() = default;

PluginManager::~PluginManager() {
    unloadAll();
}

// ============================================================
// Platform-specific dynamic library functions
// ============================================================

void* PluginManager::loadLibrary(const std::string& path) {
#ifdef _WIN32
    return static_cast<void*>(LoadLibraryA(path.c_str()));
#else
    return dlopen(path.c_str(), RTLD_LAZY);
#endif
}

void* PluginManager::getSymbol(void* handle, const char* symbol) {
#ifdef _WIN32
    return reinterpret_cast<void*>(
        GetProcAddress(static_cast<HMODULE>(handle), symbol));
#else
    return dlsym(handle, symbol);
#endif
}

void PluginManager::unloadLibrary(void* handle) {
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

std::string PluginManager::libraryExtension() const {
#ifdef _WIN32
    return ".dll";
#elif __APPLE__
    return ".dylib";
#else
    return ".so";
#endif
}

std::string PluginManager::lastError() const {
#ifdef _WIN32
    DWORD err = GetLastError();
    if (err == 0) return "No error";
    char buf[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, sizeof(buf), nullptr);
    return std::string(buf);
#else
    const char* err = dlerror();
    return err ? std::string(err) : "No error";
#endif
}

// ============================================================
// Plugin Loading
// ============================================================

bool PluginManager::loadPlugin(const std::string& filePath) {
    // Validate file exists
    if (!fs::exists(filePath)) {
        std::cerr << "[PluginManager] File not found: " << filePath << "\n";
        return false;
    }

    // Load dynamic library
    void* handle = loadLibrary(filePath);
    if (!handle) {
        std::cerr << "[PluginManager] Failed to load library: " << filePath
                  << " (" << lastError() << ")\n";
        return false;
    }

    // Get entry points
    auto createFunc = reinterpret_cast<CreatePluginFunc>(
        getSymbol(handle, "createPlugin"));
    auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(
        getSymbol(handle, "destroyPlugin"));

    if (!createFunc || !destroyFunc) {
        std::cerr << "[PluginManager] Invalid plugin (missing entry points): "
                  << filePath << "\n";
        unloadLibrary(handle);
        return false;
    }

    // Create plugin instance
    IPlugin* instance = createFunc();
    if (!instance) {
        std::cerr << "[PluginManager] Failed to create plugin instance: "
                  << filePath << "\n";
        unloadLibrary(handle);
        return false;
    }

    // Check for duplicate ID
    std::string id = instance->id();
    if (plugins_.find(id) != plugins_.end()) {
        std::cerr << "[PluginManager] Plugin already loaded: " << id << "\n";
        destroyFunc(instance);
        unloadLibrary(handle);
        return false;
    }

    // Register plugin
    LoadedPlugin loaded;
    loaded.handle = handle;
    loaded.instance = instance;
    loaded.destroyFunc = destroyFunc;
    loaded.filePath = filePath;
    plugins_[id] = loaded;

    std::cout << "[PluginManager] Loaded plugin: " << instance->name()
              << " v" << instance->version()
              << " (" << id << ")\n";
    return true;
}

int PluginManager::loadPluginsFromDirectory(const std::string& directoryPath) {
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        return 0;
    }

    int loaded = 0;
    std::string ext = libraryExtension();

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            std::string fileExt = entry.path().extension().string();
            
            // Case-insensitive extension comparison
            std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);
            
            if (fileExt == ext) {
                if (loadPlugin(filePath)) {
                    loaded++;
                }
            }
        }
    }

    return loaded;
}

void PluginManager::unloadPlugin(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) return;

    auto& loaded = it->second;
    if (loaded.instance && loaded.destroyFunc) {
        loaded.destroyFunc(loaded.instance);
    }
    if (loaded.handle) {
        unloadLibrary(loaded.handle);
    }

    plugins_.erase(it);
    std::cout << "[PluginManager] Unloaded plugin: " << pluginId << "\n";
}

void PluginManager::unloadAll() {
    for (auto& [id, loaded] : plugins_) {
        if (loaded.instance && loaded.destroyFunc) {
            loaded.destroyFunc(loaded.instance);
        }
        if (loaded.handle) {
            unloadLibrary(loaded.handle);
        }
    }
    plugins_.clear();
}

// ============================================================
// Plugin Access
// ============================================================

std::vector<std::string> PluginManager::loadedPluginIds() const {
    std::vector<std::string> ids;
    ids.reserve(plugins_.size());
    for (const auto& [id, _] : plugins_) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

const IPlugin* PluginManager::getPlugin(const std::string& pluginId) const {
    auto it = plugins_.find(pluginId);
    return (it != plugins_.end()) ? it->second.instance : nullptr;
}

Image PluginManager::applyPlugin(
    const std::string& pluginId,
    const Image& input,
    const std::unordered_map<std::string, std::string>& params)
{
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        std::cerr << "[PluginManager] Plugin not found: " << pluginId << "\n";
        return input.deepCopy();
    }

    try {
        return it->second.instance->apply(input, params);
    } catch (const std::exception& e) {
        std::cerr << "[PluginManager] Plugin error (" << pluginId << "): "
                  << e.what() << "\n";
        return input.deepCopy();
    }
}

std::vector<PluginManager::PluginInfo> PluginManager::allPluginInfo() const {
    std::vector<PluginInfo> info;
    info.reserve(plugins_.size());

    for (const auto& [id, loaded] : plugins_) {
        if (!loaded.instance) continue;

        PluginInfo pi;
        pi.id = loaded.instance->id();
        pi.name = loaded.instance->name();
        pi.version = loaded.instance->version();
        pi.description = loaded.instance->description();
        pi.author = loaded.instance->author();
        pi.category = loaded.instance->category();
        pi.paramNames = loaded.instance->parameterNames();
        info.push_back(pi);
    }

    return info;
}

bool PluginManager::hasPlugins() const {
    return !plugins_.empty();
}

std::string PluginManager::defaultPluginDirectory() {
    // Get executable directory and look for /plugins subfolder
    try {
        auto exePath = fs::current_path();
        auto pluginDir = exePath / "plugins";
        if (fs::exists(pluginDir)) {
            return pluginDir.string();
        }
    } catch (...) {}

    return "plugins";
}

} // namespace PixelForge