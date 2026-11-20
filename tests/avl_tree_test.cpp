#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "tree/avlTree.hpp"

namespace {

using NodeId = general_functions::AVLTree::NodeId;
constexpr auto NO_NODE = general_functions::BinaryTree<int>::INVALID_NODE;

struct ValidationResult {
    int height;
    std::size_t subtreeSize;
};

std::optional<ValidationResult> validateNode(
    const general_functions::AVLTree& avl,
    NodeId id, NodeId parent, std::optional<int> lower,
    std::optional<int> upper, std::vector<bool>& visited) {
    if (id == NO_NODE) {
        return ValidationResult{0, 0};
    }
    if (id >= avl.size() || visited[id]) {
        return std::nullopt;
    }
    visited[id] = true;
    const auto& link = avl.tree().getNode(id);
    if (link.parent() != parent) {
        return std::nullopt;
    }
    const auto& node = link.data();
    if ((lower && node.key <= *lower) ||
        (upper && node.key >= *upper)) {
        return std::nullopt;
    }
    const auto left = validateNode(avl, link.left(), id, lower, node.key, visited);
    if (!left) {
        return std::nullopt;
    }
    const auto right = validateNode(avl, link.right(), id, node.key, upper, visited);
    if (!right) {
        return std::nullopt;
    }
    const int expectedHeight = 1 + std::max(left->height, right->height);
    const auto expectedSize = 1 + left->subtreeSize + right->subtreeSize;
    const int balance = left->height - right->height;
    if (balance < -1 || balance > 1 || node.height != expectedHeight ||
        node.subtreeSize != expectedSize) {
        return std::nullopt;
    }
    return ValidationResult{expectedHeight, expectedSize};
}

bool validateInvariants(const general_functions::AVLTree& avl) {
    std::vector<bool> visited(avl.size(), false);
    const auto result = validateNode(avl, avl.tree().root(), NO_NODE, std::nullopt,
                                     std::nullopt, visited);
    return result && result->subtreeSize == avl.size();
}

} // namespace

TEST(AVLTreeTest, InsertAndDuplicate) {
    general_functions::AVLTree tree;
    EXPECT_TRUE(validateInvariants(tree));
    EXPECT_EQ(tree.size(), 0U);
    EXPECT_EQ(tree.tree().root(), NO_NODE);
    EXPECT_TRUE(tree.insert(5));
    EXPECT_FALSE(tree.insert(5));
    EXPECT_EQ(tree.size(), 1U);
    EXPECT_EQ(tree.tree().getNode(tree.tree().root()).data().key, 5);
    EXPECT_TRUE(validateInvariants(tree));
}

TEST(AVLTreeTest, BalancesAllFourRotationCases) {
    for (const auto input : {std::array{3, 2, 1},
                             std::array{1, 2, 3},
                             std::array{3, 1, 2},
                             std::array{1, 3, 2}}) {
        general_functions::AVLTree tree;
        for (int key : input) {
            tree.insert(key);
            EXPECT_TRUE(validateInvariants(tree));
        }
        const auto& binary = tree.tree();
        const auto& root = binary.getNode(binary.root());
        EXPECT_EQ(root.data().key, 2);
        EXPECT_EQ(binary.getNode(root.left()).data().key, 1);
        EXPECT_EQ(binary.getNode(root.right()).data().key, 3);
    }
}

TEST(AVLTreeTest, InvariantsHoldAcrossInsertionsAndExtremeKeys) {
    general_functions::AVLTree tree;
    for (int key : {std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::max()}) {
        tree.insert(key);
        ASSERT_TRUE(validateInvariants(tree));
    }
    for (int i = 0; i < 1000; ++i) {
        tree.insert((i * 37) % 401 - 200);
        ASSERT_TRUE(validateInvariants(tree)) << "Insertion " << i;
    }
    auto copy = tree;
    EXPECT_TRUE(validateInvariants(copy));
    auto moved = std::move(copy);
    EXPECT_TRUE(validateInvariants(moved));
    EXPECT_TRUE(validateInvariants(copy));
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
