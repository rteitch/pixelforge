#pragma once

#include "core/CoreTypes.h"
#include "core/Image.h"

#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace PixelForge {

/// Manages undo/redo history for non-destructive editing.
/// Each state stores a snapshot of the image and the associated command name.
class HistoryManager {
public:
    HistoryManager();
    ~HistoryManager();

    /// Push a new state onto the history stack.
    /// Clears any redo states that were ahead.
    void pushState(const Image& image, const std::string& description);

    /// Undo the last action. Returns the previous state image.
    /// Returns empty image if cannot undo.
    Image undo();

    /// Redo the last undone action. Returns the next state image.
    /// Returns empty image if cannot redo.
    Image redo();

    /// Check if undo is available
    bool canUndo() const;

    /// Check if redo is available
    bool canRedo() const;

    /// Get the current state image
    const Image& currentState() const;

    /// Get the current description
    std::string currentDescription() const;

    /// Get total history depth
    int historyCount() const;

    /// Get current position in history (0-based)
    int currentPosition() const;

    /// Get description at a specific index
    std::string descriptionAt(int index) const;

    /// Clear all history
    void clear();

    /// Set maximum history depth (default 50)
    void setMaxHistory(int maxSteps);

    int maxHistory() const;

private:
    struct State {
        Image image;
        std::string description;
    };

    std::vector<State> states_;
    int currentIndex_ = -1;
    int maxHistory_ = 50;
};

} // namespace PixelForge