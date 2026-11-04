#include <array>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

import geometry;

TEST(Geometry, RightTriangleAreaAndHypotenuse) {
    const geometry::triangle_t triangle{{0, 0, 0}, {3, 0, 0}, {0, 4, 0}};
    EXPECT_DOUBLE_EQ(triangle.area(), 6.0);
    EXPECT_DOUBLE_EQ(triangle.lenBC(), 5.0);
}

namespace {

template <typename Point>
concept HasZ = requires(const Point& point) { point.z(); };

template <typename Point>
concept HasOrient3D = requires(const Point& point) {
    Point::orient3D(point, point, point, point);
};

static_assert(!HasZ<geometry::point_t<2>>);
static_assert(HasZ<geometry::point_t<3>>);
static_assert(!HasOrient3D<geometry::point_t<2>>);
static_assert(HasOrient3D<geometry::point_t<3>>);
static_assert(!std::is_constructible_v<geometry::point_t<2>, double, double, double>);
static_assert(!std::is_constructible_v<geometry::point_t<3>, double, double>);

TEST(Point, SupportsDifferentDimensionsAndArrayConstruction) {
    const geometry::point_t<1> line{5};
    const geometry::point_t<2> plane{3, 4};
    const geometry::point_t<3> space{1, 2, 3};
    const geometry::point_t<4> higher(std::array<double, 4>{1, 2, 3, 4});
    EXPECT_DOUBLE_EQ(line.x(), 5);
    EXPECT_DOUBLE_EQ(plane.y(), 4);
    EXPECT_DOUBLE_EQ(space.z(), 3);
    EXPECT_DOUBLE_EQ(higher[3], 4);
    const geometry::point_t deduced{1.0, 2.0};
    static_assert(std::is_same_v<decltype(deduced), const geometry::point_t<2>>);
}

TEST(Point, CalculatesDistanceInEveryDimensionWithoutSquaredOverflow) {
    EXPECT_DOUBLE_EQ((geometry::point_t<1>{2}).distance({-3}), 5);
    EXPECT_DOUBLE_EQ((geometry::point_t<2>{0, 0}).distance({3, 4}), 5);
    EXPECT_DOUBLE_EQ((geometry::point_t<3>{0, 0, 0}).distance({2, 3, 6}), 7);
    EXPECT_DOUBLE_EQ((geometry::point_t<4>{0, 0, 0, 0}).distance({1, 2, 2, 4}), 5);
    EXPECT_DOUBLE_EQ((geometry::point_t<2>{0, 0}).distance({3e200, 4e200}), 5e200);
}

TEST(Point, ComparesAllCoordinatesAndValidatesTolerance) {
    const geometry::point_t<4> point{1, 2, 3, 4};
    EXPECT_EQ(point, (geometry::point_t<4>{1, 2, 3, 4}));
    EXPECT_NE(point, (geometry::point_t<4>{1, 2, 3, 5}));
    EXPECT_TRUE(point.isApprox({1, 2, 3, 4.01}, 0.02));
    EXPECT_FALSE(point.isApprox({1, 2, 3, 4.01}, 0.001));
    EXPECT_THROW(point.isApprox(point, -1), std::invalid_argument);
    EXPECT_THROW(point.isApprox(point, std::numeric_limits<double>::infinity()),
                 std::invalid_argument);
    EXPECT_THROW(point.isApprox(point, std::numeric_limits<double>::quiet_NaN()),
                 std::invalid_argument);
}

TEST(Point, RejectsNonFiniteCoordinates) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    EXPECT_THROW((geometry::point_t<1>{nan}), std::invalid_argument);
    EXPECT_THROW((geometry::point_t<2>{0, inf}), std::invalid_argument);
    EXPECT_THROW((geometry::point_t<3>{0, 0, -inf}), std::invalid_argument);
    EXPECT_THROW((geometry::point_t<4>{0, 0, 0, nan}), std::invalid_argument);
}

TEST(Point, PreservesOrient3DSignAndCoplanarity) {
    const geometry::point_t<3> a{0, 0, 0};
    const geometry::point_t<3> b{1, 0, 0};
    const geometry::point_t<3> c{0, 1, 0};
    const geometry::point_t<3> d{0, 0, 1};
    EXPECT_DOUBLE_EQ(geometry::point_t<3>::orient3D(a, b, c, d), -1);
    EXPECT_DOUBLE_EQ(geometry::point_t<3>::orient3D(a, c, b, d), 1);
    EXPECT_DOUBLE_EQ(geometry::point_t<3>::orient3D(a, b, c, {2, 2, 0}), 0);
}

} // namespace
