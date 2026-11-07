#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include "spetial/bvh.hpp"

namespace {

using geometry::axisAlignedBoundingBox_t;
using geometry::point_t;
using general_functions::Bvh;

std::vector<std::size_t> collect(const Bvh& index, const axisAlignedBoundingBox_t& query) {
    std::vector<std::size_t> result;
    index.query(query, [&](std::size_t id) {
        result.push_back(id);
        return true;
    });
    std::sort(result.begin(), result.end());
    return result;
}

TEST(Bvh, HandlesEmptyAndSingleObject) {
    const axisAlignedBoundingBox_t box(point_t<3>({0, 0, 0}), point_t<3>({1, 1, 1}));
    const Bvh empty({});
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.size(), 0U);
    EXPECT_TRUE(collect(empty, box).empty());

    const Bvh single({box});
    EXPECT_FALSE(single.empty());
    EXPECT_EQ(single.size(), 1U);
    EXPECT_EQ(collect(single, box), (std::vector<std::size_t>{0}));
    EXPECT_TRUE(collect(single, axisAlignedBoundingBox_t(point_t<3>({2, 2, 2}), point_t<3>({3, 3, 3}))).empty());
}

TEST(Bvh, PreservesOriginalIdsAndChecksIndividualBoundsInsideLeaves) {
    std::vector<axisAlignedBoundingBox_t> boxes;
    for (int i = 19; i >= 0; --i) {
        const double x = i * 3;
        boxes.emplace_back(point_t<3>({x, 0, 0}),
                           point_t<3>({x + 1, 1, 0}));
    }
    const Bvh index(boxes);

    for (std::size_t id = 0; id < boxes.size(); ++id) {
        EXPECT_EQ(collect(index, boxes.at(id)), (std::vector<std::size_t>{id}));
    }
    EXPECT_TRUE(collect(index, axisAlignedBoundingBox_t(point_t<3>({1.5, 0, 0}), point_t<3>({2, 1, 0}))).empty());
    EXPECT_EQ(boxes.front().min(), (point_t<3>({57, 0, 0})));
}

TEST(Bvh, IncludesContactAndHandlesCoincidentCenters) {
    const axisAlignedBoundingBox_t point(point_t<3>({1, 1, 0}), point_t<3>({1, 1, 0}));
    const Bvh index(std::vector<axisAlignedBoundingBox_t>(100, point));
    const auto result = collect(index, axisAlignedBoundingBox_t(point_t<3>({0, 0, 0}), point_t<3>({1, 1, 0})));

    ASSERT_EQ(result.size(), 100U);
    for (std::size_t id = 0; id < result.size(); ++id) {
        EXPECT_EQ(result.at(id), id);
    }
}

TEST(Bvh, StopsWhenVisitorReturnsFalse) {
    const axisAlignedBoundingBox_t box(point_t<3>({0, 0, 0}), point_t<3>({1, 1, 1}));
    const Bvh index(std::vector<axisAlignedBoundingBox_t>(100, box));
    std::size_t calls = 0;
    index.query(box, [&](std::size_t) {
        ++calls;
        return false;
    });

    EXPECT_EQ(calls, 1U);
    EXPECT_EQ(collect(index, box).size(), 100U);
}

TEST(Bvh, MatchesBruteForceAcrossRandomAndFlatBounds) {
    std::mt19937 random(42);
    std::uniform_real_distribution<double> coordinate(-100, 100);
    std::uniform_real_distribution<double> extent(0, 20);
    std::vector<axisAlignedBoundingBox_t> boxes;
    for (int i = 0; i < 300; ++i) {
        const point_t<3> lower({
            coordinate(random), coordinate(random), coordinate(random)});
        const point_t<3> upper({lower[0] + extent(random),
                                      lower[1] + extent(random),
                                      lower[2] + (i % 2 ? extent(random) : 0)});
        boxes.emplace_back(lower, upper);
    }
    const Bvh index(boxes);

    for (const auto& query : boxes) {
        std::vector<std::size_t> expected;
        for (std::size_t id = 0; id < boxes.size(); ++id) {
            if (boxes.at(id).overlaps(query)) {
                expected.push_back(id);
            }
        }
        EXPECT_EQ(collect(index, query), expected);
    }
}

TEST(Bvh, HandlesExtremeCoordinatesAndOwnsInput) {
    constexpr double MAX_VALUE = std::numeric_limits<double>::max();
    std::vector<axisAlignedBoundingBox_t> boxes;
    for (int i = 0; i < 10; ++i) {
        const double x = i % 2 ? MAX_VALUE : -MAX_VALUE;
        boxes.emplace_back(point_t<3>({x, 0, 0}), point_t<3>({x, 0, 0}));
    }
    const Bvh index(boxes);
    boxes.clear();

    EXPECT_EQ(collect(index, axisAlignedBoundingBox_t(point_t<3>({MAX_VALUE, 0, 0}), point_t<3>({MAX_VALUE, 0, 0}))),
              (std::vector<std::size_t>{1, 3, 5, 7, 9}));
}

} // namespace
