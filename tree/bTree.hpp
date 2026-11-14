#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "multiwayTree.hpp"

namespace general_functions {

template <typename Key, std::size_t minDegree, typename Compare = std::less<Key>>
class BTree {
    static_assert(minDegree >= 2, "BTree minimum degree must be at least 2");
    static_assert(minDegree <= std::numeric_limits<std::size_t>::max() / 2,
                  "BTree minimum degree is too large");

public:
    using KeyType = Key;
    using SizeType = std::size_t;

    BTree() = default;
    explicit BTree(Compare compare) : compare_(std::move(compare)) {}
    BTree(const BTree&) = default;

    BTree& operator=(const BTree&) = delete;
    BTree(BTree&&) = delete;
    BTree& operator=(BTree&&) = delete;

    SizeType size() const noexcept {
        const auto rootId = tree_.root();

        return rootId ? tree_.value(*rootId).subtreeSize : 0;
    }

    bool empty() const noexcept { return tree_.empty(); }

    bool contains(const Key& key) const {
        auto currentNodeId = tree_.root();

        while (currentNodeId) {
            const auto& keys = tree_.value(*currentNodeId).keys;
            const auto lowerBoundIndex = findLowerBoundIndex(*currentNodeId, key);

            if (lowerBoundIndex < keys.size() &&
                !compare_(key, keys[lowerBoundIndex])) {
                return true;
            }

            if (isLeaf(*currentNodeId)) {
                return false;
            }

            currentNodeId = tree_.child(*currentNodeId, lowerBoundIndex);
        }

        return false;
    }

    void insert(const Key& key) {
        const auto rootId = tree_.root();

        if (!rootId) {
            createRoot(key);
            return;
        }

        if (size() == std::numeric_limits<SizeType>::max()) {
            throw std::length_error("BTree: too many keys");
        }

        if (isFull(*rootId)) {
            splitRoot();
        }

        insertNonFull(*tree_.root(), key);
    }

    bool validateInvariants() const {
        const auto rootId = tree_.root();

        if (!rootId) {
            return tree_.empty();
        }

        ValidationState state;
        state.visited.resize(tree_.size(), false);

        if (!validateNode(*rootId, nullptr, nullptr, 0, state) ||
            tree_.parent(*rootId)) {
            return false;
        }

        return std::all_of(state.visited.begin(),
                           state.visited.end(),
                           [](bool visited) { return visited; });
    }

private:
    static constexpr SizeType MIN_KEYS = minDegree - 1;
    static constexpr SizeType MAX_KEYS = 2 * minDegree - 1;
    static constexpr SizeType MEDIAN_KEY_INDEX = MIN_KEYS;
    static constexpr SizeType ROOT_RIGHT_CHILD_INDEX = 1;

    struct NodeData {
        std::vector<Key> keys;
        SizeType subtreeSize = 0;
    };

    using Tree = MultiwayTree<NodeData>;
    using NodeId = typename Tree::NodeId;

    bool isLeaf(NodeId nodeId) const { return tree_.childCount(nodeId) == 0; }

    bool isFull(NodeId nodeId) const {
        return tree_.value(nodeId).keys.size() == MAX_KEYS;
    }

    SizeType findLowerBoundIndex(NodeId nodeId, const Key& key) const {
        const auto& keys = tree_.value(nodeId).keys;
        const auto boundIt =
            std::lower_bound(keys.begin(), keys.end(), key, std::cref(compare_));

        return static_cast<SizeType>(boundIt - keys.begin());
    }

    SizeType findUpperBoundIndex(NodeId nodeId, const Key& key) const {
        const auto& keys = tree_.value(nodeId).keys;
        const auto boundIt =
            std::upper_bound(keys.begin(), keys.end(), key, std::cref(compare_));

        return static_cast<SizeType>(boundIt - keys.begin());
    }

    void createRoot(const Key& key) {
        NodeData rootData;
        rootData.keys.push_back(key);
        rootData.subtreeSize = 1;

        tree_.addRoot(std::move(rootData));
    }

    void insertIntoLeaf(NodeId leafId, const Key& key) {
        auto& leafData = tree_.value(leafId);
        const auto insertionIndex = findUpperBoundIndex(leafId, key);

        auto updatedKeys = leafData.keys;
        updatedKeys.insert(updatedKeys.begin() + insertionIndex, key);

        leafData.keys.swap(updatedKeys);
        leafData.subtreeSize = leafData.keys.size();
    }

    struct SplitData {
        NodeData left;
        NodeData right;
        Key median;
    };

    struct ValidationState {
        std::vector<bool> visited;
        std::optional<SizeType> leafDepth;
    };

    SplitData prepareSplit(NodeId nodeId) const {
        const auto& keys = tree_.value(nodeId).keys;
        SplitData split{NodeData{}, NodeData{}, keys[MEDIAN_KEY_INDEX]};
        split.left.keys.assign(keys.begin(), keys.begin() + MEDIAN_KEY_INDEX);
        split.right.keys.assign(keys.begin() + MEDIAN_KEY_INDEX + 1, keys.end());

        split.left.subtreeSize = split.left.keys.size();
        split.right.subtreeSize = split.right.keys.size();

        for (SizeType childIndex = 0; childIndex < tree_.childCount(nodeId);
             ++childIndex) {
            const auto childSubtreeSize =
                tree_.value(tree_.child(nodeId, childIndex)).subtreeSize;

            if (childIndex < minDegree) {
                split.left.subtreeSize += childSubtreeSize;
            } else {
                split.right.subtreeSize += childSubtreeSize;
            }
        }

        return split;
    }

    void splitRoot() {
        const NodeId oldRootId = *tree_.root();
        auto split = prepareSplit(oldRootId);

        NodeData newRootData;
        newRootData.keys.push_back(split.median);
        newRootData.subtreeSize = tree_.value(oldRootId).subtreeSize;
        const SizeType firstRightChildIndex = isLeaf(oldRootId) ? 0 : minDegree;

        auto editor = tree_.editTopology();
        const NodeId newRootId =
            editor.insertParent(oldRootId, std::move(newRootData));
        const NodeId siblingId = editor.addChild(
            newRootId, ROOT_RIGHT_CHILD_INDEX, std::move(split.right));
        const SizeType rightChildCount =
            tree_.childCount(oldRootId) - firstRightChildIndex;
        editor.moveChildren(
            oldRootId, firstRightChildIndex, rightChildCount, siblingId, 0);
        editor.commitChanges();

        tree_.value(oldRootId).keys.swap(split.left.keys);
        tree_.value(oldRootId).subtreeSize = split.left.subtreeSize;
    }

    void splitChild(NodeId parentId, SizeType childIndex) {
        const NodeId childId = tree_.child(parentId, childIndex);
        auto split = prepareSplit(childId);

        auto parentKeys = tree_.value(parentId).keys;
        parentKeys.insert(parentKeys.begin() + childIndex, split.median);
        const SizeType firstRightChildIndex = isLeaf(childId) ? 0 : minDegree;

        auto editor = tree_.editTopology();
        const NodeId siblingId =
            editor.addChild(parentId, childIndex + 1, std::move(split.right));
        const SizeType rightChildCount =
            tree_.childCount(childId) - firstRightChildIndex;
        editor.moveChildren(
            childId, firstRightChildIndex, rightChildCount, siblingId, 0);
        editor.commitChanges();

        tree_.value(childId).keys.swap(split.left.keys);
        tree_.value(childId).subtreeSize = split.left.subtreeSize;
        tree_.value(parentId).keys.swap(parentKeys);
    }

    void insertNonFull(NodeId nodeId, const Key& key) {
        if (isLeaf(nodeId)) {
            insertIntoLeaf(nodeId, key);
            return;
        }

        auto childIndex = findUpperBoundIndex(nodeId, key);

        if (isFull(tree_.child(nodeId, childIndex))) {
            splitChild(nodeId, childIndex);

            if (!compare_(key, tree_.value(nodeId).keys[childIndex])) {
                ++childIndex;
            }
        }

        insertNonFull(tree_.child(nodeId, childIndex), key);
        ++tree_.value(nodeId).subtreeSize;
    }

    std::optional<SizeType> validateNode(NodeId nodeId, const Key* lower,
                                         const Key* upper, SizeType depth,
                                         ValidationState& state) const {
        if (nodeId >= state.visited.size() || state.visited[nodeId]) {
            return std::nullopt;
        }

        state.visited[nodeId] = true;
        const auto& data = tree_.value(nodeId);
        const auto& keys = data.keys;

        if (keys.empty() || keys.size() > MAX_KEYS ||
            (nodeId != *tree_.root() && keys.size() < MIN_KEYS)) {
            return std::nullopt;
        }

        if (!std::is_sorted(keys.begin(), keys.end(), std::cref(compare_))) {
            return std::nullopt;
        }

        for (const auto& key : keys) {
            if ((lower && compare_(key, *lower)) ||
                (upper && compare_(*upper, key))) {
                return std::nullopt;
            }
        }

        SizeType countedKeys = keys.size();
        if (isLeaf(nodeId)) {
            if (state.leafDepth && *state.leafDepth != depth) {
                return std::nullopt;
            }

            state.leafDepth = depth;
        } else {
            if (tree_.childCount(nodeId) != keys.size() + 1) {
                return std::nullopt;
            }

            for (SizeType index = 0; index <= keys.size(); ++index) {
                const Key* childLower = index == 0 ? lower : &keys[index - 1];
                const Key* childUpper = index == keys.size() ? upper : &keys[index];
                const NodeId childId = tree_.child(nodeId, index);
                const auto childSize =
                    validateNode(childId, childLower, childUpper, depth + 1, state);

                if (!childSize || tree_.parent(childId) != nodeId) {
                    return std::nullopt;
                }

                countedKeys += *childSize;
            }
        }

        if (countedKeys != data.subtreeSize) {
            return std::nullopt;
        }

        return countedKeys;
    }

    Compare compare_{};
    Tree tree_;
};

} // namespace general_functions
