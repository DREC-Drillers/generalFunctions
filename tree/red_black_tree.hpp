#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "binary_tree.hpp"

namespace red_black_tree {

template <typename keyT, typename Comp>
class redBlackTree_t {
private:
    Comp comp_;
    enum class nodeColor_t : std::uint8_t {RED, BLACK};
    enum class ChildSide : std::uint8_t {LEFT, RIGHT};
    enum class KeyInclusion : std::uint8_t {EXCLUDE_EQUAL, INCLUDE_EQUAL};

    class NodeData {
    private:
        keyT key_;
        nodeColor_t color_ = nodeColor_t::RED;
        std::size_t subtreeSize_ = 1;

    public:
        explicit NodeData(keyT&& key) : key_(std::move(key)) {}

        const keyT& key() const noexcept { return key_; }

        nodeColor_t color() const noexcept { return color_; }
        void setColor(nodeColor_t color) noexcept { color_ = color; }

        std::size_t subtreeSize() const noexcept { return subtreeSize_; }
        void setSubtreeSize(std::size_t size) noexcept { subtreeSize_ = size; }
    };

    using Tree = general_functions::BinaryTree<NodeData>;
    using NodeId = typename Tree::NodeId;

    Tree tree_;
    static constexpr NodeId nil() { return Tree::INVALID_NODE; }

    nodeColor_t colorOf(NodeId node) const;
    std::size_t subtreeSize(NodeId node) const;
    std::size_t countKeysUpTo(const keyT& key, KeyInclusion inclusion) const;
    void updateSubtreeSize(NodeId node);
    void updateAncestorSizes(NodeId node);
    void rotateLeft(NodeId node);
    void rotateRight(NodeId node);
    NodeId grandparent(NodeId node) const;
    NodeId uncle(NodeId node) const;
    NodeId sibling(NodeId node, NodeId parent) const;
    NodeId nearNephew(NodeId node, NodeId parent) const;
    NodeId farNephew(NodeId node, NodeId parent) const;

    NodeId findInSubtree(const keyT& key, NodeId subTreeRoot) const;
    NodeId successor(NodeId node) const;
    void insertInSubtree(keyT&& key, NodeId subtreeRoot, NodeId parent, ChildSide side);
    void insertCase1(NodeId node);
    void insertCase2(NodeId node);
    void insertCase3(NodeId node);
    void insertCase4(NodeId node);
    void insertCase5(NodeId node);
    NodeId treeMaxKeyNode(NodeId node) const;
    NodeId treeMinKeyNode(NodeId node) const;
    std::tuple<NodeId, NodeId, nodeColor_t> removeNodeWithTwoChildren(NodeId rmNode);
    void deleteFixup(NodeId extraBlackNode, NodeId parent);
    void rotateParent(NodeId node, NodeId parent);
    void rotateSibling(NodeId node, NodeId parent);
    void deleteNode(NodeId rmNode);

public:
    explicit redBlackTree_t(Comp comp = Comp{});
    ~redBlackTree_t() = default;

    redBlackTree_t(const redBlackTree_t&) = delete;
    redBlackTree_t& operator=(const redBlackTree_t&) = delete;
    redBlackTree_t(redBlackTree_t&&) = delete;
    redBlackTree_t& operator=(redBlackTree_t&&) = delete;

public:
    std::size_t size() const noexcept { return tree_.size(); }
    const keyT& select(std::size_t index) const;

    std::size_t countKeysLessThan(const keyT& key) const;
    std::size_t countKeysLessThanOrEqual(const keyT& key) const;

    std::optional<keyT> lowerBound(const keyT& key) const;
    std::optional<keyT> upperBound(const keyT& key) const;

public:
    void insert(keyT key);
    void erase(const keyT& key);
    bool find(const keyT& key) const;
};

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::redBlackTree_t(Comp comp)
    : comp_(std::move(comp)) {}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::nodeColor_t
redBlackTree_t<keyT, Comp>::colorOf(NodeId nodeId) const {
    return nodeId == nil() ? nodeColor_t::BLACK :
                             tree_.getNode(nodeId).data().color();
}

template <typename keyT, typename Comp>
std::size_t redBlackTree_t<keyT, Comp>::subtreeSize(NodeId nodeId) const {
    return nodeId == nil() ? 0 : tree_.getNode(nodeId).data().subtreeSize();
}

template <typename keyT, typename Comp>
void redBlackTree_t<keyT, Comp>::updateSubtreeSize(NodeId nodeId) {
    auto& node = tree_.getNode(nodeId);
    node.data().setSubtreeSize(1 + subtreeSize(node.left()) + subtreeSize(node.right()));
}

template <typename keyT, typename Comp>
void redBlackTree_t<keyT, Comp>::updateAncestorSizes(NodeId nodeId) {
    while (nodeId != nil()) {
        updateSubtreeSize(nodeId);
        nodeId = tree_.getNode(nodeId).parent();
    }
}

template <typename keyT, typename Comp>
void redBlackTree_t<keyT, Comp>::rotateLeft(NodeId nodeId) {
    tree_.rotateLeft(nodeId);
    updateSubtreeSize(nodeId);
    updateSubtreeSize(tree_.getNode(nodeId).parent());
}

template <typename keyT, typename Comp>
void redBlackTree_t<keyT, Comp>::rotateRight(NodeId nodeId) {
    tree_.rotateRight(nodeId);
    updateSubtreeSize(nodeId);
    updateSubtreeSize(tree_.getNode(nodeId).parent());
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::NodeId
redBlackTree_t<keyT, Comp>::grandparent(NodeId nodeId) const {
    const auto& node = tree_.getNode(nodeId);
    const NodeId parentId = node.parent();
    return parentId == nil() ? nil() : tree_.getNode(parentId).parent();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::NodeId
redBlackTree_t<keyT, Comp>::uncle(NodeId nodeId) const {
    const NodeId grandparentId = grandparent(nodeId);
    if (grandparentId == nil()) {
        return nil();
    }

    const auto& node = tree_.getNode(nodeId);
    const auto& grandparent = tree_.getNode(grandparentId);
    return node.parent() == grandparent.left() ? grandparent.right() :
                                                 grandparent.left();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::NodeId
redBlackTree_t<keyT, Comp>::sibling(NodeId nodeId, NodeId parentId) const {
    if (parentId == nil()) {
        return nil();
    }

    const auto& parent = tree_.getNode(parentId);
    return nodeId == parent.left() ? parent.right() : parent.left();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::NodeId
redBlackTree_t<keyT, Comp>::nearNephew(NodeId nodeId, NodeId parentId) const {
    const NodeId siblingId = sibling(nodeId, parentId);
    if (siblingId == nil()) {
        return nil();
    }

    const auto& sibling = tree_.getNode(siblingId);
    const auto& parent = tree_.getNode(parentId);

    return nodeId == parent.left() ? sibling.left() : sibling.right();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::NodeId
redBlackTree_t<keyT, Comp>::farNephew(NodeId nodeId, NodeId parentId) const {
    const NodeId siblingId = sibling(nodeId, parentId);
    if (siblingId == nil()) {
        return nil();
    }

    const auto& sibling = tree_.getNode(siblingId);
    const auto& parent = tree_.getNode(parentId);
    return nodeId == parent.left() ? sibling.right() : sibling.left();
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase1(NodeId nodeId) {
    auto& node = tree_.getNode(nodeId);
    if (node.parent() == nil()) {
        node.data().setColor(nodeColor_t::BLACK);
    } else {
        insertCase2(nodeId);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase2(NodeId nodeId) {
    const auto& node = tree_.getNode(nodeId);
    if (colorOf(node.parent()) == nodeColor_t::BLACK) {
        return;
    }
    insertCase3(nodeId);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase3(NodeId nodeId) {
    const auto& node = tree_.getNode(nodeId);

    const NodeId uncleId = uncle(nodeId);
    const NodeId grandparentId = grandparent(nodeId);

    auto& parent = tree_.getNode(node.parent());
    auto& grandparent = tree_.getNode(grandparentId);

    if (colorOf(uncleId) == nodeColor_t::RED) {
        auto& uncle = tree_.getNode(uncleId);

        parent.data().setColor(nodeColor_t::BLACK);
        uncle.data().setColor(nodeColor_t::BLACK);
        grandparent.data().setColor(nodeColor_t::RED);

        insertCase1(grandparentId);
    } else {
        insertCase4(nodeId);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase4(NodeId nodeId) {
    const auto& node = tree_.getNode(nodeId);

    const NodeId parentId = node.parent();
    const auto& parent = tree_.getNode(parentId);

    const auto& grandparent = tree_.getNode(parent.parent());

    if (nodeId == parent.right() && parentId == grandparent.left()) {
        rotateLeft(parentId);
        nodeId = node.left();
    } else if (nodeId == parent.left() && parentId == grandparent.right()) {
        rotateRight(parentId);
        nodeId = node.right();
    }

    insertCase5(nodeId);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase5(NodeId nodeId) {
    const auto& node = tree_.getNode(nodeId);

    const NodeId parentId = node.parent();
    auto& parent = tree_.getNode(parentId);

    const NodeId grandparentId = parent.parent();
    auto& grandparent = tree_.getNode(grandparentId);

    parent.data().setColor(nodeColor_t::BLACK);
    grandparent.data().setColor(nodeColor_t::RED);

    if (nodeId == parent.left() && parentId == grandparent.left()) {
        rotateRight(grandparentId);
    } else {
        rotateLeft(grandparentId);
    }
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::NodeId
redBlackTree_t<keyT, Comp>::findInSubtree(const keyT& key, NodeId subTreeRootId) const {
    if (subTreeRootId == nil()) {
        return nil();
    }
    const auto& subTreeRoot = tree_.getNode(subTreeRootId);
    if (comp_(key, subTreeRoot.data().key())) {
        return findInSubtree(key, subTreeRoot.left());
    }
    if (comp_(subTreeRoot.data().key(), key)) {
        return findInSubtree(key, subTreeRoot.right());
    }
    return subTreeRootId;
}

template <typename keyT, typename Comp>
bool
redBlackTree_t<keyT, Comp>::find(const keyT& key) const {
    return findInSubtree(key, tree_.root()) != nil();
}

template <typename keyT, typename Comp>
std::size_t redBlackTree_t<keyT, Comp>::countKeysLessThan(const keyT& key) const {
    return countKeysUpTo(key, KeyInclusion::EXCLUDE_EQUAL);
}

template <typename keyT, typename Comp>
std::size_t redBlackTree_t<keyT, Comp>::countKeysLessThanOrEqual(const keyT& key) const {
    return countKeysUpTo(key, KeyInclusion::INCLUDE_EQUAL);
}

template <typename keyT, typename Comp>
std::size_t redBlackTree_t<keyT, Comp>::countKeysUpTo(
    const keyT& key, KeyInclusion inclusion) const {
    std::size_t count = 0;
    NodeId currentId = tree_.root();

    while (currentId != nil()) {
        const auto& current = tree_.getNode(currentId);
        const auto& currentKey = current.data().key();

        const bool countCurrent = (inclusion == KeyInclusion::INCLUDE_EQUAL)
                                ? !comp_(key, currentKey)
                                : comp_(currentKey, key);
        if (countCurrent) {
            count += 1 + subtreeSize(current.left());
            currentId = current.right();
        } else {
            currentId = current.left();
        }
    }
    return count;
}

template <typename keyT, typename Comp>
const keyT& redBlackTree_t<keyT, Comp>::select(std::size_t index) const {
    if (index >= size()) {
        throw std::out_of_range("RBTree: sorted index out of range");
    }

    NodeId currentId = tree_.root();
    while (true) {
        const auto& current = tree_.getNode(currentId);
        const auto leftSubtreeSize = subtreeSize(current.left());

        if (index < leftSubtreeSize) {
            currentId = current.left();
        } else if (index == leftSubtreeSize) {
            return current.data().key();
        } else {
            index -= leftSubtreeSize + 1;
            currentId = current.right();
        }
    }
}

template <typename keyT, typename Comp>
std::optional<keyT>
redBlackTree_t<keyT, Comp>::lowerBound(const keyT& key) const {
    NodeId currentId = tree_.root();
    NodeId resultId = nil();
    while (currentId != nil()) {
        const auto& current = tree_.getNode(currentId);
        if (comp_(current.data().key(), key)) {
            currentId = current.right();
        } else {
            resultId = currentId;
            currentId = current.left();
        }
    }
    if (resultId == nil()) {
        return std::nullopt;
    }
    return tree_.getNode(resultId).data().key();
}

template <typename keyT, typename Comp>
std::optional<keyT>
redBlackTree_t<keyT, Comp>::upperBound(const keyT& key) const {
    NodeId currentId = tree_.root();
    NodeId resultId = nil();
    while (currentId != nil()) {
        const auto& current = tree_.getNode(currentId);
        if (comp_(key, current.data().key())) {
            resultId = currentId;
            currentId = current.left();
        } else {
            currentId = current.right();
        }
    }
    if (resultId == nil()) {
        return std::nullopt;
    }
    return tree_.getNode(resultId).data().key();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::NodeId
redBlackTree_t<keyT, Comp>::successor(NodeId nodeId) const {
    if (nodeId == nil()) {
        return nil();
    }
    const auto& node = tree_.getNode(nodeId);
    if (node.right() != nil()) {
        return treeMinKeyNode(node.right());
    }

    NodeId parentId = node.parent();
    while (parentId != nil()) {
        const auto& parent = tree_.getNode(parentId);
        if (nodeId != parent.right()) {
            break;
        }
        nodeId = parentId;
        parentId = parent.parent();
    }
    return parentId;
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertInSubtree(
    keyT&& key, NodeId subtreeRootId, NodeId parentId, ChildSide side) {
    if (subtreeRootId == nil()) {
        NodeId newNodeId;
        if (parentId == nil()) {
            newNodeId = tree_.addRoot(NodeData{std::move(key)});
        } else {
            newNodeId = tree_.allocateNode(NodeData{std::move(key)});
            auto& newNode = tree_.getNode(newNodeId);
            auto& parent = tree_.getNode(parentId);
            newNode.setParent(parentId);
            if (side == ChildSide::LEFT) {
                parent.setLeft(newNodeId);
            } else {
                parent.setRight(newNodeId);
            }
        }
        updateAncestorSizes(parentId);
        insertCase1(newNodeId);
        return;
    }

    const auto& subtreeRoot = tree_.getNode(subtreeRootId);
    if (comp_(key, subtreeRoot.data().key())) {
        return insertInSubtree(std::move(key), subtreeRoot.left(), subtreeRootId, ChildSide::LEFT);
    }
    if (comp_(subtreeRoot.data().key(), key)) {
        return insertInSubtree(std::move(key), subtreeRoot.right(), subtreeRootId, ChildSide::RIGHT);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insert(keyT key) {
    return insertInSubtree(std::move(key), tree_.root(), nil(), ChildSide::LEFT);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::erase(const keyT& key) {
    const NodeId rmNodeId = findInSubtree(key, tree_.root());
    deleteNode(rmNodeId);
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::NodeId
redBlackTree_t<keyT, Comp>::treeMaxKeyNode(NodeId nodeId) const {
    if (nodeId == nil()) {
        return nil();
    }
    const auto& node = tree_.getNode(nodeId);
    return node.right() == nil() ? nodeId : treeMaxKeyNode(node.right());
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::NodeId
redBlackTree_t<keyT, Comp>::treeMinKeyNode(NodeId nodeId) const {
    if (nodeId == nil()) {
        return nil();
    }
    const auto& node = tree_.getNode(nodeId);
    return node.left() == nil() ? nodeId : treeMinKeyNode(node.left());
}

template <typename keyT, typename Comp>
std::tuple<typename redBlackTree_t<keyT, Comp>::NodeId,
           typename redBlackTree_t<keyT, Comp>::NodeId,
           typename redBlackTree_t<keyT, Comp>::nodeColor_t>
redBlackTree_t<keyT, Comp>::removeNodeWithTwoChildren(NodeId rmNodeId) {
    const auto& rmNode = tree_.getNode(rmNodeId);
    const NodeId successorId = treeMinKeyNode(rmNode.right());
    auto& successor = tree_.getNode(successorId);

    const nodeColor_t removableNodeOriginalColor = successor.data().color();
    const NodeId replacementId = successor.right();
    const NodeId replacementParentId = successor.parent();

    if (successorId != rmNode.right()) {
        tree_.transplant(successorId, replacementId);
        successor.setRight(rmNode.right());
        tree_.getNode(rmNode.right()).setParent(successorId);
    }

    tree_.transplant(rmNodeId, successorId);
    successor.setLeft(rmNode.left());
    tree_.getNode(successor.left()).setParent(successorId);
    successor.data().setColor(rmNode.data().color());

    return {replacementId, successorId == rmNode.right() ? successorId : replacementParentId,
            removableNodeOriginalColor};
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::deleteFixup(NodeId extraBlackNodeId, NodeId parentId) {
    while (extraBlackNodeId != tree_.root() && colorOf(extraBlackNodeId) == nodeColor_t::BLACK) {
        auto& parent = tree_.getNode(parentId);
        NodeId siblingId = sibling(extraBlackNodeId, parentId);

        if (colorOf(siblingId) == nodeColor_t::RED) {
            tree_.getNode(siblingId).data().setColor(nodeColor_t::BLACK);
            parent.data().setColor(nodeColor_t::RED);
            rotateParent(extraBlackNodeId, parentId);
            siblingId = sibling(extraBlackNodeId, parentId);
        }

        const NodeId nearNephewId = nearNephew(extraBlackNodeId, parentId);
        NodeId farNephewId = farNephew(extraBlackNodeId, parentId);
        if (colorOf(nearNephewId) == nodeColor_t::BLACK &&
            colorOf(farNephewId) == nodeColor_t::BLACK) {
            tree_.getNode(siblingId).data().setColor(nodeColor_t::RED);
            extraBlackNodeId = parentId;
            parentId = parent.parent();
        } else {
            if (colorOf(farNephewId) == nodeColor_t::BLACK) {
                tree_.getNode(nearNephewId).data().setColor(nodeColor_t::BLACK);
                tree_.getNode(siblingId).data().setColor(nodeColor_t::RED);

                rotateSibling(extraBlackNodeId, parentId);
                siblingId = sibling(extraBlackNodeId, parentId);
                farNephewId = farNephew(extraBlackNodeId, parentId);
            }
            tree_.getNode(siblingId).data().setColor(parent.data().color());
            parent.data().setColor(nodeColor_t::BLACK);
            tree_.getNode(farNephewId).data().setColor(nodeColor_t::BLACK);

            rotateParent(extraBlackNodeId, parentId);

            extraBlackNodeId = tree_.root();
        }
    }
    if (extraBlackNodeId != nil()) {
        tree_.getNode(extraBlackNodeId).data().setColor(nodeColor_t::BLACK);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::rotateParent(NodeId nodeId, NodeId parentId) {
    if (parentId == nil()) {
        return;
    }

    const auto& parent = tree_.getNode(parentId);
    if (nodeId == parent.left()) {
        rotateLeft(parentId);
    } else {
        rotateRight(parentId);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::rotateSibling(NodeId nodeId, NodeId parentId) {
    const NodeId siblingId = sibling(nodeId, parentId);
    if (siblingId == nil()) {
        return;
    }

    const auto& parent = tree_.getNode(parentId);
    if (nodeId == parent.left()) {
        rotateRight(siblingId);
    } else {
        rotateLeft(siblingId);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::deleteNode(NodeId rmNodeId) {
    if (rmNodeId == nil()) {
        return;
    }
    auto& rmNode = tree_.getNode(rmNodeId);
    nodeColor_t removableNodeOriginalColor = rmNode.data().color();

    NodeId replacementId = nil();
    NodeId replacementParentId = rmNode.parent();

    tree_.prepareRelease();

    if (rmNode.left() == nil()) {
        replacementId = rmNode.right();
        tree_.transplant(rmNodeId, replacementId);
    } else if (rmNode.right() == nil()) {
        replacementId = rmNode.left();
        tree_.transplant(rmNodeId, replacementId);
    } else {
        std::tie(replacementId, replacementParentId, removableNodeOriginalColor) =
            removeNodeWithTwoChildren(rmNodeId);
    }

    updateAncestorSizes(replacementParentId);
    if (removableNodeOriginalColor == nodeColor_t::BLACK) {
        deleteFixup(replacementId, replacementParentId);
    }

    rmNode.setLeft(nil());
    rmNode.setRight(nil());
    tree_.releaseNode(rmNodeId);
}

} // namespace red_black_tree
