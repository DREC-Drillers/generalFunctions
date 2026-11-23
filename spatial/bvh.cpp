#include "bvh.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <utility>

namespace general_functions {

Bvh::Bvh(std::vector<axisAlignedBoundingBox_t> bounds) : bounds_(std::move(bounds)) {
    if (bounds_.empty()) {
        return;
    }

    tree_.reserve(bounds_.size());
    objectIndices_.resize(bounds_.size());
    std::iota(objectIndices_.begin(), objectIndices_.end(), std::size_t{0});

    std::vector<point_t<3>> centers;
    centers.reserve(bounds_.size());
    for (const auto& box : bounds_) {
        centers.push_back(box.center());
    }

    const auto root = tree_.addRoot(makeNode(0, bounds_.size()));
    build(root, 0, bounds_.size(), centers);
}

std::size_t Bvh::size() const noexcept {
    return bounds_.size();
}

bool Bvh::empty() const noexcept {
    return bounds_.empty();
}

const axisAlignedBoundingBox_t& Bvh::objectBounds(std::size_t object) const {
    return bounds_.at(object);
}

Bvh::NodeData Bvh::makeNode(std::size_t begin, std::size_t count) const {
    if (count <= MAX_LEAF_SIZE) {
        return ObjectRange{begin, count};
    }

    axisAlignedBoundingBox_t bounds = bounds_.at(objectIndices_.at(begin));
    for (std::size_t offset = 1; offset < count; ++offset) {
        bounds.expand(bounds_.at(objectIndices_.at(begin + offset)));
    }
    return bounds;
}

std::size_t
Bvh::selectSplitAxis(std::size_t begin, std::size_t count,
                     const std::vector<point_t<3>>& centers) const {
    const auto& firstCenter = centers.at(objectIndices_.at(begin));
    std::array<double, 3> minCenter{firstCenter[0], firstCenter[1], firstCenter[2]};
    auto maxCenter = minCenter;
    for (std::size_t offset = 1; offset < count; ++offset) {
        const auto& center = centers.at(objectIndices_.at(begin + offset));
        for (std::size_t coordinateAxis = 0; coordinateAxis < 3;
             ++coordinateAxis) {
            minCenter.at(coordinateAxis) =
                std::min(minCenter.at(coordinateAxis), center[coordinateAxis]);
            maxCenter.at(coordinateAxis) =
                std::max(maxCenter.at(coordinateAxis), center[coordinateAxis]);
        }
    }

    const auto halfCenterSpread = [&](std::size_t dimension) {
        return maxCenter.at(dimension) / 2 - minCenter.at(dimension) / 2;
    };

    std::size_t splitAxis = 0;
    for (std::size_t candidateAxis = 1; candidateAxis < minCenter.size();
         ++candidateAxis) {
        if (halfCenterSpread(candidateAxis) > halfCenterSpread(splitAxis)) {
            splitAxis = candidateAxis;
        }
    }

    return splitAxis;
}

std::size_t Bvh::partitionObjects(std::size_t begin, std::size_t count,
                                  std::size_t splitAxis,
                                  const std::vector<point_t<3>>& centers) {
    const auto middle = begin + count / 2;
    using Difference = std::vector<std::size_t>::difference_type;
    std::nth_element(objectIndices_.begin() + static_cast<Difference>(begin),
                     objectIndices_.begin() + static_cast<Difference>(middle),
                     objectIndices_.begin() + static_cast<Difference>(begin + count),
                     [&](std::size_t first, std::size_t second) {
                         const auto a = centers.at(first)[splitAxis];
                         const auto b = centers.at(second)[splitAxis];
                         return a < b || (a == b && first < second);
                     });

    return middle;
}

void Bvh::build(NodeId node, std::size_t begin, std::size_t count,
                const std::vector<point_t<3>>& centers) {
    if (count <= MAX_LEAF_SIZE) {
        return;
    }

    const auto splitAxis = selectSplitAxis(begin, count, centers);

    const auto middle = partitionObjects(begin, count, splitAxis, centers);

    const auto leftCount = middle - begin;
    const auto rightCount = count - leftCount;

    const auto left = tree_.allocateNode(makeNode(begin, leftCount));
    tree_.getNode(node).setLeft(left);
    tree_.getNode(left).setParent(node);
    build(left, begin, leftCount, centers);

    const auto right = tree_.allocateNode(makeNode(middle, rightCount));
    tree_.getNode(node).setRight(right);
    tree_.getNode(right).setParent(node);
    build(right, middle, rightCount, centers);
}

} // namespace general_functions
