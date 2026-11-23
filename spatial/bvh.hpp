#pragma once

#include <cstddef>
#include <variant>
#include <vector>

#include "tree/binary_tree.hpp"

import geometry;

namespace general_functions {

using geometry::axisAlignedBoundingBox_t;
using geometry::point_t;

class Bvh {
public:
    explicit Bvh(std::vector<axisAlignedBoundingBox_t> bounds);

    std::size_t size() const noexcept;
    bool empty() const noexcept;
    const axisAlignedBoundingBox_t& objectBounds(std::size_t object) const;

    template <typename Visitor>
    void query(const axisAlignedBoundingBox_t& bounds, Visitor&& visitor) const {
        if (const auto root = tree_.root(); root != Tree::INVALID_NODE) {
            visitNode(root, bounds, visitor);
        }
    }

private:
    struct ObjectRange {
        std::size_t begin;
        std::size_t count;
    };

    using NodeData = std::variant<ObjectRange, axisAlignedBoundingBox_t>;

    using Tree = general_functions::BinaryTree<NodeData>;
    using NodeId = Tree::NodeId;

    NodeData makeNode(std::size_t begin, std::size_t count) const;
    std::size_t selectSplitAxis(std::size_t begin, std::size_t count,
                                const std::vector<point_t<3>>& centers) const;
    std::size_t partitionObjects(std::size_t begin, std::size_t count,
                                 std::size_t splitAxis,
                                 const std::vector<point_t<3>>& centers);
    void build(NodeId node, std::size_t begin, std::size_t count,
               const std::vector<point_t<3>>& centers);

    template <typename Visitor>
    void visitNode(NodeId id, const axisAlignedBoundingBox_t& bounds, Visitor& visitor) const {
        const auto& treeNode = tree_.getNode(id);
        const auto& node = treeNode.data();
        if (const auto* objects = std::get_if<ObjectRange>(&node)) {
            for (std::size_t offset = 0; offset < objects->count; ++offset) {
                const auto object = objectIndices_.at(objects->begin + offset);
                if (bounds_.at(object).overlaps(bounds)) {
                    visitor(object);
                }
            }
            return;
        }

        if (!std::get<axisAlignedBoundingBox_t>(node).overlaps(bounds)) {
            return;
        }

        visitNode(treeNode.left(), bounds, visitor);
        visitNode(treeNode.right(), bounds, visitor);
    }

    static constexpr std::size_t MAX_LEAF_SIZE = 4;

    std::vector<axisAlignedBoundingBox_t> bounds_;
    std::vector<std::size_t> objectIndices_;
    Tree tree_;
};

} // namespace general_functions
