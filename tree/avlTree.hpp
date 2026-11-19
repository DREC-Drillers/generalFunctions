#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>

#include "binary_tree.hpp"

namespace general_functions {

class AVLTree {
    struct NodeData {
        explicit NodeData(int value) : key(value) {}

        int key;
        int height = 1;
        std::size_t subtreeSize = 1;
    };

    using Tree = general_functions::BinaryTree<NodeData>;

public:
    using NodeId = typename Tree::NodeId;
    using KeyType = int;

    AVLTree() = default;

    bool insert(int key) {
        if (tree_.root() == Tree::INVALID_NODE) {
            tree_.addRoot(NodeData{key});
            return true;
        }
        return insert(tree_.root(), key);
    }

    std::size_t size() const noexcept { return tree_.size(); }

    std::size_t lower_bound(const KeyType& key) const {
        return boundIndex(key, false);
    }

    std::size_t upper_bound(const KeyType& key) const {
        return boundIndex(key, true);
    }

    const int& select(std::size_t index) const {
        if (index >= size()) {
            throw std::out_of_range("AVLTree: sorted index out of range");
        }
        auto current = tree_.root();
        while (true) {
            const auto& node = tree_.getNode(current);
            const auto leftSize = subtreeSize(node.left());
            if (index < leftSize) {
                current = node.left();
            } else if (index == leftSize) {
                return node.data().key;
            } else {
                index -= leftSize + 1;
                current = node.right();
            }
        }
    }

    const Tree& tree() const noexcept { return tree_; }

private:
    int height(NodeId id) const {
        return id == Tree::INVALID_NODE ? 0 : tree_.getNode(id).data().height;
    }

    std::size_t subtreeSize(NodeId id) const {
        return id == Tree::INVALID_NODE ? 0 : tree_.getNode(id).data().subtreeSize;
    }

    void update(NodeId id) {
        auto& node = tree_.getNode(id);
        node.data().height = 1 + std::max(height(node.left()), height(node.right()));
        node.data().subtreeSize = 1 + subtreeSize(node.left()) + subtreeSize(node.right());
    }

    int balanceFactor(NodeId id) const {
        const auto& node = tree_.getNode(id);
        return height(node.left()) - height(node.right());
    }

    void rotateLeft(NodeId id) {
        const auto newRoot = tree_.getNode(id).right();
        tree_.rotateLeft(id);
        update(id);
        update(newRoot);
    }

    void rotateRight(NodeId id) {
        const auto newRoot = tree_.getNode(id).left();
        tree_.rotateRight(id);
        update(id);
        update(newRoot);
    }

    void rebalance(NodeId id) {
        const auto balance = balanceFactor(id);
        if (balance > 1) {
            const auto left = tree_.getNode(id).left();
            if (balanceFactor(left) < 0) {
                rotateLeft(left);
            }
            rotateRight(id);
        } else if (balance < -1) {
            const auto right = tree_.getNode(id).right();
            if (balanceFactor(right) > 0) {
                rotateRight(right);
            }
            rotateLeft(id);
        }
    }

    bool insert(NodeId id, int key) {
        const auto& node = tree_.getNode(id);
        const int currentKey = node.data().key;
        if (key == currentKey) {
            return false;
        }

        const bool insertLeft = key < currentKey;
        const auto child = insertLeft ? node.left() : node.right();
        if (child != Tree::INVALID_NODE) {
            if (!insert(child, key)) {
                return false;
            }
        } else {
            const auto newChild = tree_.allocateNode(NodeData{key});
            // Allocation can move the node storage, so look up the parent again.
            auto& parent = tree_.getNode(id);
            if (insertLeft) {
                parent.setLeft(newChild);
            } else {
                parent.setRight(newChild);
            }
            tree_.getNode(newChild).setParent(id);
        }

        update(id);
        rebalance(id);
        return true;
    }

    std::size_t boundIndex(int key, bool includeEqual) const {
        std::size_t count = 0;
        auto current = tree_.root();
        while (current != Tree::INVALID_NODE) {
            const auto& node = tree_.getNode(current);
            const auto currentKey = node.data().key;
            if (currentKey < key || (includeEqual && currentKey == key)) {
                count += subtreeSize(node.left()) + 1;
                current = node.right();
            } else {
                current = node.left();
            }
        }
        return count;
    }

    Tree tree_;
};

} // namespace general_functions
