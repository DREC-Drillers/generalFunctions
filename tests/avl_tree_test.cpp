#include <array>
#include <utility>

#include <gtest/gtest.h>

#include "tree/avlTree.hpp"

TEST(AVLTreeTest, InsertAndDuplicate) {
    general_functions::AVLTree tree;
    EXPECT_EQ(tree.size(), 0U);
    EXPECT_FALSE(tree.tree().root());
    EXPECT_TRUE(tree.insert(5));
    EXPECT_FALSE(tree.insert(5));
    EXPECT_EQ(tree.size(), 1U);
    EXPECT_EQ(tree.tree().value(*tree.tree().root()).key, 5);
}

TEST(AVLTreeTest, BalancesAllFourRotationCases) {
    for (const auto input : {std::array{3, 2, 1},
                             std::array{1, 2, 3},
                             std::array{3, 1, 2},
                             std::array{1, 3, 2}}) {
        general_functions::AVLTree tree;
        for (int key : input) {
            tree.insert(key);
        }
        const auto& binary = tree.tree();
        const auto root = *binary.root();
        EXPECT_EQ(binary.value(root).key, 2);
        EXPECT_EQ(binary.value(*binary.left(root)).key, 1);
        EXPECT_EQ(binary.value(*binary.right(root)).key, 3);
    }
}

TEST(AVLTreeTest, CopyAndMove) {
    general_functions::AVLTree original;
    original.insert(5);
    auto copy = original;
    copy.insert(3);
    EXPECT_EQ(original.size(), 1U);
    EXPECT_EQ(copy.size(), 2U);
    auto moved = std::move(copy);
    EXPECT_EQ(moved.size(), 2U);
    EXPECT_EQ(copy.size(), 0U);
    EXPECT_TRUE(copy.insert(9));
}
