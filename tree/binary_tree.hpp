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

private:
    static constexpr NodeId INVALID_NODE = std::numeric_limits<NodeId>::max();

    class Node {
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
