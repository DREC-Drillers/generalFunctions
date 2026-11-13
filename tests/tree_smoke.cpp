#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include "tree/binary_tree.hpp"

namespace {

void require(bool condition) {
    if (!condition) {
        throw std::runtime_error("BinaryTree test failed");
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

    ThrowingCopy(const ThrowingCopy& other) : value(other.value) { checkCopy(); }
    ThrowingCopy(ThrowingCopy&&) noexcept = default;

    ThrowingCopy& operator=(const ThrowingCopy& other) {
        checkCopy();
        value = other.value;
        return *this;
    }
    ThrowingCopy& operator=(ThrowingCopy&&) noexcept = default;

    static void checkCopy() {
        if (copiesBeforeThrow == 0) {
            throw std::runtime_error("Injected copy failure");
        }
        if (copiesBeforeThrow > 0) {
            --copiesBeforeThrow;
        }
    }

    int value;
    inline static int copiesBeforeThrow = -1;
};

void testCopyExceptionSafety() {
    using Tree = general_functions::BinaryTree<ThrowingCopy>;
    Tree target;
    target.reserve(8);
    const auto targetRoot = target.addRoot(ThrowingCopy(1));
    const auto targetLeft = target.addLeft(targetRoot, ThrowingCopy(2));
    Tree source;
    const auto sourceRoot = source.addRoot(ThrowingCopy(10));
    const auto sourceLeft = source.addLeft(sourceRoot, ThrowingCopy(20));
    const auto sourceRight = source.addRight(sourceRoot, ThrowingCopy(30));

    // The old vector assignment copied the root links before the second copy
    // failed, leaving target.right(root) == 2 while target.size() == 2.
    ThrowingCopy::copiesBeforeThrow = 1;
    requireThrows<std::runtime_error>([&] { target = source; });
    require(target.size() == 2 && target.root() == targetRoot);
    require(target.left(targetRoot) == targetLeft && !target.right(targetRoot));
    require(target.value(targetRoot).value == 1 &&
            target.value(targetLeft).value == 2);
    require(source.size() == 3 && source.value(sourceRoot).value == 10);
    require(source.left(sourceRoot) == sourceLeft &&
            source.right(sourceRoot) == sourceRight);

    ThrowingCopy::copiesBeforeThrow = 1;
    requireThrows<std::runtime_error>([&] { Tree copy(source); });
    require(source.size() == 3 && source.value(sourceLeft).value == 20);

    // Self-assignment must not copy even if the next copy would throw.
    ThrowingCopy::copiesBeforeThrow = 0;
    auto& sameTarget = target;
    target = sameTarget;
    require(target.size() == 2 && target.value(targetRoot).value == 1);

    ThrowingCopy::copiesBeforeThrow = -1;
    target = source;
    require(target.size() == 3 && target.left(sourceRoot) == sourceLeft);
    require(target.right(sourceRoot) == sourceRight);
    target.value(sourceLeft).value = 99;
    require(source.value(sourceLeft).value == 20);

    Tree empty;
    target = empty;
    require(target.empty() && !target.root());
    empty = source;
    require(empty.size() == 3 && empty.value(sourceRight).value == 30);

    using std::swap;
    swap(target, empty);
    require(target.size() == 3 && empty.empty());
}

} // namespace

int main() {
    testCopyExceptionSafety();
    using general_functions::BinaryTree;
    BinaryTree<int> tree;
    require(tree.empty() && tree.size() == 0 && !tree.root());
    tree.reserve(3);
    const auto root = tree.addRoot(10);
    const auto left = tree.addLeft(root, 20);
    const auto right = tree.addRight(root, 30);
    require(tree.root() == root && tree.left(root) == left &&
            tree.right(root) == right);
    require(!tree.left(left) && !tree.right(left));
    tree.value(left) = 25;
    const auto& constTree = tree;
    require(constTree.value(left) == 25);

    requireThrows<std::logic_error>([&] { tree.addRoot(40); });
    requireThrows<std::logic_error>([&] { tree.addLeft(root, 40); });
    requireThrows<std::logic_error>([&] { tree.addRight(root, 40); });
    constexpr auto INVALID_ID = std::numeric_limits<BinaryTree<int>::NodeId>::max();
    requireThrows<std::out_of_range>([&] { tree.addLeft(INVALID_ID, 40); });
    requireThrows<std::out_of_range>([&] { tree.addRight(INVALID_ID, 40); });
    requireThrows<std::out_of_range>([&] { tree.value(INVALID_ID); });
    requireThrows<std::out_of_range>([&] { constTree.value(INVALID_ID); });
    requireThrows<std::out_of_range>([&] { tree.left(INVALID_ID); });
    requireThrows<std::out_of_range>([&] { tree.right(INVALID_ID); });
    require(tree.size() == 3);

    auto tail = left;
    for (int value = 0; value < 1000; ++value) {
        tail = tree.addLeft(tail, value);
    }
    tree.reserve(2000);
    require(tree.value(root) == 10 && tree.value(left) == 25);
    require(tree.value(right) == 30 && tree.value(tail) == 999);
    require(tree.left(root) == left && tree.right(root) == right);

    auto copy = tree;
    copy.value(root) = -1;
    require(tree.value(root) == 10 && copy.value(root) == -1);
    auto moved = std::move(copy);
    require(moved.value(root) == -1 && moved.left(root) == left);

    BinaryTree<std::unique_ptr<int>> owning;
    const auto owningRoot = owning.addRoot(std::make_unique<int>(7));
    const auto owningChild = owning.addRight(owningRoot, std::make_unique<int>(9));
    require(*owning.value(owningRoot) == 7 && *owning.value(owningChild) == 9);
    auto movedOwning = std::move(owning);
    require(*movedOwning.value(owningChild) == 9);
    BinaryTree<std::unique_ptr<int>> assignedOwning;
    assignedOwning.addRoot(std::make_unique<int>(-1));
    assignedOwning = std::move(movedOwning);
    require(*assignedOwning.value(owningRoot) == 7);
    require(assignedOwning.right(owningRoot) == owningChild);
    require(*assignedOwning.value(owningChild) == 9);
}
