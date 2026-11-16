#include <memory>
#include <utility>

export namespace red_black_tree {

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
    iterator findInSubtree(const keyT& key, iterator subTreeRoot);
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
    iterator lowerBound(const keyT& key) const;
    iterator upperBound(const keyT& key) const;
    // Both positions must belong to this tree; nil denotes the end position.
    int distance(iterator fst, iterator snd) const;

public:
    void insert(keyT key);
    void erase(const keyT& key);
    iterator find(const keyT& key);
};

} // namespace red_black_tree
