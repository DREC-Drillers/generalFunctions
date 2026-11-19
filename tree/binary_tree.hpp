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
public:
    using NodeId = std::uint32_t;

    BinaryTree() = default;
    BinaryTree(const BinaryTree&) = default;
    BinaryTree(BinaryTree&& other) noexcept { swap(other); }

    // Strong exception guarantee: failed copying leaves this tree unchanged.
    BinaryTree& operator=(const BinaryTree& other) {
        if (this != &other) {
            BinaryTree copy(other);
            swap(copy);
        }
        return *this;
    }

    BinaryTree& operator=(BinaryTree&& other) noexcept {
        if (this != &other) {
            BinaryTree moved(std::move(other));
            swap(moved);
        }
        return *this;
    }

    void swap(BinaryTree& other) noexcept {
        nodes_.swap(other.nodes_);
        std::swap(rootId_, other.rootId_);
    }

    friend void swap(BinaryTree& left, BinaryTree& right) noexcept {
        left.swap(right);
    }

    bool empty() const noexcept { return nodes_.empty(); }
    std::size_t size() const noexcept { return nodes_.size(); }

    void reserve(std::size_t count) {
        if (count > INVALID_NODE) {
            throw std::length_error("BinaryTree: too many nodes");
        }
        nodes_.reserve(count);
    }

    std::optional<NodeId> root() const noexcept { return childId(rootId_); }

    NodeId addRoot(T value) {
        if (!empty()) {
            throw std::logic_error("BinaryTree: root already exists");
        }
        rootId_ = append(std::move(value));
        return rootId_;
    }

    NodeId addLeft(NodeId parent, T value) {
        if (nodes_.at(parent).left != INVALID_NODE) {
            throw std::logic_error("BinaryTree: left child already exists");
        }
        const NodeId child = append(std::move(value));
        nodes_.at(parent).left = child;
        return child;
    }

    NodeId addRight(NodeId parent, T value) {
        if (nodes_.at(parent).right != INVALID_NODE) {
            throw std::logic_error("BinaryTree: right child already exists");
        }
        const NodeId child = append(std::move(value));
        nodes_.at(parent).right = child;
        return child;
    }

    std::optional<NodeId> left(NodeId node) const {
        return childId(nodes_.at(node).left);
    }

    std::optional<NodeId> right(NodeId node) const {
        return childId(nodes_.at(node).right);
    }

    void setRoot(NodeId node) {
        checkNodeId(node);
        rootId_ = node;
    }

    void setLeft(NodeId node, std::optional<NodeId> child) {
        checkNodeId(node);
        if (child.has_value()) {
            checkNodeId(child.value());
        }
        nodes_[node].left = child.value_or(INVALID_NODE);
    }

    void setRight(NodeId node, std::optional<NodeId> child) {
        checkNodeId(node);
        if (child.has_value()) {
            checkNodeId(child.value());
        }
        nodes_[node].right = child.value_or(INVALID_NODE);
    }

    T& value(NodeId node) { return nodes_.at(node).value; }
    const T& value(NodeId node) const { return nodes_.at(node).value; }

private:
    static constexpr NodeId INVALID_NODE = std::numeric_limits<NodeId>::max();

    struct Node {
        explicit Node(T&& data) : value(std::move(data)) {}

        T value;
        NodeId left = INVALID_NODE;
        NodeId right = INVALID_NODE;
    };

    void checkNodeId(NodeId node) const {
        if (node >= nodes_.size()) {
            throw std::out_of_range("BinaryTree: node ID out of range");
        }
    }

    static std::optional<NodeId> childId(NodeId child) noexcept {
        return child == INVALID_NODE ? std::nullopt : std::optional<NodeId>{child};
    }

    NodeId append(T&& value) {
        if (nodes_.size() >= INVALID_NODE) {
            throw std::length_error("BinaryTree: too many nodes");
        }
        const auto id = static_cast<NodeId>(nodes_.size());
        nodes_.emplace_back(std::move(value));
        return id;
    }

    std::vector<Node> nodes_;
    NodeId rootId_ = INVALID_NODE;
};

} // namespace general_functions
