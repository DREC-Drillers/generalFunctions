

namespace red_black_tree {

template <typename keyT>
class redBlackTree_t<keyT>::node_t {
private:
    keyT key_;
    nodeColor_t color_ = nodeColor_t::RED;
    iterator parent_;
    iterator left_ = nullptr;
    iterator right_ = nullptr;
public:
    node_t(keyT key, iterator parent = nullptr)
        : key_(key), parent_(parent) {};

    iterator grandparent() const;
    iterator uncle() const;
    iterator sibling() const;

    bool isLeaf() { return left() == nullptr && right == nullptr };
    bool hasRedChilds();

    keyT key() { return key; };
    void setKey(keyT key) { key_ = key; };

    iterator parent() const { return parent_; };
    iterator left() const { return left_; };
    iterator right() const { return right_; };

    void setParent(iterator parent) { parent_ = parent; };
    void setLeft(iterator left) { left_ = left; };
    void setRight(iterator right) { right_ = right; };

    nodeColor_t color() const { return color_; };
    void setColor(nodeColor_t color) { color_ = color};
    void switchColor() { color_ = color_ == nodeColor_t::BLACK ? nodeColor_t::BLACK
                                                               : nodeColor_t::RED; };
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
    return node == parent.left() ? parent.right() : parent.left();
}

bool
node_t::hasRedChilds() {
    return (left() && left().color() == nodeColor_t::RED) ||
           (right() && right().color() == nodeColor_t::RED);
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
    } else if (node == parent.left() && parent == grandparent.right()) {
        rotateRight(parent);
    }

    insertCase5(parent);
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

iterator
redBlackTree_t<keyT>::find(keyT key, iterator node) {
    if (!node) {
        return nullptr;
    }
    if (node.key() == key) {
        return node;
    }
    return key < node.key() ? find(key, node.left()) : find(key, node.right());
}

void
redBlackTree_t<keyT>::insert(keyT key, iterator node, iterator parent) {
    if (!node) {
        iterator newNode = new iterator(key, parent);
        insertCase1(newNode);
    }

    if (node.key() == key) {
        return;
    }

    return node.key() < key ? insert(key, node.left, node) :
                              insert(key, node.right, node);
}

void
redBlackTree_t<keyT>::delete(keyT key, iterator node = root_) {
    if (!node) {
        return;
    }

    if (key > node->key()) {
        return delete(key, node->right());
    }
    if (key < node->key()) {
        return delete(key, node->left());
    }

    if (node->left() && node->right()) {
        return deleteNodeWithTwoChilds(node);
    }

    deleteNodeWithLessThanTwoChilds(node);
}

void
redBlackTree_t<keyT>::deleteNodeWithLessThanTwoChilds(iterator node) {
    iterator child = node.right().isLeaf() ? node.left() : node.right();

    replaceNodeByChild(node, child);
	if (node->color() == nodeColor_t::BLACK) {
        if (child)
            child->setColor(nodeColor_t::BLACK);
		else
            chooseDeleteCases(node);
	}
	delete(node);
}

iterator
redBlackTree_t<keyT>::findMaxKeyNode(iterator node) {
    if (!node->right()) {
        return node;
    }
    return findMaxKeyNode(node->right());
}

void
redBlackTree_t<keyT>::deleteNodeWithTwoChilds(iterator node) {
    iterator maxKeyNode = findMaxKeyNode(node->right);
    node->setKey(maxKeyNode->key());
    deleteNodeWithLessThanTwoChilds(maxKeyNode);
}

void
redBlackTree_t<keyT>::replaceNodeByChild(iterator node, iterator child) {
    iterator parent = node.parent();
    if (child) {
        child.setParent(parent);
    }

    if (node == parent->left()) {
        parent->left() = child;
    } else {
        parent->right() = child;
    }
}

void
redBlackTree_t<keyT>chooseDeleteCases(iterator node) {
    iterator parent = node->parent();
    iterator sibling = node->sibling();
    if(!parent) {
        return;
    }

    if (parent->color == nodeColor_t::RED) {
        return sibling.hasRedChilds() ? deleteCase1() : deleteCAse2();
    }

    if (sibling->color() = nodeColor_t::RED) {
        return
    }

    return sibling.hasRedChilds() ? deleteCase5() : deleteCAse6();
}

void
redBlackTree_t<keyT>::deleteCase1(iterator node) {

}

void
redBlackTree_t<keyT>::deleteCase2(iterator node) {

}

void
redBlackTree_t<keyT>::deleteCase3(iterator node) {

}

void
redBlackTree_t<keyT>::deleteCase4(iterator node) {

}

void
redBlackTree_t<keyT>::deleteCase5(iterator node) {

}

void
redBlackTree_t<keyT>::deleteCase6(iterator node) {

}

} // namespace red_black_tree
