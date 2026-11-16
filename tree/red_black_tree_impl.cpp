

namespace red_black_tree {

template <typename keyT, typename Comp>
class redBlackTree_t<keyT, Comp>::node_t {
private:
    keyT key_;
    nodeColor_t color_ = nodeColor_t::RED;
    iterator parent_;
    iterator left_ = nullptr;
    iterator right_ = nullptr;

public:
    node_t(keyT key, iterator parent = nullptr)
        : key_(key), parent_(parent) {};

    keyT key() const { return key_; };
    nodeColor_t color() const { return color_; };
    iterator parent() const { return parent_; };
    iterator left() const { return left_; };
    iterator right() const { return right_; };

    iterator grandparent() const;
    iterator uncle() const;
    iterator sibling() const;
    iterator nearNephew() const;
    iterator farNephew() const;

public:
    void setParent(iterator parent) { parent_ = parent; };
    void setLeft(iterator left) { left_ = left; };
    void setRight(iterator right) { right_ = right; };
    void setColor(nodeColor_t color) { color_ = color};
}

iterator
node_t::grandparent() {
    return parent_ == nullptr ? nullptr : parent_.parent();
}

iterator
node_t::uncle() {
    iterator grandparent = grandparent();
    if (!grandparent) {
        return nullptr;
    }

    return parent() == grandparent.left() ? grandparent.right() : grandparent.left();
}

iterator
node_t::sibling() {
    iterator parent = node.parent();
    if (!parent) {
        return nullptr;
    }

    return node == parent.left() ? parent.right() : parent.left();
}

iterator
node_t::nearNephew() {
    iterator sibling = node->sibling();
    if (!sibling) {
        return nullptr;
    }

    return node == node->parent()->left() ? sibling->left() : sibling->right();
}

iterator
node_t::farNephew() {
    iterator sibling = node->sibling();
    if (!sibling) {
        return nullptr;
    }

    return node == node->parent()->left() ? sibling->right() : sibling->left();
}

void
redBlackTree_t<keyT>::rotateLeft(iterator node) {

    iterator pivot = node.right();

    pivot.setParent(node.parent());
    if (node.parent()) {
        if (node.parent().left() == node)
            node.parent().setLeft(pivot);
        else
            node.parent().setRight(pivot);
    } else {
        root_ = pivot;
    }

    node.setRight(pivot.left());
    if (pivot.left()) {
        pevot.left().setParent(node);
    }

    node.setParent(pivot);
    pivot.setLeft(node);
}

void
redBlackTree_t<keyT>::rotateRight(iterator node) {

    iterator pivot = node.left();

    pivot.setParent(node.parent());
    if (node.parent()) {
        if (node.parent().left() == node)
            node.parent().setLeft(pivot);
        else
            node.parent().setRight(pivot);
    } else {
        root_ = pivot;
    }

    node.setLeft(pivot.right());
    if (pivot.right()) {
        pivot.right().setParent(node);
    }

    node.setParent(pivot);
    pivot.setRight(node);
}

void
redBlackTree_t<keyT>::insertCase1(iterator node) {
    if (!node.parent()) {
        node.setColor(nodeColor_t::BLACK);
    } else {
        insertCase2(node);
    }
}

void
redBlackTree_t<keyT>::insertCase2(iterator node) {
    if (n.parent().color() == nodeColor_t::BLACK)
		return;
	else
		insertCase3(node);
}

void
redBlackTree_t<keyT>::insertCase3(iterator node) {
    iterator uncle = node.uncle();
    iterator grandparent = node.grandparent();
    iterator parent = node.parent();

	if ((uncle) && (uncle.color() == nodeColor_t::RED)) {
		parent.setColor(nodeColor_t::BLACK);
		uncle.setColor(nodeColor_t::BLACK);

        grandparent.setColor(nodeColor_t::RED);
		insertCase1(grandparent);
	} else {
		insertCase4(node);
	}
}

void
redBlackTree_t<keyT>::insertCase4(iterator node) {
    iterator grandparent = node.grandparent();
    iterator parent = node.parent();

    if (node == parent.right() && parent == grandparent.left()) {
        rotateLeft(parent);
        node = node->left();
    } else if (node == parent.left() && parent == grandparent.right()) {
        rotateRight(parent);
        node = node->right();
    }

    insertCase5(node);
}

void
redBlackTree_t<keyT>::insertCase5(iterator node) {
    iterator grandparent = node.grandparent();
    iterator parent = node.parent();

    parent().setColor(nodeColor_t::BLACK);
    grandparent.setColor(nodeColor_t::RED);

    if (node == parent.left() && parent == grandparent.left()) {
        rotateRight(grandparent);
    } else {
        rotateLeft(grandparent);
    }
}

template <typename keyT, typename Comp>
redBlackTree_t<keyT, Comp>::iterator
redBlackTree_t<keyT, Comp>::findInSubtree(keyT key, iterator subTreeRoot) {
    if (!subTreeRoot) {
        return nullptr;
    }
    if (comp_(key, subTreeRoot->key())) {
        return findInSubtree(key, subTreeRoot->left());
    }
    if (comp_(node->key(), key)) {
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
redBlackTree_t<keyT, Comp>::insertInSubtree(keyT key, iterator subtreeRoot, iterator parent) {
    if (!subtreeRoot) {
        const bool insertLeft = parent && comp_(key, parent->key());
        iterator newNode = new node_t(key, parent);
        if (!parent) {
            root_ = newNode;
        } else if (insertLeft) {
            parent->setLeft(newNode);
        } else {
            parent->setRight(newNode);
        }
        insertCase1(newNode);
        return;
    }

    if (comp_(key, subtreeRoot->key())) {
        return insertInSubtree(key, subtreeRoot->left(), subtreeRoot);
    }
    if (comp_(subtreeRoot->key(), key)) {
        return insertInSubtree(key, subtreeRoot->right(), subtreeRoot);
    }
}

template <typename keyT, typename Comp>
void
redBlackTree_t<keyT, Comp>::insert(keyT key) {
    return insertInSubtree(key, root_, nullptr);
}

void
redBlackTree_t<keyT>::delete(keyT key) {
    iterator rmNode = find(key);
    deleteNode(rmNode);
}

iterator
redBlackTree_t<keyT, Comp>::treeMaxKeyNode(iterator node) {
    if (node->right() == nil()) {
        return node;
    }
    return treeMaxKeyNode(node->right());
}

iterator
redBlackTree_t<keyT, Comp>::treeMinKeyNode(iterator node) {
    if (node->left() == nil()) {
        return node;
    }
    return treeMinKeyNode(node->left());
}

void
redBlackTree_t<keyT, Comp>transplant(iterator replaceableNode, iterator newNode) {
    iterator parent = replaceableNode->parent();

    if (parent == nil()) {
        root_ = newNode;
    } else if (replaceableNode == parent->left()) {
        parent->left() = newNode;
    } else {
        parent->right() = newNode;
    }
    newNode->setParent(parent);
}

std::pair<iterator, nodeColor_t>
removeNodeWithTwoChildren(iterator rmNode) {
    iterator successor = treeMinKeyNode(rmNode->right());

    nodeColor_t removableNodeOriginalColor = successor->color();
    iterator replacement = successor->right();

    if (successor != rmNode->right()) {
        transplant(successor, replacement);
        successor->setRight(rmNode->right());
        rmNode->right()->parent = successor;
    } else {
        replacement->setParent(successor);
    }
    transplant(rmNode, successor);
    successor->setLeft(rmNode->left());
    successor->left()->setParent(successor);
    successor->setColor(rmNode->color);

    return {replacement, originalColor};
}

void
redBlackTree_t<keyT, Comp>deleteFixup(iterator extraBlackNode) {
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

void
redBlackTree_t<keyT, Comp>::rotateParent(iterator node) {
    iterator parent = node->parent();
    if (!parent) {
        return;
    }

    if (node == parent->left()) {
        rotateLeft(parent);
    } else {
        rotateRight(parent);
    }
}

void
redBlackTree_t<keyT, Comp>::rotateSibling(iterator node) {
    iterator sibling = node->sibling();
    if (!sibling) {
        return;
    }

    iterator parent = node->parent();
    if (node == parent->left()) {
        rotateRight(sibling);
    } else {
        rotateLeft(sibling);
    }
}

void
redBlackTree_t<keyT, Comp>()deleteNode(iterator rmNode) {
    nodeColor_t removableNodeOriginalColor = rmNode->color();
    iterator replacement = nil();

    if (rmNode->left() == nil()) {
        replacement = rmNode->right();
        transplant(rmNode, replacement);
    } else if (rmNode->right() == nil) {
        replacement = rmNode->left();
        transplant(rmNode, replacement);
    } else {
        std::tie(replacement, removableNodeOriginalColor) = removeTwoChildren(rmNode);
    }

    if (successorOriginalColor == nodeColor_t::BLACK) {
        deleteFixup(replacement);
    }

    delete(rmNode);
}

} // namespace red_black_tree
