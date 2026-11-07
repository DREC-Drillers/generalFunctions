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

} // namespace

int main() {
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
}
