#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace general_functions {

// Node IDs remain valid after insertion. References to values may be
// invalidated by insertion or reserve. IDs belong to this tree, not to other
// instances. Nodes are stored in insertion order; the builder controls
// traversal locality.
template <typename T>
class BinaryTree {
public:
    using NodeId = std::uint32_t;

    bool empty() const noexcept { return nodes_.empty(); }
    std::size_t size() const noexcept { return nodes_.size(); }

    void reserve(std::size_t count) {
        if (count > INVALID_NODE) {
            throw std::length_error("BinaryTree: too many nodes");
        }
        nodes_.reserve(count);
    }

    std::optional<NodeId> root() const noexcept {
        return empty() ? std::nullopt : std::optional<NodeId>{0};
    }

    NodeId addRoot(T value) {
        if (!empty()) {
            throw std::logic_error("BinaryTree: root already exists");
        }
        return append(std::move(value));
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
};

} // namespace general_functions
