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
        const auto root = tree_.root();

        return root ? tree_.value(*root).subtreeSize : 0;
    }

    bool empty() const noexcept { return tree_.empty(); }

    bool contains(const Key& key) const {
        auto currentNode = tree_.root();

        while (currentNode) {
            const auto& keys = tree_.value(*currentNode).keys;
            const auto position = findLowerBoundIndex(*currentNode, key);

            if (position < keys.size() && !compare_(key, keys[position])) {
                return true;
            }

            if (isLeaf(*currentNode)) {
                return false;
            }

            currentNode = tree_.child(*currentNode, position);
        }

        return false;
    }

    void insert(const Key& key) {
        const auto root = tree_.root();

        if (!root) {
            createRoot(key);
            return;
        }

        if (size() == std::numeric_limits<SizeType>::max()) {
            throw std::length_error("BTree: too many keys");
        }

        if (isFull(*root)) {
            splitRoot();
        }

        insertNonFull(*tree_.root(), key);
    }

    bool validateInvariants() const {
        const auto root = tree_.root();

        if (!root) {
            return tree_.empty();
        }

        ValidationState state;
        state.visited.resize(tree_.size(), false);

        if (!validateNode(*root, nullptr, nullptr, 0, state)) {
            return false;
        }

        return std::all_of(state.visited.begin(),
                           state.visited.end(),
                           [](bool visited) { return visited; });
    }

private:
    static constexpr SizeType MAX_KEYS = 2 * minDegree - 1;

    struct NodeData {
        std::vector<Key> keys;
        SizeType subtreeSize = 0;
    };

    using Tree = MultiwayTree<NodeData>;
    using NodeId = typename Tree::NodeId;

    bool isLeaf(NodeId node) const { return tree_.childCount(node) == 0; }

    bool isFull(NodeId node) const {
        return tree_.value(node).keys.size() == MAX_KEYS;
    }

    SizeType findLowerBoundIndex(NodeId node, const Key& key) const {
        const auto& keys = tree_.value(node).keys;
        const auto position =
            std::lower_bound(keys.begin(), keys.end(), key, std::cref(compare_));

        return static_cast<SizeType>(position - keys.begin());
    }

    SizeType findUpperBoundIndex(NodeId node, const Key& key) const {
        const auto& keys = tree_.value(node).keys;
        const auto position =
            std::upper_bound(keys.begin(), keys.end(), key, std::cref(compare_));

        return static_cast<SizeType>(position - keys.begin());
    }

    void createRoot(const Key& key) {
        NodeData root;
        root.keys.push_back(key);
        root.subtreeSize = 1;

        tree_.addRoot(std::move(root));
    }

    void insertIntoLeaf(NodeId leaf, const Key& key) {
        auto& node = tree_.value(leaf);
        const auto position = findUpperBoundIndex(leaf, key);

        auto updatedKeys = node.keys;
        updatedKeys.insert(updatedKeys.begin() + position, key);

        node.keys.swap(updatedKeys);
        node.subtreeSize = node.keys.size();
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

    SplitData prepareSplit(NodeId node) const {
        const auto& keys = tree_.value(node).keys;
        SplitData split{NodeData{}, NodeData{}, keys[minDegree - 1]};
        split.left.keys.assign(keys.begin(), keys.begin() + minDegree - 1);
        split.right.keys.assign(keys.begin() + minDegree, keys.end());

        split.left.subtreeSize = split.left.keys.size();
        split.right.subtreeSize = split.right.keys.size();

        for (SizeType index = 0; index < tree_.childCount(node); ++index) {
            const auto childSize = tree_.value(tree_.child(node, index)).subtreeSize;

            if (index < minDegree) {
                split.left.subtreeSize += childSize;
            } else {
                split.right.subtreeSize += childSize;
            }
        }

        return split;
    }

    void splitRoot() {
        const NodeId oldRoot = *tree_.root();
        auto split = prepareSplit(oldRoot);

        NodeData newRoot;
        newRoot.keys.push_back(split.median);
        newRoot.subtreeSize = tree_.value(oldRoot).subtreeSize;
        const SizeType firstMovedChild = isLeaf(oldRoot) ? 0 : minDegree;

        auto editor = tree_.editTopology();
        const NodeId newRootId = editor.insertParent(oldRoot, std::move(newRoot));
        const NodeId sibling = editor.addChild(newRootId, 1, std::move(split.right));
        const SizeType movedCount = tree_.childCount(oldRoot) - firstMovedChild;
        editor.moveChildren(oldRoot, firstMovedChild, movedCount, sibling, 0);
        editor.commitChanges();

        tree_.value(oldRoot).keys.swap(split.left.keys);
        tree_.value(oldRoot).subtreeSize = split.left.subtreeSize;
    }

    void splitChild(NodeId parent, SizeType childIndex) {
        const NodeId child = tree_.child(parent, childIndex);
        auto split = prepareSplit(child);

        auto parentKeys = tree_.value(parent).keys;
        parentKeys.insert(parentKeys.begin() + childIndex, split.median);
        const SizeType firstMovedChild = isLeaf(child) ? 0 : minDegree;

        auto editor = tree_.editTopology();
        const NodeId sibling =
            editor.addChild(parent, childIndex + 1, std::move(split.right));
        const SizeType movedCount = tree_.childCount(child) - firstMovedChild;
        editor.moveChildren(child, firstMovedChild, movedCount, sibling, 0);
        editor.commitChanges();

        tree_.value(child).keys.swap(split.left.keys);
        tree_.value(child).subtreeSize = split.left.subtreeSize;
        tree_.value(parent).keys.swap(parentKeys);
    }

    void insertNonFull(NodeId node, const Key& key) {
        if (isLeaf(node)) {
            insertIntoLeaf(node, key);
            return;
        }

        auto childIndex = findUpperBoundIndex(node, key);

        if (isFull(tree_.child(node, childIndex))) {
            splitChild(node, childIndex);

            if (!compare_(key, tree_.value(node).keys[childIndex])) {
                ++childIndex;
            }
        }

        insertNonFull(tree_.child(node, childIndex), key);
        ++tree_.value(node).subtreeSize;
    }

    std::optional<SizeType> validateNode(NodeId node, const Key* lower,
                                         const Key* upper, SizeType depth,
                                         ValidationState& state) const {
        if (node >= state.visited.size() || state.visited[node]) {
            return std::nullopt;
        }

        state.visited[node] = true;
        const auto& data = tree_.value(node);
        const auto& keys = data.keys;

        if (keys.empty() || keys.size() > MAX_KEYS ||
            (node != *tree_.root() && keys.size() < minDegree - 1)) {
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
        if (isLeaf(node)) {
            if (state.leafDepth && *state.leafDepth != depth) {
                return std::nullopt;
            }

            state.leafDepth = depth;
        } else {
            if (tree_.childCount(node) != keys.size() + 1) {
                return std::nullopt;
            }

            for (SizeType index = 0; index <= keys.size(); ++index) {
                const Key* childLower = index == 0 ? lower : &keys[index - 1];
                const Key* childUpper = index == keys.size() ? upper : &keys[index];
                const auto childSize = validateNode(tree_.child(node, index),
                                                    childLower,
                                                    childUpper,
                                                    depth + 1,
                                                    state);

                if (!childSize) {
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
