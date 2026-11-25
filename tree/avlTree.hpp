#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>

#include "binary_tree.hpp"

namespace general_functions {

template <typename Key = int, typename Compare = std::less<Key>>
class AVLTree {
    struct NodeData {
        explicit NodeData(const Key& value) : key(value) {}

        Key key;
        int height = 1;
        std::size_t subtreeSize = 1;
    };

    using Tree = BinaryTree<NodeData>;
    using NodeId = typename Tree::NodeId;

public:
    using KeyType = Key;

    class const_iterator {
        friend class AVLTree;

        typename Tree::const_iterator current_;

        explicit const_iterator(typename Tree::const_iterator current) : current_(current) {}

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Key;
        using difference_type = std::ptrdiff_t;
        using reference = const Key&;
        using pointer = const Key*;

        const_iterator() = default;

        reference operator*() const { return current_->key; }
        pointer operator->() const { return std::addressof(operator*()); }

        const_iterator& operator++() {
            ++current_;
            return *this;
        }

        const_iterator operator++(int) {
            auto previous = *this;
            ++*this;
            return previous;
        }

        const_iterator& operator--() {
            --current_;
            return *this;
        }

        const_iterator operator--(int) {
            auto previous = *this;
            --*this;
            return previous;
        }

        bool operator==(const const_iterator& other) const {
            return current_ == other.current_;
        }

        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }
    };

    using iterator = const_iterator;

    AVLTree() = default;
    explicit AVLTree(Compare compare) : compare_(std::move(compare)) {}

    bool insert(const Key& key) {
        if (tree_.root() == Tree::INVALID_NODE) {
            tree_.addRoot(NodeData{key});
            return true;
        }
        return insert(tree_.root(), key);
    }

    bool erase(const Key& key) {
        const auto id = findBound(key, false).node;
        if (id == Tree::INVALID_NODE || compare_(key, tree_.getNode(id).data().key)) {
            return false;
        }

        tree_.prepareRelease();
        auto& node = tree_.getNode(id);
        auto ancestor = node.parent();
        if (node.left() != Tree::INVALID_NODE && node.right() != Tree::INVALID_NODE) {
            ancestor = replaceWithSuccessor(id);
        } else {
            tree_.transplant(id, node.left() != Tree::INVALID_NODE ? node.left() : node.right());
        }
        node.setLeft(Tree::INVALID_NODE);
        node.setRight(Tree::INVALID_NODE);
        tree_.releaseNode(id);

        while (ancestor != Tree::INVALID_NODE) {
            const auto parent = tree_.getNode(ancestor).parent();
            update(ancestor);
            rebalance(ancestor);
            ancestor = parent;
        }
        return true;
    }

    std::size_t size() const noexcept { return tree_.size(); }

    const_iterator begin() const { return const_iterator(tree_.begin()); }
    const_iterator end() const { return const_iterator(tree_.end()); }

    const_iterator lower_bound(const KeyType& key) const {
        return const_iterator(tree_.iteratorAt(findBound(key, false).node));
    }

    const_iterator upper_bound(const KeyType& key) const {
        return const_iterator(tree_.iteratorAt(findBound(key, true).node));
    }

    // Number of keys that precede key according to Compare.
    std::size_t rank(const Key& key) const { return findBound(key, false).rank; }

    const Key& select(std::size_t index) const {
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

private:
    NodeId replaceWithSuccessor(NodeId id) {
        const auto& node = tree_.getNode(id);
        auto successorId = node.right();
        while (tree_.getNode(successorId).left() != Tree::INVALID_NODE) {
            successorId = tree_.getNode(successorId).left();
        }

        // Move the successor node so iterators to surviving keys keep their node IDs.
        auto& successor = tree_.getNode(successorId);
        auto ancestor = successorId;
        if (successorId != node.right()) {
            ancestor = successor.parent();
            tree_.transplant(successorId, successor.right());
            successor.setRight(node.right());
            tree_.getNode(node.right()).setParent(successorId);
        }

        tree_.transplant(id, successorId);
        successor.setLeft(node.left());
        tree_.getNode(node.left()).setParent(successorId);
        return ancestor;
    }

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

    bool insert(NodeId id, const Key& key) {
        const auto& node = tree_.getNode(id);
        const auto& currentKey = node.data().key;
        const bool insertLeft = compare_(key, currentKey);
        if (!insertLeft && !compare_(currentKey, key)) {
            return false;
        }

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

    struct Bound {
        NodeId node = Tree::INVALID_NODE;
        std::size_t rank = 0;
    };

    Bound findBound(const Key& key, bool includeEqual) const {
        Bound bound;
        auto current = tree_.root();
        while (current != Tree::INVALID_NODE) {
            const auto& node = tree_.getNode(current);
            const auto& currentKey = node.data().key;
            if (includeEqual ? !compare_(key, currentKey) : compare_(currentKey, key)) {
                bound.rank += subtreeSize(node.left()) + 1;
                current = node.right();
            } else {
                bound.node = current;
                current = node.left();
            }
        }
        return bound;
    }

    Compare compare_{};
    Tree tree_;
};

} // namespace general_functions
