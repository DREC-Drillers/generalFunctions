#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
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

    BinaryTree() = default;
    BinaryTree(const BinaryTree&) = default;
    BinaryTree(BinaryTree&& other) noexcept { swap(other); }

    BinaryTree& operator=(const BinaryTree& other);
    BinaryTree& operator=(BinaryTree&& other) noexcept;

    void swap(BinaryTree& other) noexcept;

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
