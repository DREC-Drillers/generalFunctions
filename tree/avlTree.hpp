#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
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
        if (!tree_.root().has_value()) {
            tree_.addRoot(NodeData{key});
            return true;
        }
        return insert(tree_.root().value(), key, std::nullopt);
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
        auto current = tree_.root().value();
        while (true) {
            const auto leftSize = subtreeSize(tree_.left(current));
            if (index < leftSize) {
                current = tree_.left(current).value();
            } else if (index == leftSize) {
                return tree_.value(current).key;
            } else {
                index -= leftSize + 1;
                current = tree_.right(current).value();
            }
        }
    }

    const Tree& tree() const noexcept { return tree_; }

private:
    int height(std::optional<NodeId> id) const {
        return id.has_value() ? tree_.value(id.value()).height : 0;
    }

    std::size_t subtreeSize(std::optional<NodeId> id) const {
        return id.has_value() ? tree_.value(id.value()).subtreeSize : 0;
    }

    void update(NodeId id) {
        auto& node = tree_.value(id);
        node.height = 1 + std::max(height(tree_.left(id)), height(tree_.right(id)));
        node.subtreeSize = 1 + subtreeSize(tree_.left(id)) + subtreeSize(tree_.right(id));
    }

    int balanceFactor(NodeId id) const {
        return height(tree_.left(id)) - height(tree_.right(id));
    }

    void replaceRoot(NodeId oldRoot, NodeId newRoot, std::optional<NodeId> parent) {
        if (!parent.has_value()) {
            tree_.setRoot(newRoot);
        } else if (tree_.left(parent.value()) == oldRoot) {
            tree_.setLeft(parent.value(), newRoot);
        } else {
            tree_.setRight(parent.value(), newRoot);
        }
    }

    void rotateLeft(NodeId id, std::optional<NodeId> parent) {
        const auto newRoot = tree_.right(id).value();
        tree_.setRight(id, tree_.left(newRoot));
        tree_.setLeft(newRoot, id);
        replaceRoot(id, newRoot, parent);
        update(id);
        update(newRoot);
    }

    void rotateRight(NodeId id, std::optional<NodeId> parent) {
        const auto newRoot = tree_.left(id).value();
        tree_.setLeft(id, tree_.right(newRoot));
        tree_.setRight(newRoot, id);
        replaceRoot(id, newRoot, parent);
        update(id);
        update(newRoot);
    }

    void rebalance(NodeId id, std::optional<NodeId> parent) {
        const auto balance = balanceFactor(id);
        if (balance > 1) {
            const auto left = tree_.left(id).value();
            if (balanceFactor(left) < 0) {
                rotateLeft(left, id);  //LR
            }
            rotateRight(id, parent); //LL
        } else if (balance < -1) {
            const auto right = tree_.right(id).value();
            if (balanceFactor(right) > 0) {
                rotateRight(right, id); //RL
            }
            rotateLeft(id, parent); //RR
        }
    }

    bool insert(NodeId id, int key, std::optional<NodeId> parent) {
        const int currentKey = tree_.value(id).key;
        bool inserted = false;
        if (key < currentKey) {
            const auto left = tree_.left(id);
            if (left.has_value()) {
                inserted = insert(left.value(), key, id);
            } else {
                tree_.addLeft(id, NodeData{key});
                inserted = true;
            }
        } else if (key > currentKey) {
            const auto right = tree_.right(id);
            if (right.has_value()) {
                inserted = insert(right.value(), key, id);
            } else {
                tree_.addRight(id, NodeData{key});
                inserted = true;
            }
        }
        if (inserted) {
            update(id);
            rebalance(id, parent);
        }
        return inserted;
    }

    std::size_t boundIndex(int key, bool includeEqual) const {
        std::size_t count = 0;
        auto current = tree_.root();
        while (current.has_value()) {
            const auto& node = tree_.value(current.value());
            if (node.key < key || (includeEqual && node.key == key)) {
                count += subtreeSize(tree_.left(current.value())) + 1;
                current = tree_.right(current.value());
            } else {
                current = tree_.left(current.value());
            }
        }
        return count;
    }

    Tree tree_;
};

} // namespace general_functions
