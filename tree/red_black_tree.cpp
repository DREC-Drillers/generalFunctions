

export namespace red_black_tree {

template <typename keyT, typename Comp>
class redBlackTree_t {
private:
    enum class nodeColor_t {RED, BLACK};
    class node_t;
    using iterator = node_t*;
    iterator root_ = nullptr;
    node_t = nil_;
    iterator nil() { return &nil; }

public:
    iterator lowerBound(keyT key) const;
    iterator upperBound(keyT key) const;
    iterator distance(iterator fst, iterator snd) const;

public:
    void insert(keyT key, iterator node = root_, iterator parent = nullptr);
    void delete(keyT key, iterator node = root_);
    iterator find(keyT key, iterator node = root_);
}

} // namespace red_black_tree
