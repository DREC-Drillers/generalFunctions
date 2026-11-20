#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace general_functions {

template <typename T>
class BinaryTree {
private:
    class Node;

    std::vector<std::optional<Node>> nodes_;
    std::vector<std::uint32_t> freeNodes_;
    std::uint32_t rootId_ = std::numeric_limits<std::uint32_t>::max();

public:
    using NodeId = std::uint32_t;
    static constexpr NodeId INVALID_NODE = std::numeric_limits<NodeId>::max();

    enum class TraversalOrder { InOrder, PreOrder, PostOrder };

    template <bool IsConst>
    class Iterator;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    template <bool IsConst>
    class Traversal;

    BinaryTree() = default;
    BinaryTree(const BinaryTree&) = default;
    BinaryTree(BinaryTree&& other) noexcept { swap(other); }

    BinaryTree& operator=(const BinaryTree& other);
    BinaryTree& operator=(BinaryTree&& other) noexcept;

    void swap(BinaryTree& other) noexcept;

    iterator begin() { return traverse(TraversalOrder::InOrder).begin(); }
    iterator end() { return traverse(TraversalOrder::InOrder).end(); }
    const_iterator begin() const { return traverse(TraversalOrder::InOrder).begin(); }
    const_iterator end() const { return traverse(TraversalOrder::InOrder).end(); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    Traversal<false> traverse(TraversalOrder order) & {
        return Traversal<false>(this, order);
    }

    Traversal<true> traverse(TraversalOrder order) const & {
        return Traversal<true>(this, order);
    }

    Traversal<true> traverse(TraversalOrder order) const && = delete;

    bool empty() const noexcept { return size() == 0; }
    std::size_t size() const noexcept { return nodes_.size() - freeNodes_.size(); }

    void reserve(std::size_t count);

    NodeId root() const noexcept { return rootId_; }
    Node& getNode(NodeId node);
    const Node& getNode(NodeId node) const;

    NodeId addRoot(T value);
    NodeId allocateNode(T&& value);
    // Reserve room to release one node before changing any links.
    void prepareRelease();
    void releaseNode(NodeId node);

    void rotateLeft(NodeId node);
    void rotateRight(NodeId node);

    void transplant(NodeId replaceableNodeId, NodeId newNodeId);

};

template <typename T>
template <bool IsConst>
class BinaryTree<T>::Iterator {
private:
    using Tree = std::conditional_t<IsConst, const BinaryTree, BinaryTree>;

    Tree* tree_ = nullptr;
    NodeId node_ = INVALID_NODE;
    TraversalOrder order_ = TraversalOrder::InOrder;

    Iterator(Tree* tree, NodeId node, TraversalOrder order)
        : tree_(tree), node_(node), order_(order) {}

    friend class Traversal<IsConst>;
    template <bool>
    friend class Iterator;

    enum class Side { Left, Right };

    static Side opposite(Side side) {
        return side == Side::Left ? Side::Right : Side::Left;
    }

    NodeId child(NodeId id, Side side) const {
        const auto& node = tree_->getNode(id);
        return side == Side::Left ? node.left() : node.right();
    }

    NodeId extremeNode(NodeId id, Side side) const {
        while (id != INVALID_NODE) {
            const auto next = child(id, side);
            if (next == INVALID_NODE) {
                break;
            }

            id = next;
        }

        return id;
    }

    NodeId leafPreferring(NodeId id, Side preferredSide) const {
        while (id != INVALID_NODE) {
            auto next = child(id, preferredSide);
            if (next == INVALID_NODE) {
                next = child(id, opposite(preferredSide));
            }

            if (next == INVALID_NODE) {
                break;
            }

            id = next;
        }

        return id;
    }

    NodeId inOrderStep(Side subtreeSide) const {
        const auto subtree = child(node_, subtreeSide);
        if (subtree != INVALID_NODE) {
            return extremeNode(subtree, opposite(subtreeSide));
        }

        auto current = node_;
        auto parent = tree_->getNode(current).parent();
        while (parent != INVALID_NODE && child(parent, subtreeSide) == current) {
            current = parent;
            parent = tree_->getNode(current).parent();
        }

        return parent;
    }

    // Reverse post-order is pre-order with left and right exchanged.
    NodeId preOrderNext(Side firstSide) const {
        auto current = node_;
        const auto secondSide = opposite(firstSide);

        if (const auto first = child(current, firstSide); first != INVALID_NODE) {
            return first;
        }
        if (const auto second = child(current, secondSide); second != INVALID_NODE) {
            return second;
        }

        auto parent = tree_->getNode(current).parent();
        while (parent != INVALID_NODE) {
            const auto sibling = child(parent, secondSide);
            if (child(parent, firstSide) == current && sibling != INVALID_NODE) {
                return sibling;
            }

            current = parent;
            parent = tree_->getNode(current).parent();
        }

        return INVALID_NODE;
    }

    NodeId preOrderPrevious(Side firstSide) const {
        const auto secondSide = opposite(firstSide);
        const auto parent = tree_->getNode(node_).parent();

        if (parent != INVALID_NODE && child(parent, secondSide) == node_) {
            const auto sibling = child(parent, firstSide);
            if (sibling != INVALID_NODE) {
                return leafPreferring(sibling, secondSide);
            }
        }

        return parent;
    }

    static Iterator first(Tree* tree, TraversalOrder order) {
        Iterator it(tree, tree->root(), order);

        switch (order) {
            case TraversalOrder::InOrder:
                it.node_ = it.extremeNode(it.node_, Side::Left);
                break;
            case TraversalOrder::PreOrder:
                break;
            case TraversalOrder::PostOrder:
                it.node_ = it.leafPreferring(it.node_, Side::Left);
                break;
        }

        return it;
    }

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;

    Iterator() = default;

    template <bool Other, std::enable_if_t<IsConst && !Other, int> = 0>
    Iterator(const Iterator<Other>& other)
        : tree_(other.tree_), node_(other.node_), order_(other.order_) {}

    reference operator*() const { return tree_->getNode(node_).data(); }
    pointer operator->() const { return std::addressof(operator*()); }

    Iterator& operator++() {
        switch (order_) {
            case TraversalOrder::InOrder:
                node_ = inOrderStep(Side::Right);
                break;
            case TraversalOrder::PreOrder:
                node_ = preOrderNext(Side::Left);
                break;
            case TraversalOrder::PostOrder:
                node_ = preOrderPrevious(Side::Right);
                break;
            }

        return *this;
    }

    Iterator operator++(int) { auto old = *this; ++*this; return old; }

    Iterator& operator--() {
        if (node_ == INVALID_NODE) {
            node_ = tree_->root();

            if (order_ == TraversalOrder::InOrder) {
                node_ = extremeNode(node_, Side::Right);
            } else if (order_ == TraversalOrder::PreOrder) {
                node_ = leafPreferring(node_, Side::Right);
            }
        }
        else {
            switch (order_) {
                case TraversalOrder::InOrder:
                    node_ = inOrderStep(Side::Left);
                    break;
                case TraversalOrder::PreOrder:
                    node_ = preOrderPrevious(Side::Left);
                    break;
                case TraversalOrder::PostOrder:
                    node_ = preOrderNext(Side::Right);
                    break;
            }
        }

        return *this;
    }

    Iterator operator--(int) { auto old = *this; --*this; return old; }

    template <bool Other>
    bool operator==(const Iterator<Other>& other) const {
        return tree_ == other.tree_ && node_ == other.node_ && order_ == other.order_;
    }

    template <bool Other>
    bool operator!=(const Iterator<Other>& other) const { return !(*this == other); }
};

template <typename T>
template <bool IsConst>
class BinaryTree<T>::Traversal {
private:
    using Tree = std::conditional_t<IsConst, const BinaryTree, BinaryTree>;
    Tree* tree_;
    TraversalOrder order_;

    Traversal(Tree* tree, TraversalOrder order) : tree_(tree), order_(order) {}
    friend class BinaryTree;

public:
    Iterator<IsConst> begin() const { return Iterator<IsConst>::first(tree_, order_); }
    Iterator<IsConst> end() const { return Iterator<IsConst>(tree_, INVALID_NODE, order_); }
};

template <typename T>
class BinaryTree<T>::Node {
public:
    explicit Node(T&& data) : data_(std::move(data)) {}

    NodeId left() const { return left_; }
    NodeId right() const { return right_; }
    NodeId parent() const { return parent_; }
    T& data() { return data_; }
    const T& data() const { return data_; }

    void setLeft(NodeId left) { left_ = left; }
    void setRight(NodeId right) { right_ = right; }
    void setParent(NodeId parent) { parent_ = parent; }

private:
    T data_;
    NodeId left_ = INVALID_NODE;
    NodeId right_ = INVALID_NODE;
    NodeId parent_ = INVALID_NODE;
};

template <typename T>
BinaryTree<T>& BinaryTree<T>::operator=(const BinaryTree& other) {
    if (this != &other) {
        BinaryTree copy(other);
        swap(copy);
    }
    return *this;
}

template <typename T>
BinaryTree<T>& BinaryTree<T>::operator=(BinaryTree&& other) noexcept {
    if (this != &other) {
        BinaryTree moved(std::move(other));
        swap(moved);
    }
    return *this;
}

template <typename T>
void BinaryTree<T>::swap(BinaryTree& other) noexcept {
    nodes_.swap(other.nodes_);
    freeNodes_.swap(other.freeNodes_);
    std::swap(rootId_, other.rootId_);
}

template <typename T>
void BinaryTree<T>::reserve(std::size_t count) {
    if (count > INVALID_NODE) {
        throw std::length_error("BinaryTree: too many nodes");
    }
    nodes_.reserve(count);
}

template <typename T>
typename BinaryTree<T>::Node& BinaryTree<T>::getNode(NodeId node) {
    auto& slot = nodes_.at(node);
    if (!slot) {
        throw std::out_of_range("BinaryTree: node slot is empty");
    }
    return *slot;
}

template <typename T>
const typename BinaryTree<T>::Node& BinaryTree<T>::getNode(NodeId node) const {
    const auto& slot = nodes_.at(node);
    if (!slot) {
        throw std::out_of_range("BinaryTree: node slot is empty");
    }
    return *slot;
}

template <typename T>
typename BinaryTree<T>::NodeId BinaryTree<T>::addRoot(T value) {
    if (rootId_ != INVALID_NODE) {
        throw std::logic_error("BinaryTree: root already exists");
    }
    rootId_ = allocateNode(std::move(value));
    return rootId_;
}

template <typename T>
typename BinaryTree<T>::NodeId BinaryTree<T>::allocateNode(T&& value) {
    if (!freeNodes_.empty()) {
        const NodeId id = freeNodes_.back();
        nodes_[id].emplace(std::move(value));
        freeNodes_.pop_back();
        return id;
    }

    if (nodes_.size() >= INVALID_NODE) {
        throw std::length_error("BinaryTree: too many nodes");
    }
    const auto id = static_cast<NodeId>(nodes_.size());
    nodes_.emplace_back(std::in_place, std::move(value));
    return id;
}

template <typename T>
void BinaryTree<T>::prepareRelease() {
    const auto capacity = freeNodes_.capacity();
    if (freeNodes_.size() < capacity) {
        return;
    }

    const auto maxCapacity = freeNodes_.max_size();
    if (capacity == maxCapacity) {
        throw std::length_error("BinaryTree: too many free nodes");
    }
    if (capacity == 0) {
        freeNodes_.reserve(1);
    } else {
        freeNodes_.reserve(capacity <= maxCapacity / 2 ? capacity * 2 : maxCapacity);
    }
}

template <typename T>
void BinaryTree<T>::releaseNode(NodeId nodeId) {
    const auto& node = getNode(nodeId);
    if (node.parent() != INVALID_NODE || node.left() != INVALID_NODE ||
        node.right() != INVALID_NODE) {
        throw std::logic_error("BinaryTree: unlink node before releasing it");
    }

    freeNodes_.push_back(nodeId);
    nodes_[nodeId].reset();
    if (rootId_ == nodeId) {
        rootId_ = INVALID_NODE;
    }
}

template <typename T>
void BinaryTree<T>::rotateLeft(NodeId nodeId) {
    auto& node = getNode(nodeId);
    const NodeId pivotId = node.right();
    auto& pivot = getNode(pivotId);

    const NodeId middleId = pivot.left();

    transplant(nodeId, pivotId);
    node.setRight(middleId);
    if (middleId != INVALID_NODE) {
        getNode(middleId).setParent(nodeId);
    }

    node.setParent(pivotId);
    pivot.setLeft(nodeId);
}

template <typename T>
void BinaryTree<T>::rotateRight(NodeId nodeId) {
    auto& node = getNode(nodeId);
    const NodeId pivotId = node.left();
    auto& pivot = getNode(pivotId);

    const NodeId middleId = pivot.right();

    transplant(nodeId, pivotId);
    node.setLeft(middleId);
    if (middleId != INVALID_NODE) {
        getNode(middleId).setParent(nodeId);
    }

    node.setParent(pivotId);
    pivot.setRight(nodeId);
}

template <typename T>
void BinaryTree<T>::transplant(NodeId replaceableNodeId, NodeId newNodeId) {
    if (replaceableNodeId == newNodeId) {
        return;
    }
    auto& replaceableNode = getNode(replaceableNodeId);
    auto* newNode = newNodeId == INVALID_NODE ? nullptr : &getNode(newNodeId);

    const NodeId parentId = replaceableNode.parent();

    if (replaceableNodeId == rootId_) {
        rootId_ = newNodeId;
    } else {
        auto& parent = getNode(parentId);
        if (parent.left() == replaceableNodeId) {
            parent.setLeft(newNodeId);
        } else if (parent.right() == replaceableNodeId) {
            parent.setRight(newNodeId);
        } else {
            throw std::logic_error("BinaryTree: parent does not link to node");
        }
    }

    if (newNode) {
        newNode->setParent(parentId);
    }
    replaceableNode.setParent(INVALID_NODE);
}

} // namespace general_functions
