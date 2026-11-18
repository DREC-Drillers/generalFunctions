#pragma once
#include <memory>
#include <optional>
#include <utility>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace red_black_tree {

template <typename keyT, typename Comp>
class redBlackTree_t {
private:
    Comp comp_;
    enum class nodeColor_t {RED, BLACK};
    enum class ChildSide {LEFT, RIGHT};
    class node_t;
    using iterator = node_t*;

    std::unique_ptr<node_t> nil_;
    iterator root_;
    iterator nil() const { return nil_.get(); }

    void destroySubtree(iterator node);
    iterator findInSubtree(const keyT& key, iterator subTreeRoot) const;
    iterator successor(iterator node) const;
    void insertInSubtree(keyT&& key, iterator subtreeRoot, iterator parent, ChildSide side);
    void rotateLeft(iterator node);
    void rotateRight(iterator node);
    void insertCase1(iterator node);
    void insertCase2(iterator node);
    void insertCase3(iterator node);
    void insertCase4(iterator node);
    void insertCase5(iterator node);
    iterator treeMaxKeyNode(iterator node);
    iterator treeMinKeyNode(iterator node);
    void transplant(iterator replaceableNode, iterator newNode);
    std::pair<iterator, nodeColor_t> removeNodeWithTwoChildren(iterator rmNode);
    void deleteFixup(iterator extraBlackNode);
    void rotateParent(iterator node);
    void rotateSibling(iterator node);
    void deleteNode(iterator rmNode);

public:
    explicit redBlackTree_t(Comp comp = Comp{});
    ~redBlackTree_t();

    redBlackTree_t(const redBlackTree_t&) = delete;
    redBlackTree_t& operator=(const redBlackTree_t&) = delete;
    redBlackTree_t(redBlackTree_t&&) = delete;
    redBlackTree_t& operator=(redBlackTree_t&&) = delete;

public:
    std::optional<keyT> lowerBound(const keyT& key) const;
    std::optional<keyT> upperBound(const keyT& key) const;

public:
    void insert(keyT key);
    void erase(const keyT& key);
    bool find(const keyT& key) const;
};

} // namespace red_black_tree


namespace red_black_tree {

template <typename keyT, typename Comp>
class redBlackTree_t<keyT, Comp>::node_t {
private:
    std::optional<keyT> key_;
    nodeColor_t color_;
    iterator parent_;
    iterator left_;
    iterator right_;

public:
    node_t()
        : color_(nodeColor_t::BLACK), parent_(this), left_(this), right_(this) {}

    node_t(keyT&& key, iterator parent, iterator nil)
        : key_(std::move(key)), color_(nodeColor_t::RED), parent_(parent),
          left_(nil), right_(nil) {}

    bool isNil() const { return !key_.has_value(); }
    const keyT& key() const { return key_.value(); }
    nodeColor_t color() const { return color_; }
    iterator parent() const { return parent_; }
    iterator left() const { return left_; }
    iterator right() const { return right_; }

    iterator grandparent() const;
    iterator uncle() const;
    iterator sibling() const;
    iterator nearNephew() const;
    iterator farNephew() const;

    void setParent(iterator parent) { parent_ = parent; }
    void setLeft(iterator left) { left_ = left; }
    void setRight(iterator right) { right_ = right; }
    void setColor(nodeColor_t color) { color_ = color; }
};

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::redBlackTree_t(Comp comp)
    : comp_(std::move(comp)), nil_(std::make_unique<node_t>()), root_(nil()) {}

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::~redBlackTree_t() {
    destroySubtree(root_);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::destroySubtree(iterator node) {
    if (node == nil()) {
        return;
    }
    destroySubtree(node->left());
    destroySubtree(node->right());
    delete node;
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::node_t::grandparent() const {
    return parent_->isNil() ? parent_ : parent_->parent();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::node_t::uncle() const {
    iterator grandparent = this->grandparent();
    if (grandparent->isNil()) {
        return grandparent;
    }

    return parent_ == grandparent->left() ? grandparent->right() : grandparent->left();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::node_t::sibling() const {
    if (parent_->isNil()) {
        return parent_;
    }

    return this == parent_->left() ? parent_->right() : parent_->left();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::node_t::nearNephew() const {
    iterator sibling = this->sibling();
    return this == parent_->left() ? sibling->left() : sibling->right();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::node_t::farNephew() const {
    iterator sibling = this->sibling();
    return this == parent_->left() ? sibling->right() : sibling->left();
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::rotateLeft(iterator node) {

    iterator pivot = node->right();

    pivot->setParent(node->parent());
    if (node->parent() != nil()) {
        if (node->parent()->left() == node)
            node->parent()->setLeft(pivot);
        else
            node->parent()->setRight(pivot);
    } else {
        root_ = pivot;
    }

    node->setRight(pivot->left());
    if (pivot->left() != nil()) {
        pivot->left()->setParent(node);
    }

    node->setParent(pivot);
    pivot->setLeft(node);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::rotateRight(iterator node) {

    iterator pivot = node->left();

    pivot->setParent(node->parent());
    if (node->parent() != nil()) {
        if (node->parent()->left() == node)
            node->parent()->setLeft(pivot);
        else
            node->parent()->setRight(pivot);
    } else {
        root_ = pivot;
    }

    node->setLeft(pivot->right());
    if (pivot->right() != nil()) {
        pivot->right()->setParent(node);
    }

    node->setParent(pivot);
    pivot->setRight(node);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase1(iterator node) {
    if (node->parent() == nil()) {
        node->setColor(nodeColor_t::BLACK);
    } else {
        insertCase2(node);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase2(iterator node) {
    if (node->parent()->color() == nodeColor_t::BLACK) {
        return;
    }
    insertCase3(node);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase3(iterator node) {
    iterator uncle = node->uncle();
    iterator grandparent = node->grandparent();
    iterator parent = node->parent();

    if (uncle->color() == nodeColor_t::RED) {
        parent->setColor(nodeColor_t::BLACK);
        uncle->setColor(nodeColor_t::BLACK);

        grandparent->setColor(nodeColor_t::RED);
        insertCase1(grandparent);
    } else {
        insertCase4(node);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase4(iterator node) {
    iterator grandparent = node->grandparent();
    iterator parent = node->parent();

    if (node == parent->right() && parent == grandparent->left()) {
        rotateLeft(parent);
        node = node->left();
    } else if (node == parent->left() && parent == grandparent->right()) {
        rotateRight(parent);
        node = node->right();
    }

    insertCase5(node);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertCase5(iterator node) {
    iterator grandparent = node->grandparent();
    iterator parent = node->parent();

    parent->setColor(nodeColor_t::BLACK);
    grandparent->setColor(nodeColor_t::RED);

    if (node == parent->left() && parent == grandparent->left()) {
        rotateRight(grandparent);
    } else {
        rotateLeft(grandparent);
    }
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::findInSubtree(const keyT& key, iterator subTreeRoot) const {
    if (subTreeRoot == nil()) {
        return nil();
    }
    if (comp_(key, subTreeRoot->key())) {
        return findInSubtree(key, subTreeRoot->left());
    }
    if (comp_(subTreeRoot->key(), key)) {
        return findInSubtree(key, subTreeRoot->right());
    }
    return subTreeRoot;
}

template <typename keyT, typename Comp>
bool
redBlackTree_t<keyT, Comp>::find(const keyT& key) const {
    return findInSubtree(key, root_) != nil();
}

template <typename keyT, typename Comp>
std::optional<keyT>
redBlackTree_t<keyT, Comp>::lowerBound(const keyT& key) const {
    iterator current = root_;
    iterator result = nil();
    while (current != nil()) {
        if (comp_(current->key(), key)) {
            current = current->right();
        } else {
            result = current;
            current = current->left();
        }
    }
    if (result == nil()) {
        return std::nullopt;
    }
    return result->key();
}

template <typename keyT, typename Comp>
std::optional<keyT>
redBlackTree_t<keyT, Comp>::upperBound(const keyT& key) const {
    iterator current = root_;
    iterator result = nil();
    while (current != nil()) {
        if (comp_(key, current->key())) {
            result = current;
            current = current->left();
        } else {
            current = current->right();
        }
    }
    if (result == nil()) {
        return std::nullopt;
    }
    return result->key();
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::successor(iterator node) const {
    if (node == nil()) {
        return nil();
    }
    if (node->right() != nil()) {
        node = node->right();
        while (node->left() != nil()) {
            node = node->left();
        }
        return node;
    }
    iterator parent = node->parent();
    while (parent != nil() && node == parent->right()) {
        node = parent;
        parent = parent->parent();
    }
    return parent;
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertInSubtree(
    keyT&& key, iterator subtreeRoot, iterator parent, ChildSide side) {
    if (subtreeRoot == nil()) {
        iterator newNode = new node_t(std::move(key), parent, nil());
        if (parent == nil()) {
            root_ = newNode;
        } else if (side == ChildSide::LEFT) {
            parent->setLeft(newNode);
        } else {
            parent->setRight(newNode);
        }
        insertCase1(newNode);
        return;
    }

    if (comp_(key, subtreeRoot->key())) {
        return insertInSubtree(std::move(key), subtreeRoot->left(), subtreeRoot, ChildSide::LEFT);
    }
    if (comp_(subtreeRoot->key(), key)) {
        return insertInSubtree(std::move(key), subtreeRoot->right(), subtreeRoot, ChildSide::RIGHT);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insert(keyT key) {
    return insertInSubtree(std::move(key), root_, nil(), ChildSide::LEFT);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::erase(const keyT& key) {
    iterator rmNode = findInSubtree(key, root_);
    deleteNode(rmNode);
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::treeMaxKeyNode(iterator node) {
    if (node == nil() || node->right() == nil()) {
        return node;
    }
    return treeMaxKeyNode(node->right());
}

template <typename keyT, typename Comp>
typename redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::treeMinKeyNode(iterator node) {
    if (node == nil() || node->left() == nil()) {
        return node;
    }
    return treeMinKeyNode(node->left());
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::transplant(iterator replaceableNode, iterator newNode) {
    iterator parent = replaceableNode->parent();

    if (parent == nil()) {
        root_ = newNode;
    } else if (replaceableNode == parent->left()) {
        parent->setLeft(newNode);
    } else {
        parent->setRight(newNode);
    }
    newNode->setParent(parent);
}

template <typename keyT, typename Comp>
std::pair<typename redBlackTree_t<keyT, Comp>::iterator,
          typename redBlackTree_t<keyT, Comp>::nodeColor_t>
redBlackTree_t<keyT, Comp>::removeNodeWithTwoChildren(iterator rmNode) {
    iterator successor = treeMinKeyNode(rmNode->right());

    nodeColor_t removableNodeOriginalColor = successor->color();
    iterator replacement = successor->right();

    if (successor != rmNode->right()) {
        transplant(successor, replacement);
        successor->setRight(rmNode->right());
        rmNode->right()->setParent(successor);
    } else {
        replacement->setParent(successor);
    }
    transplant(rmNode, successor);
    successor->setLeft(rmNode->left());
    successor->left()->setParent(successor);
    successor->setColor(rmNode->color());

    return {replacement, removableNodeOriginalColor};
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::deleteFixup(iterator extraBlackNode) {
    while (extraBlackNode != root_ && extraBlackNode->color() == nodeColor_t::BLACK) {
        iterator parent = extraBlackNode->parent();

        if (extraBlackNode->sibling()->color() == nodeColor_t::RED) {
            extraBlackNode->sibling()->setColor(nodeColor_t::BLACK);
            parent->setColor(nodeColor_t::RED);
            rotateParent(extraBlackNode);
        }

        if (extraBlackNode->nearNephew()->color() == nodeColor_t::BLACK &&
            extraBlackNode->farNephew()->color() == nodeColor_t::BLACK) {
            extraBlackNode->sibling()->setColor(nodeColor_t::RED);
            extraBlackNode = parent;
        } else {
            if (extraBlackNode->farNephew()->color() == nodeColor_t::BLACK) {
                extraBlackNode->nearNephew()->setColor(nodeColor_t::BLACK);
                extraBlackNode->sibling()->setColor(nodeColor_t::RED);

                rotateSibling(extraBlackNode);
            }
            extraBlackNode->sibling()->setColor(parent->color());
            parent->setColor(nodeColor_t::BLACK);
            extraBlackNode->farNephew()->setColor(nodeColor_t::BLACK);

            rotateParent(extraBlackNode);

            extraBlackNode = root_;
        }
    }
    extraBlackNode->setColor(nodeColor_t::BLACK);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::rotateParent(iterator node) {
    iterator parent = node->parent();
    if (parent == nil()) {
        return;
    }

    if (node == parent->left()) {
        rotateLeft(parent);
    } else {
        rotateRight(parent);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::rotateSibling(iterator node) {
    iterator sibling = node->sibling();
    if (sibling == nil()) {
        return;
    }

    iterator parent = node->parent();
    if (node == parent->left()) {
        rotateRight(sibling);
    } else {
        rotateLeft(sibling);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::deleteNode(iterator rmNode) {
    if (rmNode == nil()) {
        return;
    }
    nodeColor_t removableNodeOriginalColor = rmNode->color();
    iterator replacement = nil();

    if (rmNode->left() == nil()) {
        replacement = rmNode->right();
        transplant(rmNode, replacement);
    } else if (rmNode->right() == nil()) {
        replacement = rmNode->left();
        transplant(rmNode, replacement);
    } else {
        std::tie(replacement, removableNodeOriginalColor) = removeNodeWithTwoChildren(rmNode);
    }

    if (removableNodeOriginalColor == nodeColor_t::BLACK) {
        deleteFixup(replacement);
    }

    delete(rmNode);
}

} // namespace red_black_tree
