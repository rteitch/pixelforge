#include <gtest/gtest.h>
#include "app/HistoryManager.h"
#include "core/Image.h"

using namespace PixelForge;

TEST(HistoryManager, EmptyHistory) {
    HistoryManager history;
    EXPECT_FALSE(history.canUndo());
    EXPECT_FALSE(history.canRedo());
    EXPECT_EQ(history.historyCount(), 0);
    EXPECT_EQ(history.currentPosition(), -1);
}

TEST(HistoryManager, PushSingleState) {
    HistoryManager history;
    Image img = Image::empty(10, 10, 3);
    history.pushState(img, "Initial");

    EXPECT_EQ(history.historyCount(), 1);
    EXPECT_EQ(history.currentPosition(), 0);
    EXPECT_FALSE(history.canUndo());
    EXPECT_FALSE(history.canRedo());
    EXPECT_EQ(history.currentDescription(), "Initial");
}

TEST(HistoryManager, PushMultipleStates) {
    HistoryManager history;
    Image img1 = Image::empty(10, 10, 3);
    Image img2 = Image::empty(20, 20, 3);
    Image img3 = Image::empty(30, 30, 3);

    history.pushState(img1, "State 1");
    history.pushState(img2, "State 2");
    history.pushState(img3, "State 3");

    EXPECT_EQ(history.historyCount(), 3);
    EXPECT_EQ(history.currentPosition(), 2);
    EXPECT_TRUE(history.canUndo());
    EXPECT_FALSE(history.canRedo());
}

TEST(HistoryManager, UndoRedo) {
    HistoryManager history;
    Image img1 = Image::empty(10, 10, 3);
    Image img2 = Image::empty(20, 20, 3);

    history.pushState(img1, "State 1");
    history.pushState(img2, "State 2");

    // Undo
    Image undone = history.undo();
    EXPECT_FALSE(undone.isEmpty());
    EXPECT_EQ(history.currentPosition(), 0);
    EXPECT_EQ(history.currentDescription(), "State 1");
    EXPECT_TRUE(history.canRedo());

    // Redo
    Image redone = history.redo();
    EXPECT_FALSE(redone.isEmpty());
    EXPECT_EQ(history.currentPosition(), 1);
    EXPECT_EQ(history.currentDescription(), "State 2");
    EXPECT_FALSE(history.canRedo());
}

TEST(HistoryManager, UndoClearsRedoStack) {
    HistoryManager history;
    Image img1 = Image::empty(10, 10, 3);
    Image img2 = Image::empty(20, 20, 3);
    Image img3 = Image::empty(30, 30, 3);

    history.pushState(img1, "State 1");
    history.pushState(img2, "State 2");
    history.undo();
    history.pushState(img3, "State 3"); // Should clear redo of State 2

    EXPECT_EQ(history.historyCount(), 2); // State 1, State 3
    EXPECT_FALSE(history.canRedo());
}

TEST(HistoryManager, MaxHistory) {
    HistoryManager history;
    history.setMaxHistory(3);

    for (int i = 0; i < 5; ++i) {
        Image img = Image::empty(10, 10, 3);
        history.pushState(img, "State " + std::to_string(i));
    }

    EXPECT_LE(history.historyCount(), 3);
}

TEST(HistoryManager, Clear) {
    HistoryManager history;
    Image img = Image::empty(10, 10, 3);
    history.pushState(img, "Test");
    history.clear();

    EXPECT_EQ(history.historyCount(), 0);
    EXPECT_EQ(history.currentPosition(), -1);
}

TEST(HistoryManager, DescriptionAt) {
    HistoryManager history;
    Image img = Image::empty(10, 10, 3);
    history.pushState(img, "First");
    history.pushState(img, "Second");

    EXPECT_EQ(history.descriptionAt(0), "First");
    EXPECT_EQ(history.descriptionAt(1), "Second");
    EXPECT_EQ(history.descriptionAt(2), ""); // Out of range
}