#include <gtest/gtest.h>
#include <functional>
#include <random>
#include <set>
#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <vector>
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
            ASSERT_EQ(tree.countKeysLessThan(query),
                      static_cast<std::size_t>(std::distance(reference.begin(), lower)));
            ASSERT_EQ(tree.countKeysLessThanOrEqual(query),
                      static_cast<std::size_t>(std::distance(reference.begin(), upper)));
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
        EXPECT_EQ(tree.countKeysLessThan(4), 1U);
        EXPECT_EQ(tree.countKeysLessThanOrEqual(4), 2U);
        EXPECT_EQ(tree.countKeysLessThan(1), 3U);
        return tree.lowerBound(7);
    }();
    EXPECT_EQ(result, 4);
}

namespace {
template<class Tree>
void verify(Tree& tree, const std::set<int>& expected) {
    using NodeId = typename Tree::NodeId;
    using Color = typename Tree::nodeColor_t;
    const NodeId nil = tree.nil();
    const auto& storage = tree.tree_;
    const NodeId root = storage.root();
    ASSERT_EQ(storage.size(), expected.size());
    ASSERT_EQ(root == nil, expected.empty());

    auto colorOf = [&](NodeId id) {
        return id == nil ? Color::BLACK : storage.getNode(id).data().color();
    };
    ASSERT_EQ(colorOf(root), Color::BLACK);
    std::unordered_set<NodeId> visited;
    std::vector<int> keys;
    std::function<int(NodeId, NodeId)> walk = [&](NodeId id, NodeId parent) -> int {
        if (id == nil) {
            return 1;
        }
        if (!visited.insert(id).second) {
            ADD_FAILURE() << "cycle or shared node";
            return 0;
        }
        const auto& node = storage.getNode(id);
        EXPECT_EQ(node.parent(), parent);
        if (node.data().color() == Color::RED) {
            EXPECT_EQ(colorOf(node.left()), Color::BLACK);
            EXPECT_EQ(colorOf(node.right()), Color::BLACK);
        }
        const auto first = keys.size();
        int left = walk(node.left(), id);
        keys.push_back(node.data().key());
        int right = walk(node.right(), id);
        EXPECT_EQ(node.data().subtreeSize(), keys.size() - first);
        EXPECT_EQ(left, right);
        return left + (node.data().color() == Color::BLACK);
    };
    walk(root, nil);
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
    EXPECT_EQ(tree.countKeysLessThan(0), 0U);
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
    const auto root = tree.tree_.root();
    auto nodeId = tree.treeMinKeyNode(root);
    for (int key = 1; key <= 7; ++key) {
        ASSERT_NE(nodeId, tree.nil());
        EXPECT_EQ(tree.tree_.getNode(nodeId).data().key(), key);
        nodeId = tree.successor(nodeId);
    }
    EXPECT_EQ(nodeId, tree.nil());
    EXPECT_EQ(tree.tree_.getNode(tree.treeMaxKeyNode(root)).data().key(), 7);
    EXPECT_EQ(tree.grandparent(root), tree.nil());
    EXPECT_EQ(tree.uncle(root), tree.nil());
    EXPECT_EQ(tree.sibling(root, tree.nil()), tree.nil());
    tree.rotateParent(root, tree.nil());
    tree.rotateSibling(root, tree.nil());
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
