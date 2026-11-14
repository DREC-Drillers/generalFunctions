#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include "tree/binary_tree.hpp"
#include "tree/multiwayTree.hpp"

namespace {
void require(bool condition) {
    if (!condition) {
        throw std::runtime_error("MultiwayTree test failed");
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
    using Tree = general_functions::MultiwayTree<ThrowingCopy>;
    Tree target;
    target.reserve(8);
    const auto root = target.addRoot(ThrowingCopy(1));
    const auto first = target.addChild(root, 0, ThrowingCopy(2));
    Tree source;
    const auto sourceRoot = source.addRoot(ThrowingCopy(10));
    source.addChild(sourceRoot, 0, ThrowingCopy(20));
    source.addChild(sourceRoot, 1, ThrowingCopy(30));
    ThrowingCopy::copiesBeforeThrow = 1;
    requireThrows<std::runtime_error>([&] { target = source; });
    require(target.size() == 2 && target.childCount(root) == 1);
    require(target.child(root, 0) == first && target.value(root).value == 1);
    require(target.value(first).value == 2 && source.size() == 3);
    ThrowingCopy::copiesBeforeThrow = 0;
    auto& sameTree = target;
    target = sameTree;
    require(target.value(root).value == 1);
    ThrowingCopy::copiesBeforeThrow = -1;
    target = source;
    target.value(root).value = -1;
    require(source.value(sourceRoot).value == 10);
    require(target.childCount(root) == 2);
}
} // namespace

int main() {
    using general_functions::MultiwayTree;
    MultiwayTree<int> tree;
    require(tree.empty() && !tree.root() && tree.size() == 0);
    const auto root = tree.addRoot(10);
    require(tree.childCount(root) == 0);
    requireThrows<std::out_of_range>([&] { tree.child(root, 0); });
    const auto second = tree.addChild(root, 0, 30);
    const auto first = tree.addChild(root, 0, 20);
    const auto middle = tree.addChild(root, 1, 25);
    require(tree.childCount(root) == 3 && tree.size() == 4);
    require(tree.child(root, 0) == first && tree.child(root, 1) == middle);
    require(tree.child(root, 2) == second && tree.value(second) == 30);
    requireThrows<std::out_of_range>([&] { tree.child(root, 3); });
    requireThrows<std::out_of_range>([&] { tree.addChild(root, 4, 40); });
    constexpr auto INVALID_ID =
        std::numeric_limits<MultiwayTree<int>::NodeId>::max();
    requireThrows<std::out_of_range>([&] { tree.child(INVALID_ID, 0); });
    requireThrows<std::out_of_range>([&] { tree.childCount(INVALID_ID); });
    requireThrows<std::out_of_range>([&] { tree.value(INVALID_ID); });
    requireThrows<std::out_of_range>([&] { tree.addChild(INVALID_ID, 0, 40); });
    requireThrows<std::logic_error>([&] { tree.addRoot(40); });
    require(tree.size() == 4);
    auto tail = first;
    for (int value = 0; value < 1000; ++value) {
        tail = tree.addChild(tail, 0, value);
    }
    tree.reserve(2000);
    const auto& constTree = tree;
    require(constTree.value(tail) == 999 && constTree.child(root, 0) == first);
    auto copy = tree;
    copy.value(root) = -1;
    require(tree.value(root) == 10);
    auto moved = std::move(copy);
    require(moved.value(root) == -1 && moved.child(root, 2) == second);
    MultiwayTree<int> assigned;
    assigned = tree;
    require(assigned.value(tail) == 999);
    MultiwayTree<int> empty;
    assigned = empty;
    require(assigned.empty() && !assigned.root());
    using std::swap;
    swap(assigned, tree);
    require(tree.empty() && assigned.value(tail) == 999);

    MultiwayTree<std::unique_ptr<int>> owning;
    const auto owningRoot = owning.addRoot(std::make_unique<int>(7));
    const auto owningChild =
        owning.addChild(owningRoot, 0, std::make_unique<int>(9));
    MultiwayTree<std::unique_ptr<int>> movedOwning(std::move(owning));
    MultiwayTree<std::unique_ptr<int>> assignedOwning;
    assignedOwning = std::move(movedOwning);
    require(*assignedOwning.value(owningChild) == 9);
    require(assignedOwning.child(owningRoot, 0) == owningChild);
    testCopyExceptionSafety();
}
