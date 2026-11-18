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
    static constexpr NodeId INVALID_NODE = std::numeric_limits<NodeId>::max();
    class Node;

    NodeId append(T&& value);

    std::vector<Node> nodes_;
    NodeId rootId_ = INVALID_NODE;
public:
    BinaryTree() = default;
    BinaryTree(const BinaryTree&) = default;
    BinaryTree(BinaryTree&& other) noexcept { swap(other); }

    BinaryTree& operator=(const BinaryTree& other);
    BinaryTree& operator=(BinaryTree&& other) noexcept;

    void swap(BinaryTree& other) noexcept;

public:
    using NodeId = std::uint32_t;
    bool empty() const noexcept { return nodes_.empty(); }
    std::size_t size() const noexcept { return nodes_.size(); }

    void reserve(std::size_t count);

    NodeId root() const noexcept { return rootId_; }

    NodeId addRoot(T value);
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
typename BinaryTree<T>::NodeId BinaryTree<T>::addRoot(T value) {
    if (!empty()) {
        throw std::logic_error("BinaryTree: root already exists");
    }
    rootId_ = append(std::move(value));
    return rootId_;
}

template <typename T>
typename BinaryTree<T>::NodeId BinaryTree<T>::append(T&& value) {
    if (nodes_.size() >= INVALID_NODE) {
        throw std::length_error("BinaryTree: too many nodes");
    }
    const auto id = static_cast<NodeId>(nodes_.size());
    nodes_.emplace_back(std::move(value));
    return id;
}

} // namespace general_functions
