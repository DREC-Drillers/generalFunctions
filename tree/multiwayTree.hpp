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
class MultiwayTree {
public:
    using NodeId = std::uint32_t;

    MultiwayTree() = default;
    MultiwayTree(const MultiwayTree&) = default;
    MultiwayTree(MultiwayTree&&) noexcept = default;

    MultiwayTree& operator=(const MultiwayTree& other) {
        if (this != &other) {
            MultiwayTree copy(other);
            swap(copy);
        }
        return *this;
    }

    MultiwayTree& operator=(MultiwayTree&&) noexcept = default;

    void swap(MultiwayTree& other) noexcept { nodes_.swap(other.nodes_); }

    friend void swap(MultiwayTree& left, MultiwayTree& right) noexcept {
        left.swap(right);
    }

    bool empty() const noexcept { return nodes_.empty(); }
    std::size_t size() const noexcept { return nodes_.size(); }

    void reserve(std::size_t count) {
        if (count > INVALID_NODE) {
            throw std::length_error("MultiwayTree: too many nodes");
        }
        nodes_.reserve(count);
    }

    std::optional<NodeId> root() const noexcept {
        return empty() ? std::nullopt : std::optional<NodeId>{0};
    }

    NodeId addRoot(T value) {
        if (!empty()) {
            throw std::logic_error("MultiwayTree: root already exists");
        }

        return append(std::move(value));
    }

    NodeId addChild(NodeId parent, std::size_t childIndex, T value) {
        const auto& children = nodes_.at(parent).children;
        if (childIndex > children.size()) {
            throw std::out_of_range("MultiwayTree: child index out of range");
        }

        auto updatedChildren = children;
        updatedChildren.insert(updatedChildren.begin() + childIndex, INVALID_NODE);

        const NodeId child = append(std::move(value));
        updatedChildren[childIndex] = child;
        nodes_.at(parent).children.swap(updatedChildren);
        return child;
    }

    NodeId child(NodeId node, std::size_t childIndex) const {
        return nodes_.at(node).children.at(childIndex);
    }

    std::size_t childCount(NodeId node) const {
        return nodes_.at(node).children.size();
    }

    T& value(NodeId node) { return nodes_.at(node).value; }
    const T& value(NodeId node) const { return nodes_.at(node).value; }

private:
    static constexpr NodeId INVALID_NODE = std::numeric_limits<NodeId>::max();

    struct Node {
        explicit Node(T&& data) : value(std::move(data)) {}

        T value;
        std::vector<NodeId> children;
    };

    NodeId append(T&& value) {
        if (nodes_.size() >= INVALID_NODE) {
            throw std::length_error("MultiwayTree: too many nodes");
        }

        const auto id = static_cast<NodeId>(nodes_.size());
        nodes_.emplace_back(std::move(value));

        return id;
    }

    std::vector<Node> nodes_;
};

} // namespace general_functions
