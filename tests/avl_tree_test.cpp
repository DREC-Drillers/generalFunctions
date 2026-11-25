#include <array>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "tree/avlTree.hpp"

namespace {

void expectContents(const general_functions::AVLTree<int>& tree,
                    const std::set<int>& reference) {
    ASSERT_EQ(tree.size(), reference.size());
    auto current = tree.begin();
    std::size_t index = 0;
    for (int key : reference) {
        ASSERT_NE(current, tree.end());
        EXPECT_EQ(*current++, key);
        EXPECT_EQ(tree.select(index), key);
        EXPECT_EQ(tree.rank(key), index);
        ++index;
    }
    EXPECT_EQ(current, tree.end());
}

} // namespace

TEST(AVLTreeTest, InsertAndDuplicate) {
    general_functions::AVLTree tree;
    EXPECT_EQ(tree.size(), 0U);
    EXPECT_EQ(tree.begin(), tree.end());
    EXPECT_TRUE(tree.insert(5));
    EXPECT_FALSE(tree.insert(5));
    EXPECT_EQ(tree.size(), 1U);
    EXPECT_EQ(*tree.begin(), 5);
}

TEST(AVLTreeTest, PreservesOrderForAllFourRotationInputs) {
    for (const auto input : {std::array{3, 2, 1},
                             std::array{1, 2, 3},
                             std::array{3, 1, 2},
                             std::array{1, 3, 2}}) {
        general_functions::AVLTree tree;
        std::set<int> reference;
        for (int key : input) {
            EXPECT_TRUE(tree.insert(key));
            reference.insert(key);
            expectContents(tree, reference);
        }
        EXPECT_EQ(std::vector<int>(std::make_reverse_iterator(tree.end()),
                                   std::make_reverse_iterator(tree.begin())),
                  (std::vector<int>{3, 2, 1}));
    }
}

TEST(AVLTreeTest, InsertionsAndExtremeKeysMatchReference) {
    general_functions::AVLTree tree;
    std::set<int> reference;
    for (int key : {std::numeric_limits<int>::min(),
                    std::numeric_limits<int>::max()}) {
        EXPECT_TRUE(tree.insert(key));
        reference.insert(key);
        expectContents(tree, reference);
    }
    for (int i = 0; i < 1000; ++i) {
        const int key = (i * 37) % 401 - 200;
        EXPECT_EQ(tree.insert(key), reference.insert(key).second);
        expectContents(tree, reference);
    }
    auto copy = tree;
    expectContents(copy, reference);
    auto moved = std::move(copy);
    expectContents(moved, reference);
    expectContents(copy, {});
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

TEST(AVLTreeTest, ErasePreservesIteratorsToSurvivingKeys) {
    general_functions::AVLTree<int> tree;
    for (int key : {4, 2, 7, 1, 3, 6, 8, 5}) {
        tree.insert(key);
    }
    const auto successor = tree.lower_bound(5);
    const auto other = tree.lower_bound(7);
    EXPECT_TRUE(tree.erase(4));
    EXPECT_FALSE(tree.erase(4));
    expectContents(tree, {1, 2, 3, 5, 6, 7, 8});
    EXPECT_EQ(successor, tree.lower_bound(5));
    EXPECT_EQ(other, tree.lower_bound(7));
    EXPECT_EQ(*successor, 5);
    EXPECT_EQ(*std::prev(successor), 3);
}

TEST(AVLTreeTest, EraseAndInsertMatchReference) {
    general_functions::AVLTree<int> tree;
    std::set<int> reference;
    std::mt19937 random(42);
    std::uniform_int_distribution<int> keys(-100, 100);
    for (int i = 0; i < 3000; ++i) {
        const int key = keys(random);
        if (i % 2 == 0) {
            ASSERT_EQ(tree.insert(key), reference.insert(key).second);
        } else {
            ASSERT_EQ(tree.erase(key), reference.erase(key) != 0);
        }
        expectContents(tree, reference);
    }
    while (!reference.empty()) {
        const int key = *reference.begin();
        ASSERT_TRUE(tree.erase(key));
        reference.erase(reference.begin());
        expectContents(tree, reference);
    }
    EXPECT_EQ(tree.begin(), tree.end());
    EXPECT_TRUE(tree.insert(3));
    tree.clear();
    EXPECT_EQ(tree.size(), 0U);
    EXPECT_EQ(tree.begin(), tree.end());
    EXPECT_TRUE(tree.insert(5));
}

TEST(AVLTreeTest, UsesComparatorForKeysWithoutEquality) {
    struct Key { int value; };
    const auto descending = [](const Key& left, const Key& right) {
        return left.value > right.value;
    };
    general_functions::AVLTree<Key, decltype(descending)> tree(descending);
    for (int value : {1, 2, 3}) {
        EXPECT_TRUE(tree.insert(Key{value}));
    }
    EXPECT_FALSE(tree.insert(Key{2}));
    EXPECT_EQ(tree.rank(Key{2}), 1U);
    EXPECT_EQ(tree.select(0).value, 3);
    EXPECT_EQ(tree.lower_bound(Key{2})->value, 2);
    EXPECT_EQ(tree.upper_bound(Key{2})->value, 1);
    EXPECT_TRUE(tree.erase(Key{2}));
    EXPECT_FALSE(tree.erase(Key{2}));
    EXPECT_EQ(tree.lower_bound(Key{2})->value, 1);
}
