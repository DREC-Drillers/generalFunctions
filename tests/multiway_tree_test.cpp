#include <algorithm>
#include <cstddef>
#include <memory>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "tree/multiwayTree.hpp"

namespace {

using Tree = general_functions::MultiwayTree<int>;

template <typename TreeRef, typename = void>
constexpr bool canEditTopology = false;

template <typename TreeRef>
constexpr bool canEditTopology<
    TreeRef, std::void_t<decltype(std::declval<TreeRef>().editTopology())>> = true;

static_assert(canEditTopology<Tree&>);
static_assert(!canEditTopology<Tree&&>);

TEST(MultiwayTreeTest, EmptyTreeAndInvalidAccess) {
    Tree tree;

    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0U);
    EXPECT_EQ(tree.root(), std::nullopt);
    EXPECT_TRUE(tree.validateInvariants());
    EXPECT_THROW(tree.parent(0), std::out_of_range);
    EXPECT_THROW(tree.child(0, 0), std::out_of_range);
    EXPECT_THROW(tree.childCount(0), std::out_of_range);
    EXPECT_THROW(tree.value(0), std::out_of_range);
    EXPECT_THROW(tree.insertParent(0, 1), std::out_of_range);
}

TEST(MultiwayTreeTest, IndexedInsertionMaintainsParentLinks) {
    Tree tree;
    const auto root = tree.addRoot(10);
    const auto last = tree.addChild(root, 0, 30);
    const auto first = tree.addChild(root, 0, 20);
    const auto middle = tree.addChild(root, 1, 25);

    ASSERT_EQ(tree.childCount(root), 3U);
    EXPECT_EQ(tree.child(root, 0), first);
    EXPECT_EQ(tree.child(root, 1), middle);
    EXPECT_EQ(tree.child(root, 2), last);
    EXPECT_EQ(tree.parent(root), std::nullopt);
    EXPECT_EQ(tree.parent(first), root);
    EXPECT_EQ(tree.parent(middle), root);
    EXPECT_EQ(tree.parent(last), root);
    EXPECT_TRUE(tree.validateInvariants());
    EXPECT_THROW(tree.addRoot(1), std::logic_error);
    EXPECT_THROW(tree.addChild(root, 4, 1), std::out_of_range);
    EXPECT_EQ(tree.size(), 4U);
}

TEST(MultiwayTreeTest, InsertParentAboveRootAndInternalNode) {
    Tree tree;
    const auto oldRoot = tree.addRoot(10);
    const auto child = tree.addChild(oldRoot, 0, 20);
    const auto newRoot = tree.insertParent(oldRoot, 1);
    const auto middle = tree.insertParent(child, 15);

    EXPECT_EQ(tree.root(), newRoot);
    EXPECT_EQ(tree.child(newRoot, 0), oldRoot);
    EXPECT_EQ(tree.parent(oldRoot), newRoot);
    EXPECT_EQ(tree.child(oldRoot, 0), middle);
    EXPECT_EQ(tree.child(middle, 0), child);
    EXPECT_EQ(tree.parent(child), middle);
    EXPECT_EQ(tree.value(oldRoot), 10);
    EXPECT_EQ(tree.value(child), 20);
    EXPECT_TRUE(tree.validateInvariants());
}

TEST(MultiwayTreeTest, TransferRangePreservesOrderAndIds) {
    Tree tree;
    const auto root = tree.addRoot(0);
    const auto source = tree.addChild(root, 0, 10);
    const auto destination = tree.addChild(root, 1, 20);
    const auto existing = tree.addChild(destination, 0, -1);
    std::vector<Tree::NodeId> children;

    for (int value = 0; value < 4; ++value) {
        children.push_back(tree.addChild(source, tree.childCount(source), value));
    }

    tree.moveChildren(source, 1, 2, destination, 0);

    ASSERT_EQ(tree.childCount(source), 2U);
    ASSERT_EQ(tree.childCount(destination), 3U);
    EXPECT_EQ(tree.child(source, 0), children[0]);
    EXPECT_EQ(tree.child(source, 1), children[3]);
    EXPECT_EQ(tree.child(destination, 0), children[1]);
    EXPECT_EQ(tree.child(destination, 1), children[2]);
    EXPECT_EQ(tree.child(destination, 2), existing);
    EXPECT_EQ(tree.parent(children[1]), destination);
    EXPECT_EQ(tree.parent(children[3]), source);
    EXPECT_TRUE(tree.validateInvariants());
}

TEST(MultiwayTreeTest, ReorderingUsesIndexAfterRemoval) {
    Tree tree;
    const auto root = tree.addRoot(0);
    std::vector<Tree::NodeId> children;

    for (int key = 0; key < 5; ++key) {
        children.push_back(tree.addChild(root, tree.childCount(root), key));
    }

    tree.moveChildren(root, 1, 2, root, 3);
    const std::vector<Tree::NodeId> expected{
        children[0], children[3], children[4], children[1], children[2]};

    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(tree.child(root, index), expected[index]);
        EXPECT_EQ(tree.parent(expected[index]), root);
    }

    tree.moveChildren(root, 0, 5, root, 0);
    tree.moveChildren(root, 5, 0, root, 5);
    EXPECT_TRUE(tree.validateInvariants());
}

TEST(MultiwayTreeTest, RejectsCyclesAndInvalidRangesWithoutChanges) {
    Tree tree;
    const auto root = tree.addRoot(0);
    const auto child = tree.addChild(root, 0, 1);
    const auto descendant = tree.addChild(child, 0, 2);

    EXPECT_THROW(tree.moveChildren(root, 0, 1, child, 0), std::invalid_argument);
    EXPECT_THROW(tree.moveChildren(root, 0, 1, descendant, 0),
                 std::invalid_argument);
    EXPECT_THROW(tree.moveChildren(root, 2, 0, child, 0), std::out_of_range);
    EXPECT_THROW(tree.moveChildren(root, 0, 2, child, 0), std::out_of_range);
    EXPECT_THROW(tree.moveChildren(root, 0, 0, child, 2), std::out_of_range);
    EXPECT_THROW(tree.moveChildren(root, 0, 0, 999, 0), std::out_of_range);
    EXPECT_EQ(tree.size(), 3U);
    EXPECT_EQ(tree.child(root, 0), child);
    EXPECT_EQ(tree.child(child, 0), descendant);
    EXPECT_TRUE(tree.validateInvariants());
}

TEST(MultiwayTreeTest, CommitPublishesAllChangesTogether) {
    Tree tree;
    const auto oldRoot = tree.addRoot(10);
    const auto child = tree.addChild(oldRoot, 0, 20);
    auto editor = tree.editTopology();
    const auto newRoot = editor.insertParent(oldRoot, 1);
    const auto destination = editor.addChild(newRoot, 1, 30);
    editor.moveChildren(oldRoot, 0, 1, destination, 0);

    EXPECT_EQ(tree.size(), 2U);
    EXPECT_EQ(tree.root(), oldRoot);
    EXPECT_EQ(tree.parent(child), oldRoot);
    EXPECT_THROW(tree.value(destination), std::out_of_range);

    editor.commitChanges();

    EXPECT_EQ(tree.root(), newRoot);
    EXPECT_EQ(tree.size(), 4U);
    EXPECT_EQ(tree.parent(child), destination);
    EXPECT_EQ(tree.childCount(oldRoot), 0U);
    EXPECT_TRUE(tree.validateInvariants());
    EXPECT_THROW(editor.commitChanges(), std::logic_error);
    EXPECT_THROW(editor.addChild(newRoot, 0, 50), std::logic_error);
}

TEST(MultiwayTreeTest, DroppingOrFailingAnEditDiscardsStagedWork) {
    Tree tree;
    const auto root = tree.addRoot(0);

    {
        auto editor = tree.editTopology();
        editor.addChild(root, 0, 1);
    }

    EXPECT_EQ(tree.size(), 1U);

    auto editor = tree.editTopology();
    const auto child = editor.addChild(root, 0, 1);
    EXPECT_THROW(editor.moveChildren(root, 0, 1, child, 0), std::invalid_argument);
    EXPECT_THROW(editor.commitChanges(), std::logic_error);
    EXPECT_EQ(tree.childCount(root), 0U);
    EXPECT_EQ(tree.size(), 1U);
    EXPECT_TRUE(tree.validateInvariants());
}

TEST(MultiwayTreeTest, InterveningMutationsInvalidateEdits) {
    Tree tree;
    const auto root = tree.addRoot(0);
    auto first = tree.editTopology();
    auto second = tree.editTopology();
    first.addChild(root, 0, 1);
    second.addChild(root, 0, 2);
    first.commitChanges();

    EXPECT_THROW(second.commitChanges(), std::logic_error);
    EXPECT_EQ(tree.size(), 2U);

    auto reserved = tree.editTopology();
    tree.reserve(50);
    EXPECT_THROW(reserved.commitChanges(), std::logic_error);

    auto cleared = tree.editTopology();
    tree.clear();
    EXPECT_THROW(cleared.commitChanges(), std::logic_error);
    EXPECT_TRUE(tree.validateInvariants());
}

TEST(MultiwayTreeTest, CopyMoveSwapAndClearPreserveTopology) {
    Tree tree;
    const auto child = tree.addRoot(1);
    const auto root = tree.insertParent(child, 0);
    Tree copy(tree);
    copy.value(child) = -1;

    EXPECT_EQ(tree.value(child), 1);
    EXPECT_EQ(copy.root(), root);
    EXPECT_TRUE(copy.validateInvariants());

    auto pending = tree.editTopology();
    Tree moved(std::move(tree));
    EXPECT_THROW(pending.commitChanges(), std::logic_error);
    EXPECT_TRUE(moved.validateInvariants());

    tree = Tree{};
    tree.addRoot(99);
    using std::swap;
    swap(tree, moved);

    EXPECT_EQ(tree.root(), root);
    EXPECT_EQ(tree.parent(child), root);
    EXPECT_EQ(moved.value(*moved.root()), 99);

    Tree assigned;
    auto oldEdit = assigned.editTopology();
    assigned = tree;
    EXPECT_THROW(oldEdit.commitChanges(), std::logic_error);
    EXPECT_TRUE(assigned.validateInvariants());
    assigned.clear();
    EXPECT_TRUE(assigned.empty());
    EXPECT_EQ(assigned.root(), std::nullopt);
    EXPECT_THROW(assigned.value(child), std::out_of_range);
    EXPECT_TRUE(assigned.validateInvariants());
}

TEST(MultiwayTreeTest, MoveOnlyValuesSupportGroupedChanges) {
    general_functions::MultiwayTree<std::unique_ptr<int>> tree;
    const auto child = tree.addRoot(std::make_unique<int>(7));
    auto editor = tree.editTopology();
    const auto root = editor.insertParent(child, std::make_unique<int>(1));
    const auto sibling = editor.addChild(root, 1, std::make_unique<int>(9));
    editor.commitChanges();

    EXPECT_EQ(*tree.value(child), 7);
    EXPECT_EQ(*tree.value(sibling), 9);
    EXPECT_EQ(tree.parent(child), root);
    EXPECT_TRUE(tree.validateInvariants());
}

struct CopyOnly {
    explicit CopyOnly(int data) : value(data) {}
    CopyOnly(const CopyOnly&) = default;
    CopyOnly(CopyOnly&&) = delete;
    CopyOnly& operator=(const CopyOnly&) = delete;
    CopyOnly& operator=(CopyOnly&&) = delete;

    int value;
};

TEST(MultiwayTreeTest, CopyConstructibleNonAssignableValues) {
    general_functions::MultiwayTree<CopyOnly> tree;
    const auto oldRoot = tree.addRoot(CopyOnly(7));
    tree.addChild(oldRoot, 0, CopyOnly(8));
    const auto root = tree.insertParent(oldRoot, CopyOnly(1));
    general_functions::MultiwayTree<CopyOnly> copy;
    copy = tree;

    EXPECT_EQ(copy.value(oldRoot).value, 7);
    EXPECT_EQ(copy.root(), root);
    EXPECT_TRUE(copy.validateInvariants());
}

TEST(MultiwayTreeTest, EntireTreeCanBeBuiltInOneEdit) {
    Tree tree;
    auto editor = tree.editTopology();
    const auto firstRoot = editor.addRoot(10);
    const auto child = editor.addChild(firstRoot, 0, 20);
    const auto root = editor.insertParent(firstRoot, 0);
    const auto sibling = editor.addChild(root, 1, 30);
    editor.moveChildren(firstRoot, 0, 1, sibling, 0);
    editor.commitChanges();

    EXPECT_EQ(tree.root(), root);
    EXPECT_EQ(tree.parent(child), sibling);
    EXPECT_EQ(tree.childCount(firstRoot), 0U);
    EXPECT_TRUE(tree.validateInvariants());
}

struct CopyPreferred {
    explicit CopyPreferred(int data) : value(data) {}

    CopyPreferred(const CopyPreferred& other) : value(other.value) {
        if (copiesBeforeThrow == 0) {
            throw std::runtime_error("Injected copy failure");
        }

        if (copiesBeforeThrow > 0) {
            --copiesBeforeThrow;
        }
    }

    CopyPreferred(CopyPreferred&& other) noexcept(false) : value(other.value) {}
    CopyPreferred& operator=(const CopyPreferred&) = default;
    CopyPreferred& operator=(CopyPreferred&&) = default;

    int value;
    inline static int copiesBeforeThrow = -1;
};

TEST(MultiwayTreeTest, FailedCommitRollsBackPartiallyAppendedNodes) {
    for (int budget = 0; budget < 2; ++budget) {
        general_functions::MultiwayTree<CopyPreferred> tree;
        tree.reserve(16);
        const auto root = tree.addRoot(CopyPreferred(1));
        auto editor = tree.editTopology();
        editor.addChild(root, 0, CopyPreferred(2));
        editor.addChild(root, 1, CopyPreferred(3));
        CopyPreferred::copiesBeforeThrow = budget;

        EXPECT_THROW(editor.commitChanges(), std::runtime_error);
        CopyPreferred::copiesBeforeThrow = -1;

        EXPECT_EQ(tree.size(), 1U);
        EXPECT_EQ(tree.childCount(root), 0U);
        EXPECT_EQ(tree.value(root).value, 1);
        EXPECT_TRUE(tree.validateInvariants());
        EXPECT_THROW(editor.commitChanges(), std::logic_error);
    }
}

TEST(MultiwayTreeTest, FailedReserveDuringCommitPreservesExistingNodes) {
    general_functions::MultiwayTree<CopyPreferred> tree;
    const auto root = tree.addRoot(CopyPreferred(1));
    auto editor = tree.editTopology();
    editor.addChild(root, 0, CopyPreferred(2));
    CopyPreferred::copiesBeforeThrow = 0;

    EXPECT_THROW(editor.commitChanges(), std::runtime_error);
    CopyPreferred::copiesBeforeThrow = -1;

    EXPECT_EQ(tree.size(), 1U);
    EXPECT_EQ(tree.value(root).value, 1);
    EXPECT_TRUE(tree.validateInvariants());
}

TEST(MultiwayTreeTest, RandomReparentingMatchesReferenceTopology) {
    Tree tree;
    tree.reserve(100);
    tree.addRoot(0);
    std::vector<std::vector<Tree::NodeId>> children(100);
    std::vector<Tree::NodeId> parents(100, 0);
    std::mt19937 random(23);

    for (Tree::NodeId node = 1; node < 100; ++node) {
        const auto parent = static_cast<Tree::NodeId>(random() % node);
        EXPECT_EQ(tree.addChild(parent, children[parent].size(), node), node);
        children[parent].push_back(node);
        parents[node] = parent;
    }

    for (int step = 0; step < 1000; ++step) {
        const auto node = static_cast<Tree::NodeId>(1 + random() % 99);
        const auto destination = static_cast<Tree::NodeId>(random() % 100);
        const auto source = parents[node];
        bool cycle = false;

        for (auto ancestor = destination; ancestor != 0;
             ancestor = parents[ancestor]) {
            cycle = cycle || ancestor == node;
        }

        const auto position =
            std::find(children[source].begin(), children[source].end(), node);
        const auto index =
            static_cast<std::size_t>(position - children[source].begin());

        if (cycle) {
            EXPECT_THROW(tree.moveChildren(source, index, 1, destination, 0),
                         std::invalid_argument);
        } else {
            tree.moveChildren(source, index, 1, destination, 0);
            children[source].erase(position);
            children[destination].insert(children[destination].begin(), node);
            parents[node] = destination;
        }

        ASSERT_TRUE(tree.validateInvariants());

        for (Tree::NodeId parent = 0; parent < 100; ++parent) {
            ASSERT_EQ(tree.childCount(parent), children[parent].size());

            for (std::size_t childIndex = 0; childIndex < children[parent].size();
                 ++childIndex) {
                ASSERT_EQ(tree.child(parent, childIndex),
                          children[parent][childIndex]);
            }
        }
    }
}

TEST(MultiwayTreeTest, DeepTreeValidationIsIterative) {
    Tree tree;
    auto tail = tree.addRoot(0);

    for (int value = 1; value < 10000; ++value) {
        tail = tree.addChild(tail, 0, value);
    }

    EXPECT_EQ(tree.size(), 10000U);
    EXPECT_TRUE(tree.validateInvariants());
}

} // namespace
