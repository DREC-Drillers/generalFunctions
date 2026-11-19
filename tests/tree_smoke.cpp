#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include "tree/binary_tree.hpp"

namespace {

using general_functions::BinaryTree;
using Tree = BinaryTree<int>;
constexpr auto nil = Tree::INVALID_NODE;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Exception, typename Operation>
void requireThrows(Operation operation) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error("Expected exception was not thrown");
}

struct ThrowingCopy {
    explicit ThrowingCopy(int data) : value(data) {}
    ThrowingCopy(const ThrowingCopy& other) : value(other.value) {
        if (copiesBeforeThrow == 0) {
            throw std::runtime_error("Injected copy failure");
        }
        if (copiesBeforeThrow > 0) {
            --copiesBeforeThrow;
        }
    }
    ThrowingCopy(ThrowingCopy&&) noexcept = default;

    int value;
    inline static int copiesBeforeThrow = -1;
};

void testCopyExceptionSafety() {
    BinaryTree<ThrowingCopy> target;
    const auto targetRoot = target.addRoot(ThrowingCopy(1));
    const auto targetFree = target.allocateNode(ThrowingCopy(2));
    target.releaseNode(targetFree);

    BinaryTree<ThrowingCopy> source;
    const auto sourceRoot = source.addRoot(ThrowingCopy(10));
    const auto child = source.allocateNode(ThrowingCopy(20));
    source.getNode(sourceRoot).setLeft(child);
    source.getNode(child).setParent(sourceRoot);

    ThrowingCopy::copiesBeforeThrow = 1;
    requireThrows<std::runtime_error>([&] { target = source; });
    require(target.size() == 1 && target.root() == targetRoot &&
                target.getNode(targetRoot).data().value == 1,
            "Failed assignment changed the target tree");

    ThrowingCopy::copiesBeforeThrow = 1;
    requireThrows<std::runtime_error>([&] { return BinaryTree<ThrowingCopy>(source); });
    require(source.size() == 2 && source.getNode(child).data().value == 20,
            "Failed copy changed the source tree");

    ThrowingCopy::copiesBeforeThrow = 0;
    const auto& sameTarget = target;
    target = sameTarget;
    ThrowingCopy::copiesBeforeThrow = -1;
    require(target.allocateNode(ThrowingCopy(3)) == targetFree,
            "Failed assignment lost the target's free slot");

    target = source;
    require(target.root() == sourceRoot && target.getNode(sourceRoot).left() == child &&
                target.getNode(child).parent() == sourceRoot,
            "Copy assignment lost a tree link");
    target.getNode(child).data().value = 99;
    require(source.getNode(child).data().value == 20, "Copy shares data with source");
}

void testRotationsAndTransplant() {
    Tree tree;
    const auto root = tree.addRoot(20);
    const auto pivot = tree.allocateNode(40);
    const auto middle = tree.allocateNode(30);
    tree.getNode(root).setRight(pivot);
    tree.getNode(pivot).setParent(root);
    tree.getNode(pivot).setLeft(middle);
    tree.getNode(middle).setParent(pivot);

    tree.rotateLeft(root);
    require(tree.root() == pivot && tree.getNode(pivot).parent() == nil &&
                tree.getNode(pivot).left() == root && tree.getNode(root).parent() == pivot &&
                tree.getNode(root).right() == middle && tree.getNode(middle).parent() == root,
            "Left rotation at root left inconsistent links");

    tree.rotateLeft(root);
    require(tree.root() == pivot && tree.getNode(pivot).left() == middle &&
                tree.getNode(middle).parent() == pivot && tree.getNode(middle).left() == root &&
                tree.getNode(root).parent() == middle && tree.getNode(root).right() == nil,
            "Left rotation inside tree left inconsistent links");

    tree.rotateRight(middle);
    require(tree.getNode(pivot).left() == root && tree.getNode(root).right() == middle &&
                tree.getNode(middle).parent() == root,
            "Right rotation inside tree left inconsistent links");

    tree.rotateRight(pivot);
    require(tree.root() == root && tree.getNode(root).parent() == nil &&
                tree.getNode(root).right() == pivot && tree.getNode(pivot).parent() == root &&
                tree.getNode(pivot).left() == middle && tree.getNode(middle).parent() == pivot,
            "Right rotation at root left inconsistent links");

    requireThrows<std::out_of_range>([&] { tree.rotateRight(root); });
    requireThrows<std::logic_error>([&] { tree.releaseNode(middle); });

    const auto replacement = tree.allocateNode(35);
    tree.transplant(middle, replacement);
    require(tree.getNode(pivot).left() == replacement &&
                tree.getNode(replacement).parent() == pivot &&
                tree.getNode(middle).parent() == nil,
            "Transplant failed to replace a child");
    tree.transplant(replacement, replacement);
    require(tree.getNode(pivot).left() == replacement &&
                tree.getNode(replacement).parent() == pivot,
            "Transplant to the same node changed its links");
    tree.releaseNode(middle);

    requireThrows<std::out_of_range>([&] { tree.transplant(replacement, middle); });
    require(tree.getNode(pivot).left() == replacement,
            "Failed transplant changed the parent");
    tree.transplant(replacement, nil);
    require(tree.getNode(pivot).left() == nil, "Transplant failed to unlink the child");
    tree.releaseNode(replacement);

    tree.transplant(root, pivot);
    require(tree.root() == pivot && tree.getNode(pivot).parent() == nil,
            "Transplant failed to replace the root");
    tree.getNode(root).setRight(nil);
    tree.releaseNode(root);
    tree.releaseNode(pivot);
    require(tree.empty() && tree.root() == nil, "Releasing the last root did not empty the tree");
}

void testGrowthAndFreeSlots() {
    Tree tree;
    require(tree.empty() && tree.root() == nil, "New tree is not empty");
    tree.reserve(3);
    const auto root = tree.addRoot(10);
    const auto detached = tree.allocateNode(20);
    requireThrows<std::logic_error>([&] { tree.addRoot(30); });
    requireThrows<std::out_of_range>([&] { tree.getNode(nil); });
    const auto& constTree = tree;
    requireThrows<std::out_of_range>([&] { constTree.getNode(nil); });

    auto tail = root;
    for (int value = 0; value < 1000; ++value) {
        const auto child = tree.allocateNode(int{value});
        tree.getNode(tail).setLeft(child);
        tree.getNode(child).setParent(tail);
        tail = child;
    }
    tree.reserve(2000);
    require(tree.getNode(root).data() == 10 && tree.getNode(tail).data() == 999 &&
                tree.getNode(tree.getNode(root).left()).parent() == root,
            "Growing the vector changed an existing node or link");

    tree.prepareRelease();
    tree.releaseNode(detached);
    requireThrows<std::out_of_range>([&] { tree.releaseNode(detached); });
    requireThrows<std::out_of_range>([&] { constTree.getNode(detached); });

    auto copy = tree;
    require(copy.allocateNode(50) == detached, "Copy did not retain a vacant slot");
    require(tree.size() == 1001 && copy.size() == 1002, "Copy changed the source size");
    auto moved = std::move(tree);
    // Empty moved-from trees are part of the BinaryTree contract.
    // NOLINTNEXTLINE(bugprone-use-after-move)
    require(tree.empty() && tree.root() == nil, "Move constructor did not empty the source");
    tree = std::move(moved);
    // NOLINTNEXTLINE(bugprone-use-after-move)
    require(moved.empty() && moved.root() == nil, "Move assignment did not empty the source");
    require(tree.allocateNode(60) == detached && tree.size() == 1002,
            "Move lost the vacant slot or the live nodes");
    tree.swap(copy);
    require(tree.getNode(detached).data() == 50 && copy.getNode(detached).data() == 60,
            "Swap did not exchange data");
}

void testMoveOnlyValues() {
    BinaryTree<std::unique_ptr<int>> tree;
    const auto root = tree.addRoot(std::make_unique<int>(7));
    const auto child = tree.allocateNode(std::make_unique<int>(9));
    auto moved = std::move(tree);
    require(*moved.getNode(root).data() == 7 && *moved.getNode(child).data() == 9,
            "Moving the tree lost a move-only value");
    moved.releaseNode(child);
    require(moved.allocateNode(std::make_unique<int>(11)) == child &&
                *moved.getNode(child).data() == 11,
            "Reusing a slot did not construct a new move-only value");
}

struct ThrowingMove {
    explicit ThrowingMove(int data) : value(data) {}
    ThrowingMove(const ThrowingMove&) = default;
    // Exercise failure while constructing a node in a reused slot.
    // NOLINTNEXTLINE(bugprone-exception-escape,performance-noexcept-move-constructor)
    ThrowingMove(ThrowingMove&& other) : value(other.value) {
        if (failMove) {
            throw std::runtime_error("Injected move failure");
        }
    }

    int value;
    inline static bool failMove = false;
};

void testReuseAfterFailedConstruction() {
    BinaryTree<ThrowingMove> tree;
    const auto root = tree.addRoot(ThrowingMove(10));
    const auto slot = tree.allocateNode(ThrowingMove(20));
    tree.releaseNode(slot);

    ThrowingMove::failMove = true;
    requireThrows<std::runtime_error>([&] { tree.allocateNode(ThrowingMove(30)); });
    ThrowingMove::failMove = false;
    require(tree.size() == 1 && tree.getNode(root).data().value == 10,
            "Failed construction changed the live nodes");
    requireThrows<std::out_of_range>([&] { tree.getNode(slot); });
    require(tree.allocateNode(ThrowingMove(40)) == slot && tree.size() == 2,
            "Failed construction lost a vacant slot");
}

} // namespace

int main() {
    try {
        testCopyExceptionSafety();
        testRotationsAndTransplant();
        testGrowthAndFreeSlots();
        testMoveOnlyValues();
        testReuseAfterFailedConstruction();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
