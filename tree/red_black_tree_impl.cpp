#include <optional>
#include <tuple>

namespace red_black_tree {

template <typename keyT, typename Comp>
class redBlackTree_t<keyT, Comp>::node_t {
private:
    friend class redBlackTree_t<keyT, Comp>;

    std::optional<keyT> key_;
    nodeColor_t color_;
    iterator parent_;
    iterator left_;
    iterator right_;

public:
    node_t()
        : color_(nodeColor_t::BLACK), parent_(this), left_(this), right_(this) {}

    node_t(keyT key, iterator parent, iterator nil)
        : key_(std::move(key)), color_(nodeColor_t::RED), parent_(parent),
          left_(nil), right_(nil) {}

    bool isNil() const { return !key_.has_value(); }
    keyT key() const { return key_.value(); }
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
redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::node_t::grandparent() const {
    return parent_->isNil() ? parent_ : parent_->parent();
}

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::node_t::uncle() const {
    iterator grandparent = this->grandparent();
    if (grandparent->isNil()) {
        return grandparent;
    }

    return parent_ == grandparent->left() ? grandparent->right() : grandparent->left();
}

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::node_t::sibling() const {
    if (parent_->isNil()) {
        return parent_;
    }

    return this == parent_->left() ? parent_->right() : parent_->left();
}

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::node_t::nearNephew() const {
    iterator sibling = this->sibling();
    return this == parent_->left() ? sibling->left() : sibling->right();
}

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::iterator
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
redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::findInSubtree(keyT key, iterator subTreeRoot) {
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
redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::find(keyT key) {
    return findInSubtree(key, root_);
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insertInSubtree(keyT key, iterator* link, iterator parent) {
    iterator subtreeRoot = *link;
    if (subtreeRoot == nil()) {
        *link = new node_t(key, parent, nil());
        insertCase1(*link);
        return;
    }

    if (comp_(key, subtreeRoot->key())) {
        return insertInSubtree(key, &subtreeRoot->left_, subtreeRoot);
    }
    if (comp_(subtreeRoot->key(), key)) {
        return insertInSubtree(key, &subtreeRoot->right_, subtreeRoot);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insert(keyT key) {
    return insertInSubtree(key, &root_, nil());
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::delete(keyT key) {
    iterator rmNode = find(key);
    deleteNode(rmNode);
}

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::treeMaxKeyNode(iterator node) {
    if (node == nil() || node->right() == nil()) {
        return node;
    }
    return treeMaxKeyNode(node->right());
}

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::iterator
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
