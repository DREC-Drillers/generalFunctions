#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>

#include <gtest/gtest.h>

import geometry;

namespace {

using geometry::axisAlignedBoundingBox_t;
using geometry::point_t;

TEST(axisAlignedBoundingBox_t, PreservesBoundsAndComputesCenter) {
    const axisAlignedBoundingBox_t box(point_t<3>({-2, 1, 0}), point_t<3>({4, 5, 8}));

    EXPECT_EQ(box.min(), (point_t<3>({-2, 1, 0})));
    EXPECT_EQ(box.max(), (point_t<3>({4, 5, 8})));
    EXPECT_EQ(box.center(), (point_t<3>({1, 3, 4})));
}

TEST(axisAlignedBoundingBox_t, RejectsReversedAndNonFiniteBoundsOnEveryAxis) {
    constexpr double INFINITY_VALUE = std::numeric_limits<double>::infinity();
    constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();

    for (std::size_t axis = 0; axis < 3; ++axis) {
        std::array<double, 3> lower{0, 0, 0};
        std::array<double, 3> upper{1, 1, 1};
        lower.at(axis) = 2;
        EXPECT_THROW((axisAlignedBoundingBox_t(point_t<3>(lower), point_t<3>(upper))), std::invalid_argument);

        for (double invalid : {INFINITY_VALUE, -INFINITY_VALUE, NAN_VALUE}) {
            lower = {0, 0, 0};
            upper = {1, 1, 1};
            lower.at(axis) = invalid;
            EXPECT_THROW((axisAlignedBoundingBox_t(point_t<3>(lower), point_t<3>(upper))), std::invalid_argument);

            lower = {0, 0, 0};
            upper.at(axis) = invalid;
            EXPECT_THROW((axisAlignedBoundingBox_t(point_t<3>(lower), point_t<3>(upper))), std::invalid_argument);
        }
    }
}

TEST(axisAlignedBoundingBox_t, DetectsOverlapContainmentAndSelfOverlap) {
    const axisAlignedBoundingBox_t outer(point_t<3>({0, 0, 0}), point_t<3>({4, 4, 4}));
    const axisAlignedBoundingBox_t inner(point_t<3>({1, 1, 1}), point_t<3>({2, 2, 2}));
    const axisAlignedBoundingBox_t crossing(point_t<3>({3, -1, 2}), point_t<3>({5, 2, 6}));

    EXPECT_TRUE(outer.overlaps(inner));
    EXPECT_TRUE(inner.overlaps(outer));
    EXPECT_TRUE(outer.overlaps(crossing));
    EXPECT_TRUE(crossing.overlaps(outer));
    EXPECT_TRUE(outer.overlaps(outer));
}

TEST(axisAlignedBoundingBox_t, RejectsSeparationInEitherDirectionOnEveryAxis) {
    const axisAlignedBoundingBox_t box(point_t<3>({0, 0, 0}), point_t<3>({1, 1, 1}));

    for (std::size_t axis = 0; axis < 3; ++axis) {
        std::array<double, 3> lower{0, 0, 0};
        std::array<double, 3> upper{1, 1, 1};
        lower.at(axis) = 2;
        upper.at(axis) = 3;
        const axisAlignedBoundingBox_t separated{point_t<3>(lower), point_t<3>(upper)};

        EXPECT_FALSE(box.overlaps(separated));
        EXPECT_FALSE(separated.overlaps(box));
    }
}

TEST(axisAlignedBoundingBox_t, IncludesFaceEdgeAndCornerContact) {
    const axisAlignedBoundingBox_t box(point_t<3>({0, 0, 0}), point_t<3>({1, 1, 1}));

    EXPECT_TRUE(box.overlaps(axisAlignedBoundingBox_t(point_t<3>({1, 0, 0}), point_t<3>({2, 1, 1}))));
    EXPECT_TRUE(box.overlaps(axisAlignedBoundingBox_t(point_t<3>({1, 1, 0}), point_t<3>({2, 2, 1}))));
    EXPECT_TRUE(box.overlaps(axisAlignedBoundingBox_t(point_t<3>({1, 1, 1}), point_t<3>({2, 2, 2}))));
}

TEST(axisAlignedBoundingBox_t, SupportsFlatBoxesAndPointsWithoutImplicitTolerance) {
    const axisAlignedBoundingBox_t plane(point_t<3>({0, 0, 0}), point_t<3>({2, 2, 0}));
    const axisAlignedBoundingBox_t point(point_t<3>({1, 1, 0}), point_t<3>({1, 1, 0}));
    const axisAlignedBoundingBox_t offset(point_t<3>({0, 0, 1e-12}), point_t<3>({2, 2, 1e-12}));

    EXPECT_TRUE(plane.overlaps(point));
    EXPECT_TRUE(point.overlaps(plane));
    EXPECT_TRUE(point.overlaps(point));
    EXPECT_FALSE(plane.overlaps(offset));
    EXPECT_EQ(point.center(), point.min());
}

TEST(axisAlignedBoundingBox_t, ExpandsToEncloseBothBoxesAndPreservesContainingBounds) {
    axisAlignedBoundingBox_t box(point_t<3>({0, 0, 0}), point_t<3>({2, 2, 2}));
    box.expand(axisAlignedBoundingBox_t(point_t<3>({-3, 1, -1}), point_t<3>({1, 5, 1})));

    EXPECT_EQ(box.min(), (point_t<3>({-3, 0, -1})));
    EXPECT_EQ(box.max(), (point_t<3>({2, 5, 2})));

    box.expand(axisAlignedBoundingBox_t(point_t<3>({0, 1, 0}), point_t<3>({1, 2, 1})));
    box.expand(box);
    EXPECT_EQ(box.min(), (point_t<3>({-3, 0, -1})));
    EXPECT_EQ(box.max(), (point_t<3>({2, 5, 2})));
}

TEST(axisAlignedBoundingBox_t, ComputesFiniteCenterForExtremeBounds) {
    constexpr double MAX_VALUE = std::numeric_limits<double>::max();
    const axisAlignedBoundingBox_t box(point_t<3>({-MAX_VALUE, MAX_VALUE / 2, -MAX_VALUE}), point_t<3>({MAX_VALUE, MAX_VALUE, -MAX_VALUE / 2}));
    const auto center = box.center();

    EXPECT_DOUBLE_EQ(center[0], 0);
    EXPECT_DOUBLE_EQ(center[1], MAX_VALUE * 0.75);
    EXPECT_DOUBLE_EQ(center[2], -MAX_VALUE * 0.75);
    for (std::size_t axis = 0; axis < 3; ++axis) {
        EXPECT_TRUE(std::isfinite(center[axis]));
    }
}

} // namespace
