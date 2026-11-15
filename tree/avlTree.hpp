#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>

#include "binary_tree.hpp"

namespace general_functions {

class AVLTree {
    // BinaryTree owns the nodes and links. AVL only adds these node properties.
    struct NodeData {
        explicit NodeData(int value) : key(value) {}

        int key;
        int height = 1;
        std::size_t subtreeSize = 1;

        friend std::ostream& operator<<(std::ostream& output, const NodeData& node) {
            return output << node.key << "\nheight: " << node.height
                          << "\nsubtree: " << node.subtreeSize;
        }
    };

    using Tree = general_functions::BinaryTree<NodeData>;

public:
    using NodeId = typename Tree::NodeId;
    using KeyType = int;
    // Read-only sorted iterator. Insertion invalidates existing iterators.
    class iterator {
    public:
        using value_type = KeyType;
        using difference_type = std::ptrdiff_t;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;

        iterator() = default;

        reference operator*() const {
            if (!tree_) {
                throw std::logic_error("Tree iterator has no owner");
            }
            return tree_->select(position_);
        }

        pointer operator->() const { return std::addressof(operator*()); }

        iterator& operator++() {
            if (!tree_ || position_ >= tree_->size()) {
                throw std::out_of_range("Cannot increment a tree end iterator");
            }
            ++position_;
            return *this;
        }

        iterator operator++(int) {
            auto previous = *this;
            ++*this;
            return previous;
        }

        friend bool operator==(iterator left, iterator right) {
            return left.tree_ == right.tree_ && left.position_ == right.position_;
        }

        friend bool operator!=(iterator left, iterator right) {
            return !(left == right);
        }

    private:
        friend class AVLTree;

        iterator(const AVLTree* tree, std::size_t position)
            : tree_(tree), position_(position) {}

        const AVLTree* tree_ = nullptr;
        std::size_t position_ = 0;
    };
    using const_iterator = iterator;

    // BinaryTree supplies independent copying, moving and nonrecursive cleanup.
    AVLTree() = default;

    bool insert(int key) {
        if (!tree_.root()) {
            tree_.addRoot(NodeData{key});
            return true;
        }
        return insert(*tree_.root(), key, std::nullopt);
    }

    std::size_t size() const noexcept { return tree_.size(); }

    // All returned iterators are read-only and invalidated by insertion.
    iterator begin() const noexcept { return iterator(this, 0); }
    iterator end() const noexcept { return iterator(this, size()); }

    iterator lower_bound(const KeyType& key) const {
        return iterator(this, boundIndex(key, false));
    }

    iterator upper_bound(const KeyType& key) const {
        return iterator(this, boundIndex(key, true));
    }

    // Signed distance in sorted order, including negative distances.
    std::ptrdiff_t distance(iterator first, iterator last) const {
        if (first.tree_ != this || last.tree_ != this) {
            throw std::invalid_argument("Tree iterators must belong to this tree");
        }
        const auto count = first.position_ < last.position_
                               ? last.position_ - first.position_
                               : first.position_ - last.position_;
        if (count >
            static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max())) {
            throw std::overflow_error("Tree iterator distance is too large");
        }
        const auto result = static_cast<std::ptrdiff_t>(count);
        return first.position_ <= last.position_ ? result : -result;
    }

    // Zero-based sorted index. Values remain read-only to preserve ordering.
    const int& select(std::size_t index) const {
        if (index >= size()) {
            throw std::out_of_range("AVLTree: sorted index out of range");
        }
        auto current = *tree_.root();
        for (;;) {
            const auto leftSize = subtreeSize(tree_.left(current));
            if (index < leftSize) {
                current = *tree_.left(current);
            } else if (index == leftSize) {
                return tree_.value(current).key;
            } else {
                index -= leftSize + 1;
                current = *tree_.right(current);
            }
        }
    }

    // Read-only access for the common BinaryTree dump. Cannot break AVL balance.
    const Tree& tree() const noexcept { return tree_; }

    void swap(AVLTree& other) noexcept { tree_.swap(other.tree_); }

    friend void swap(AVLTree& left, AVLTree& right) noexcept { left.swap(right); }

private:
    int height(std::optional<NodeId> id) const {
        return id ? tree_.value(*id).height : 0;
    }

    std::size_t subtreeSize(std::optional<NodeId> id) const {
        return id ? tree_.value(*id).subtreeSize : 0;
    }

    void update(NodeId id) {
        auto& node = tree_.value(id);
        node.height = 1 + std::max(height(tree_.left(id)), height(tree_.right(id)));
        node.subtreeSize =
            1 + subtreeSize(tree_.left(id)) + subtreeSize(tree_.right(id));
    }

    int balanceFactor(NodeId id) const {
        return height(tree_.left(id)) - height(tree_.right(id));
    }

    void replaceRoot(NodeId oldRoot, NodeId newRoot, std::optional<NodeId> parent) {
        if (!parent) {
            tree_.setRoot(newRoot);
        } else if (tree_.left(*parent) == oldRoot) {
            tree_.setLeft(*parent, newRoot);
        } else {
            tree_.setRight(*parent, newRoot);
        }
    }

    void rotateLeft(NodeId id, std::optional<NodeId> parent) {
        const auto pivot = *tree_.right(id);
        tree_.setRight(id, tree_.left(pivot));
        tree_.setLeft(pivot, id);
        replaceRoot(id, pivot, parent);
        update(id);
        update(pivot);
    }

    void rotateRight(NodeId id, std::optional<NodeId> parent) {
        const auto pivot = *tree_.left(id);
        tree_.setLeft(id, tree_.right(pivot));
        tree_.setRight(pivot, id);
        replaceRoot(id, pivot, parent);
        update(id);
        update(pivot);
    }

    void rebalance(NodeId id, std::optional<NodeId> parent) {
        update(id);
        const auto balance = balanceFactor(id);
        if (balance > 1) {
            const auto left = *tree_.left(id);
            if (balanceFactor(left) < 0) {
                rotateLeft(left, id);
            }
            rotateRight(id, parent);
        } else if (balance < -1) {
            const auto right = *tree_.right(id);
            if (balanceFactor(right) > 0) {
                rotateRight(right, id);
            }
            rotateLeft(id, parent);
        }
    }

    bool insert(NodeId id, int key, std::optional<NodeId> parent) {
        // Keep IDs, not references: inserting into BinaryTree can grow its storage.
        const int currentKey = tree_.value(id).key;
        bool inserted = false;
        if (key < currentKey) {
            const auto left = tree_.left(id);
            if (left) {
                inserted = insert(*left, key, id);
            } else {
                tree_.addLeft(id, NodeData{key});
                inserted = true;
            }
        } else if (key > currentKey) {
            const auto right = tree_.right(id);
            if (right) {
                inserted = insert(*right, key, id);
            } else {
                tree_.addRight(id, NodeData{key});
                inserted = true;
            }
        }
        if (inserted) {
            rebalance(id, parent);
        }
        return inserted;
    }

    std::size_t boundIndex(int key, bool includeEqual) const {
        std::size_t count = 0;
        auto current = tree_.root();
        while (current) {
            const auto& node = tree_.value(*current);
            if (node.key < key || (includeEqual && node.key == key)) {
                count += subtreeSize(tree_.left(*current)) + 1;
                current = tree_.right(*current);
            } else {
                current = tree_.left(*current);
            }
        }
        return count;
    }

    Tree tree_;
};

} // namespace general_functions
