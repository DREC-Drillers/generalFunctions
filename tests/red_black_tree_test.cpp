#include <gtest/gtest.h>
#include <functional>
#include <random>
#include <set>
#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
// Inspect topology in tests without exposing nodes in the public API.
#define private public
#include "tree/red_black_tree.hpp"
#undef private

TEST(RedBlackTree, RandomInsertEraseAndBounds) {
    red_black_tree::redBlackTree_t<int, std::less<int>> tree;
    std::set<int> reference;
    std::mt19937 random(42);
    for (int step = 0; step < 3000; ++step) {
        int key = int(random() % 101) - 50;
        if (random() % 2) { tree.insert(key); reference.insert(key); }
        else { tree.erase(key); reference.erase(key); }
        for (int query = -51; query <= 51; ++query) {
            ASSERT_EQ(tree.find(query), reference.count(query) != 0);
            auto lower = reference.lower_bound(query);
            auto upper = reference.upper_bound(query);
            auto actualLower = tree.lowerBound(query);
            auto actualUpper = tree.upperBound(query);
            ASSERT_EQ(actualLower.has_value(), lower != reference.end());
            ASSERT_EQ(actualUpper.has_value(), upper != reference.end());
            if (actualLower) ASSERT_EQ(*actualLower, *lower);
            if (actualUpper) ASSERT_EQ(*actualUpper, *upper);
        }
    }
}
TEST(RedBlackTree, BoundResultSurvivesTreeDestruction) {
    auto result = [] {
        red_black_tree::redBlackTree_t<int, std::greater<int>> tree;
        tree.insert(4); tree.insert(8); tree.insert(2);
        EXPECT_EQ(tree.upperBound(8), 4);
        return tree.lowerBound(7);
    }();
    EXPECT_EQ(result, 4);
}

namespace {
template<class Tree>
void verify(Tree& tree, const std::set<int>& expected) {
    auto nil = tree.nil();
    using Color = decltype(nil->color());
    ASSERT_EQ(nil->color(), Color::BLACK);
    ASSERT_EQ(nil->left(), nil);
    ASSERT_EQ(nil->right(), nil);
    ASSERT_EQ(tree.root_->parent(), nil);
    ASSERT_EQ(tree.root_->color(), Color::BLACK);
    std::unordered_set<decltype(nil)> visited;
    std::vector<int> keys;
    std::function<int(decltype(nil))> walk = [&](auto node) -> int {
        if (node == nil) return 1;
        if (!visited.insert(node).second) { ADD_FAILURE() << "cycle or shared node"; return 0; }
        if (node->left() != nil) EXPECT_EQ(node->left()->parent(), node);
        if (node->right() != nil) EXPECT_EQ(node->right()->parent(), node);
        if (node->color() == Color::RED) {
            EXPECT_EQ(node->left()->color(), Color::BLACK);
            EXPECT_EQ(node->right()->color(), Color::BLACK);
        }
        int left = walk(node->left());
        keys.push_back(node->key());
        int right = walk(node->right());
        EXPECT_EQ(left, right);
        return left + (node->color() == Color::BLACK);
    };
    walk(tree.root_);
    EXPECT_EQ(keys, std::vector<int>(expected.begin(), expected.end()));
}
using Tree = red_black_tree::redBlackTree_t<int, std::less<int>>;
}

TEST(RedBlackTree, AllSmallInsertionAndDeletionOrders) {
    std::array<int, 5> insertion{0,1,2,3,4};
    do {
        std::array<int, 5> deletion{0,1,2,3,4};
        do {
            Tree tree;
            std::set<int> expected;
            verify(tree, expected);
            for (int key : insertion) {
                tree.insert(key); expected.insert(key); verify(tree, expected);
            }
            for (int key : deletion) {
                tree.erase(key); expected.erase(key); verify(tree, expected);
            }
            tree.erase(100);
            verify(tree, expected);
        } while (std::next_permutation(deletion.begin(), deletion.end()));
    } while (std::next_permutation(insertion.begin(), insertion.end()));
}

TEST(RedBlackTree, RandomTopologyAndCompleteDrain) {
    Tree tree;
    std::set<int> expected;
    std::mt19937 random(8123);
    for (int step = 0; step < 10000; ++step) {
        int key = int(random() % 1000);
        if (random() % 2) { tree.insert(key); expected.insert(key); }
        else { tree.erase(key); expected.erase(key); }
        verify(tree, expected);
    }
    while (!expected.empty()) {
        int key = *expected.begin();
        tree.erase(key); expected.erase(key); verify(tree, expected);
    }
    tree.insert(42); verify(tree, {42});
}

TEST(RedBlackTree, EmptyDuplicatesAndIntegerLimits) {
    Tree tree;
    tree.erase(0);
    EXPECT_FALSE(tree.find(0));
    EXPECT_FALSE(tree.lowerBound(0));
    EXPECT_FALSE(tree.upperBound(0));
    std::set<int> keys{std::numeric_limits<int>::min(), 0, std::numeric_limits<int>::max()};
    for (int key : keys) for (int i=0; i<10; ++i) tree.insert(key);
    verify(tree, keys);
    EXPECT_EQ(tree.lowerBound(*keys.begin()), *keys.begin());
    EXPECT_FALSE(tree.upperBound(*keys.rbegin()));
    for (int key : keys) { tree.erase(key); EXPECT_FALSE(tree.find(key)); }
    verify(tree, {});
}

TEST(RedBlackTree, PrivateTraversalHelpers) {
    Tree tree;
    EXPECT_EQ(tree.successor(tree.nil()), tree.nil());
    EXPECT_EQ(tree.treeMaxKeyNode(tree.nil()), tree.nil());
    for(int key : {4,2,6,1,3,5,7}) tree.insert(key);
    auto node = tree.treeMinKeyNode(tree.root_);
    for(int key=1; key<=7; ++key) { ASSERT_NE(node, tree.nil()); EXPECT_EQ(node->key(),key); node=tree.successor(node); }
    EXPECT_EQ(node, tree.nil());
    EXPECT_EQ(tree.treeMaxKeyNode(tree.root_)->key(), 7);
    EXPECT_EQ(tree.root_->grandparent(),tree.nil());
    EXPECT_EQ(tree.root_->uncle(),tree.nil());
    EXPECT_EQ(tree.root_->sibling(),tree.nil());
    tree.rotateParent(tree.root_);
    tree.rotateSibling(tree.root_);
    verify(tree, {1,2,3,4,5,6,7});
}

namespace {
struct Key {
    inline static int live = 0;
    inline static bool failCopy = false;
    int value;
    explicit Key(int v) : value(v) { ++live; }
    Key(const Key& other) : value(other.value) {
        if (failCopy) throw std::runtime_error("copy");
        ++live;
    }
    Key(Key&& other) noexcept : value(other.value) { ++live; }
    ~Key() { --live; }
};
struct Compare {
    bool* fail;
    bool descending;
    bool operator()(const Key& a, const Key& b) const {
        if (*fail) throw std::runtime_error("comparison");
        return descending ? a.value > b.value : a.value < b.value;
    }
};
}
TEST(RedBlackTree, StatefulComparatorLifetimeAndExceptions) {
    EXPECT_EQ(Key::live,0);
    bool fail = false;
    {
        red_black_tree::redBlackTree_t<Key, Compare> tree(Compare{&fail,true});
        tree.insert(Key(1)); tree.insert(Key(3)); tree.insert(Key(2));
        tree.insert(Key(2));
        EXPECT_EQ(Key::live,3);
        const auto& constTree=tree;
        EXPECT_TRUE(constTree.find(Key(2)));
        EXPECT_EQ(constTree.lowerBound(Key(2))->value,2);
        EXPECT_EQ(constTree.upperBound(Key(2))->value,1);
        fail=true;
        EXPECT_THROW(tree.insert(Key(4)),std::runtime_error);
        EXPECT_THROW(tree.erase(Key(2)),std::runtime_error);
        EXPECT_THROW(tree.find(Key(2)),std::runtime_error);
        EXPECT_THROW(tree.lowerBound(Key(2)),std::runtime_error);
        EXPECT_THROW(tree.upperBound(Key(2)),std::runtime_error);
        fail=false;
        EXPECT_EQ(Key::live,3);
        Key::failCopy=true;
        EXPECT_THROW(tree.lowerBound(Key(2)),std::runtime_error);
        EXPECT_THROW(tree.upperBound(Key(2)),std::runtime_error);
        Key::failCopy=false;
        EXPECT_TRUE(tree.find(Key(2)));
        tree.erase(Key(2)); EXPECT_FALSE(tree.find(Key(2)));
        EXPECT_EQ(Key::live,2);
    }
    EXPECT_EQ(Key::live,0);
}
TEST(RedBlackTree, MoveOnlyKeysSupportInsertFindAndErase) {
    auto compare=[](const std::unique_ptr<int>& a,const std::unique_ptr<int>& b) { return *a < *b; };
    red_black_tree::redBlackTree_t<std::unique_ptr<int>, decltype(compare)> tree(compare);
    tree.insert(std::make_unique<int>(2)); tree.insert(std::make_unique<int>(1)); tree.insert(std::make_unique<int>(3));
    auto query=std::make_unique<int>(2);
    EXPECT_TRUE(tree.find(query)); tree.erase(query); EXPECT_FALSE(tree.find(query));
}
