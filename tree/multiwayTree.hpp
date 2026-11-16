#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace general_functions {

template <typename T>
class MultiwayTree {
    struct Node;

    static_assert(std::is_copy_constructible_v<T> ||
                      std::is_nothrow_move_constructible_v<T>,
                  "MultiwayTree values must be copyable or nothrow movable");

public:
    using NodeId = std::uint32_t;

    class TopologyEditor {
    public:
        explicit TopologyEditor(MultiwayTree& tree)
            : tree_(tree), initialTopologyVersion_(tree.topologyVersion_),
              initialNodeCount_(tree.nodes_.size()), rootId_(tree.rootId_) {}

        TopologyEditor(const TopologyEditor&) = delete;
        TopologyEditor& operator=(const TopologyEditor&) = delete;
        TopologyEditor(TopologyEditor&&) = delete;
        TopologyEditor& operator=(TopologyEditor&&) = delete;

        NodeId addRoot(T value) {
            return executeOperation([&] {
                if (rootId_ != INVALID_NODE) {
                    throw std::logic_error("MultiwayTree: root already exists");
                }

                rootId_ = stageNode(std::move(value), INVALID_NODE);
                return rootId_;
            });
        }

        NodeId addChild(NodeId parentId, std::size_t childIndex, T value) {
            return executeOperation([&] {
                auto updatedChildIds = getChildIds(parentId);

                if (childIndex > updatedChildIds.size()) {
                    throw std::out_of_range(
                        "MultiwayTree: child index out of range");
                }

                const NodeId nodeId = stageNode(std::move(value), parentId);
                updatedChildIds.insert(updatedChildIds.begin() + childIndex, nodeId);
                stageChildIds(parentId, std::move(updatedChildIds));

                return nodeId;
            });
        }

        NodeId insertParent(NodeId nodeId, T value) {
            return executeOperation([&] {
                const NodeId oldParentId = getParentId(nodeId);
                const NodeId newParentId = stageNode(std::move(value), oldParentId);
                pendingNodes_.back().childIds.push_back(nodeId);

                if (oldParentId == INVALID_NODE) {
                    rootId_ = newParentId;
                } else {
                    auto updatedParentChildIds = getChildIds(oldParentId);
                    const auto position = std::find(updatedParentChildIds.begin(),
                                                    updatedParentChildIds.end(),
                                                    nodeId);
                    *position = newParentId;
                    stageChildIds(oldParentId, std::move(updatedParentChildIds));
                }

                stageParentId(nodeId, newParentId);
                return newParentId;
            });
        }

        void moveChildren(NodeId sourceParentId, std::size_t firstChild,
                          std::size_t count, NodeId destinationParentId,
                          std::size_t destinationIndex) {
            executeOperation([&] {
                auto sourceChildIds = getChildIds(sourceParentId);
                const auto& destinationChildIds = getChildIds(destinationParentId);

                if (firstChild > sourceChildIds.size() ||
                    count > sourceChildIds.size() - firstChild) {
                    throw std::out_of_range(
                        "MultiwayTree: child range out of range");
                }

                const auto destinationSize = sourceParentId == destinationParentId
                                                 ? sourceChildIds.size() - count
                                                 : destinationChildIds.size();

                if (destinationIndex > destinationSize) {
                    throw std::out_of_range(
                        "MultiwayTree: destination index out of range");
                }

                if (count == 0) {
                    return;
                }

                const auto first = sourceChildIds.begin() + firstChild;
                std::vector<NodeId> movedChildIds(first, first + count);

                if (sourceParentId != destinationParentId) {
                    ensureMoveDoesNotCreateCycle(destinationParentId, movedChildIds);
                }

                sourceChildIds.erase(first, first + count);

                if (sourceParentId == destinationParentId) {
                    sourceChildIds.insert(sourceChildIds.begin() + destinationIndex,
                                          movedChildIds.begin(),
                                          movedChildIds.end());
                    stageChildIds(sourceParentId, std::move(sourceChildIds));
                    return;
                }

                auto updatedDestinationChildIds = destinationChildIds;
                updatedDestinationChildIds.insert(
                    updatedDestinationChildIds.begin() + destinationIndex,
                    movedChildIds.begin(),
                    movedChildIds.end());

                for (NodeId nodeId : movedChildIds) {
                    stageParentId(nodeId, destinationParentId);
                }

                stageChildIds(sourceParentId, std::move(sourceChildIds));
                stageChildIds(destinationParentId,
                              std::move(updatedDestinationChildIds));
            });
        }

        void commitChanges() {
            executeOperation([&] {
                const auto requiredSize = initialNodeCount_ + pendingNodes_.size();
                tree_.ensureCapacity(requiredSize);

                try {
                    for (auto& pendingNode : pendingNodes_) {
                        tree_.nodes_.push_back(std::move_if_noexcept(pendingNode));
                    }
                } catch (...) {
                    while (tree_.nodes_.size() > initialNodeCount_) {
                        tree_.nodes_.pop_back();
                    }

                    throw;
                }

                for (auto& change : pendingChildIds_) {
                    tree_.nodes_[change.first].childIds.swap(change.second);
                }

                for (const auto& change : pendingParentIds_) {
                    tree_.nodes_[change.first].parentId = change.second;
                }

                tree_.rootId_ = rootId_;
                ++tree_.topologyVersion_;
                closed_ = true;
            });
        }

    private:
        template <typename Operation>
        auto executeOperation(Operation&& operation) {
            ensureEditorIsActive();

            try {
                return operation();
            } catch (...) {
                closed_ = true;
                throw;
            }
        }

        void ensureEditorIsActive() const {
            if (closed_) {
                throw std::logic_error("MultiwayTree: topology edit is closed");
            }

            if (tree_.topologyVersion_ != initialTopologyVersion_) {
                throw std::logic_error(
                    "MultiwayTree: tree changed during topology edit");
            }
        }

        void validateNodeId(NodeId nodeId) const {
            if (nodeId >= initialNodeCount_ + pendingNodes_.size()) {
                throw std::out_of_range("MultiwayTree: node ID out of range");
            }
        }

        const std::vector<NodeId>& getChildIds(NodeId nodeId) const {
            validateNodeId(nodeId);

            if (nodeId >= initialNodeCount_) {
                return pendingNodes_[nodeId - initialNodeCount_].childIds;
            }

            const auto change = pendingChildIds_.find(nodeId);

            if (change != pendingChildIds_.end()) {
                return change->second;
            }

            return tree_.nodes_[nodeId].childIds;
        }

        NodeId getParentId(NodeId nodeId) const {
            validateNodeId(nodeId);

            if (nodeId >= initialNodeCount_) {
                return pendingNodes_[nodeId - initialNodeCount_].parentId;
            }

            const auto change = pendingParentIds_.find(nodeId);

            if (change != pendingParentIds_.end()) {
                return change->second;
            }

            return tree_.nodes_[nodeId].parentId;
        }

        NodeId stageNode(T&& value, NodeId parentId) {
            const auto index = initialNodeCount_ + pendingNodes_.size();

            if (index >= INVALID_NODE) {
                throw std::length_error("MultiwayTree: too many nodes");
            }

            pendingNodes_.emplace_back(std::move(value), parentId);
            return static_cast<NodeId>(index);
        }

        void stageChildIds(NodeId nodeId, std::vector<NodeId> childIds) {
            if (nodeId >= initialNodeCount_) {
                pendingNodes_[nodeId - initialNodeCount_].childIds =
                    std::move(childIds);
            } else {
                pendingChildIds_.insert_or_assign(nodeId, std::move(childIds));
            }
        }

        void stageParentId(NodeId nodeId, NodeId parentId) {
            if (nodeId >= initialNodeCount_) {
                pendingNodes_[nodeId - initialNodeCount_].parentId = parentId;
            } else {
                pendingParentIds_.insert_or_assign(nodeId, parentId);
            }
        }

        void ensureMoveDoesNotCreateCycle(NodeId destinationParentId,
                                          const std::vector<NodeId>& moved) const {
            auto ancestorId = destinationParentId;

            while (ancestorId != INVALID_NODE) {
                if (std::find(moved.begin(), moved.end(), ancestorId) !=
                    moved.end()) {
                    throw std::invalid_argument(
                        "MultiwayTree: moving children would create a cycle");
                }

                ancestorId = getParentId(ancestorId);
            }
        }

        MultiwayTree& tree_;
        std::uint64_t initialTopologyVersion_;
        std::size_t initialNodeCount_;
        NodeId rootId_;
        std::vector<Node> pendingNodes_;
        std::map<NodeId, std::vector<NodeId>> pendingChildIds_;
        std::map<NodeId, NodeId> pendingParentIds_;
        bool closed_ = false;
    };

    MultiwayTree() = default;

    MultiwayTree(const MultiwayTree& other)
        : nodes_(other.nodes_), rootId_(other.rootId_) {}

    MultiwayTree(MultiwayTree&& other) noexcept
        : nodes_(std::move(other.nodes_)),
          rootId_(std::exchange(other.rootId_, INVALID_NODE)) {
        other.nodes_.clear();
        ++other.topologyVersion_;
    }

    MultiwayTree& operator=(const MultiwayTree& other) {
        if (this != &other) {
            MultiwayTree copy(other);
            swap(copy);
        }

        return *this;
    }

    MultiwayTree& operator=(MultiwayTree&& other) noexcept {
        if (this != &other) {
            nodes_ = std::move(other.nodes_);
            rootId_ = std::exchange(other.rootId_, INVALID_NODE);
            other.nodes_.clear();
            ++topologyVersion_;
            ++other.topologyVersion_;
        }

        return *this;
    }

    void swap(MultiwayTree& other) noexcept {
        if (this != &other) {
            nodes_.swap(other.nodes_);
            std::swap(rootId_, other.rootId_);
            ++topologyVersion_;
            ++other.topologyVersion_;
        }
    }

    friend void swap(MultiwayTree& left, MultiwayTree& right) noexcept {
        left.swap(right);
    }

    bool empty() const noexcept { return nodes_.empty(); }
    std::size_t size() const noexcept { return nodes_.size(); }

    void clear() noexcept {
        nodes_.clear();
        rootId_ = INVALID_NODE;
        ++topologyVersion_;
    }

    void reserve(std::size_t count) {
        if (count > INVALID_NODE) {
            throw std::length_error("MultiwayTree: too many nodes");
        }

        nodes_.reserve(count);
        ++topologyVersion_;
    }

    std::optional<NodeId> root() const noexcept { return optionalId(rootId_); }

    std::optional<NodeId> parent(NodeId nodeId) const {
        return optionalId(nodes_.at(nodeId).parentId);
    }

    NodeId child(NodeId nodeId, std::size_t childIndex) const {
        return nodes_.at(nodeId).childIds.at(childIndex);
    }

    std::size_t childCount(NodeId nodeId) const {
        return nodes_.at(nodeId).childIds.size();
    }

    T& value(NodeId nodeId) { return nodes_.at(nodeId).value; }
    const T& value(NodeId nodeId) const { return nodes_.at(nodeId).value; }

    TopologyEditor editTopology() { return TopologyEditor(*this); }

    NodeId addRoot(T value) {
        auto editor = editTopology();
        const NodeId nodeId = editor.addRoot(std::move_if_noexcept(value));
        editor.commitChanges();

        return nodeId;
    }

    NodeId addChild(NodeId parentId, std::size_t childIndex, T value) {
        auto editor = editTopology();
        const NodeId nodeId =
            editor.addChild(parentId, childIndex, std::move_if_noexcept(value));
        editor.commitChanges();

        return nodeId;
    }

    NodeId insertParent(NodeId nodeId, T value) {
        auto editor = editTopology();
        const NodeId newParentId =
            editor.insertParent(nodeId, std::move_if_noexcept(value));
        editor.commitChanges();

        return newParentId;
    }

    void moveChildren(NodeId sourceParentId, std::size_t firstChild,
                      std::size_t count, NodeId destinationParentId,
                      std::size_t destinationIndex) {
        auto editor = editTopology();
        editor.moveChildren(sourceParentId,
                            firstChild,
                            count,
                            destinationParentId,
                            destinationIndex);
        editor.commitChanges();
    }

    bool validateInvariants() const {
        if (empty()) {
            return rootId_ == INVALID_NODE;
        }

        if (rootId_ >= nodes_.size() || nodes_[rootId_].parentId != INVALID_NODE) {
            return false;
        }

        std::vector<bool> visited(nodes_.size(), false);
        std::vector<NodeId> pending{rootId_};

        while (!pending.empty()) {
            const NodeId nodeId = pending.back();
            pending.pop_back();

            if (nodeId >= nodes_.size() || visited[nodeId]) {
                return false;
            }

            visited[nodeId] = true;

            for (NodeId childId : nodes_[nodeId].childIds) {
                if (childId >= nodes_.size() || nodes_[childId].parentId != nodeId) {
                    return false;
                }

                pending.push_back(childId);
            }
        }

        return std::all_of(
            visited.begin(), visited.end(), [](bool value) { return value; });
    }

private:
    static constexpr NodeId INVALID_NODE = std::numeric_limits<NodeId>::max();

    struct Node {
        Node(T&& data, NodeId parentNodeId)
            : value(std::move_if_noexcept(data)), parentId(parentNodeId) {}

        T value;
        NodeId parentId;
        std::vector<NodeId> childIds;
    };

    static std::optional<NodeId> optionalId(NodeId nodeId) noexcept {
        return nodeId == INVALID_NODE ? std::nullopt : std::optional<NodeId>{nodeId};
    }

    void ensureCapacity(std::size_t count) {
        if (count <= nodes_.capacity()) {
            return;
        }

        const auto capacity = nodes_.capacity();
        const auto grownCapacity = capacity > INVALID_NODE / 2
                                       ? static_cast<std::size_t>(INVALID_NODE)
                                       : std::max(count, capacity * 2);
        nodes_.reserve(grownCapacity);
    }

    std::vector<Node> nodes_;
    NodeId rootId_ = INVALID_NODE;
    std::uint64_t topologyVersion_ = 0;
};

} // namespace general_functions
