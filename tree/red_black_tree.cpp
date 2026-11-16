#include <utility>

export namespace red_black_tree {

template <typename keyT, typename Comp>
class redBlackTree_t {
private:
    Comp comp_;
    enum class nodeColor_t {RED, BLACK};
    class node_t;
    using iterator = node_t*;
    iterator root_ = nullptr;
    node_t = nil_;
    iterator nil() { return &nil; }
    void insertInSubtree(keyT key, iterator* link, iterator parent);

public:
    explicit redBlackTree_t(Comp comp = Comp{}) : comp_(std::move(comp)) {}

    iterator lowerBound(keyT key) const;
    iterator upperBound(keyT key) const;
    iterator distance(iterator fst, iterator snd) const;

public:
    void insert(keyT key);
    void delete(keyT key);
    iterator find(keyT key);
}

} // namespace red_black_tree
