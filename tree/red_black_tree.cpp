

export namespace red_black_tree {

template <typename keyT>
class redBlackTree_t {
private:
    enum class nodeColor_t {RED, BLACK};
    class node_t;
    using iterator = node_t*;

    iterator root_ = nullptr;

    void rotateLeft(iterator node);
    void rotateRight(iterator node);

public:
    iterator root() { return root_; };

    void insert(keyT key, iterator node = root_, iterator parent = nullptr);
    void delete(keyT key, iterator node = root_);
    iterator find(keyT key, iterator node = root_);
}

} // namespace red_black_tree
