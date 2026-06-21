#include "HistoryManager.h"

#include <algorithm>

namespace PixelForge {

HistoryManager::HistoryManager() = default;
HistoryManager::~HistoryManager() = default;

void HistoryManager::pushState(const Image& image, const std::string& description) {
    // Remove any states ahead of current index (redo stack)
    if (currentIndex_ < static_cast<int>(states_.size()) - 1) {
        states_.erase(states_.begin() + currentIndex_ + 1, states_.end());
    }

    // Add new state
    State state;
    state.image = image.deepCopy();
    state.description = description;
    states_.push_back(std::move(state));

    // Enforce max history
    if (static_cast<int>(states_.size()) > maxHistory_) {
        states_.erase(states_.begin());
    }

    currentIndex_ = static_cast<int>(states_.size()) - 1;
}

Image HistoryManager::undo() {
    if (!canUndo()) return Image();
    currentIndex_--;
    return states_[currentIndex_].image.deepCopy();
}

Image HistoryManager::redo() {
    if (!canRedo()) return Image();
    currentIndex_++;
    return states_[currentIndex_].image.deepCopy();
}

bool HistoryManager::canUndo() const {
    return currentIndex_ > 0;
}

bool HistoryManager::canRedo() const {
    return currentIndex_ < static_cast<int>(states_.size()) - 1;
}

const Image& HistoryManager::currentState() const {
    static const Image empty;
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(states_.size())) {
        return empty;
    }
    return states_[currentIndex_].image;
}

std::string HistoryManager::currentDescription() const {
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(states_.size())) {
        return "";
    }
    return states_[currentIndex_].description;
}

int HistoryManager::historyCount() const {
    return static_cast<int>(states_.size());
}

int HistoryManager::currentPosition() const {
    return currentIndex_;
}

std::string HistoryManager::descriptionAt(int index) const {
    if (index < 0 || index >= static_cast<int>(states_.size())) return "";
    return states_[index].description;
}

void HistoryManager::clear() {
    states_.clear();
    currentIndex_ = -1;
}

void HistoryManager::setMaxHistory(int maxSteps) {
    maxHistory_ = std::max(1, maxSteps);
    while (static_cast<int>(states_.size()) > maxHistory_) {
        states_.erase(states_.begin());
        currentIndex_--;
    }
}

int HistoryManager::maxHistory() const {
    return maxHistory_;
}

} // namespace PixelForge