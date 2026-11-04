#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

import geometry;

namespace {

template <std::size_t Dim>
geometry::point_t<Dim> makePoint(std::array<double, Dim> coordinates) {
    return geometry::point_t<Dim>(coordinates);
}

template <typename T>
concept HasZ = requires(const T& value) { value.z(); };
template <typename T>
concept HasNormal = requires(const T& value) { value.unitNormal(); };

template <typename T>
concept HasOrient3D = requires(const T& value) { T::orient3D(value, value, value, value); };

static_assert(!HasZ<geometry::point_t<2>>);
static_assert(HasZ<geometry::point_t<3>>);
static_assert(!HasOrient3D<geometry::point_t<2>>);
static_assert(HasOrient3D<geometry::point_t<3>>);

TEST(Point, SupportsDifferentDimensionsAndArrayConstruction) {
    EXPECT_DOUBLE_EQ(makePoint<1>({5}).x(), 5);
    EXPECT_DOUBLE_EQ(makePoint<2>({3, 4}).y(), 4);
    EXPECT_DOUBLE_EQ(makePoint<3>({1, 2, 3}).z(), 3);
    EXPECT_DOUBLE_EQ(makePoint<4>({1, 2, 3, 4})[3], 4);
}

TEST(Point, CalculatesDistanceInEveryDimensionWithoutSquaredOverflow) {
    EXPECT_DOUBLE_EQ(makePoint<1>({2}).distance(makePoint<1>({-3})), 5);
    EXPECT_DOUBLE_EQ(makePoint<2>({0, 0}).distance(makePoint<2>({3, 4})), 5);
    EXPECT_DOUBLE_EQ(makePoint<3>({0, 0, 0}).distance(makePoint<3>({2, 3, 6})), 7);
    EXPECT_DOUBLE_EQ(makePoint<4>({0, 0, 0, 0}).distance(makePoint<4>({1, 2, 2, 4})), 5);
    EXPECT_DOUBLE_EQ(makePoint<2>({0, 0}).distance(makePoint<2>({3e200, 4e200})), 5e200);
}

TEST(Point, ComparesAllCoordinatesAndValidatesTolerance) {
    const auto point = makePoint<4>({1, 2, 3, 4});
    EXPECT_EQ(point, makePoint<4>({1, 2, 3, 4}));
    EXPECT_NE(point, makePoint<4>({1, 2, 3, 5}));
    EXPECT_TRUE(point.isApprox(makePoint<4>({1, 2, 3, 4.01}), 0.02));
    EXPECT_FALSE(point.isApprox(makePoint<4>({1, 2, 3, 4.01}), 0.001));
    EXPECT_THROW(point.isApprox(point, -1), std::invalid_argument);
    EXPECT_THROW(point.isApprox(point, std::numeric_limits<double>::infinity()),
                 std::invalid_argument);
    EXPECT_THROW(point.isApprox(point, std::numeric_limits<double>::quiet_NaN()),
                 std::invalid_argument);
}

TEST(Point, RejectsNonFiniteCoordinates) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    EXPECT_THROW(makePoint<1>({nan}), std::invalid_argument);
    EXPECT_THROW(makePoint<2>({0, inf}), std::invalid_argument);
    EXPECT_THROW(makePoint<3>({0, 0, -inf}), std::invalid_argument);
    EXPECT_THROW(makePoint<4>({0, 0, 0, nan}), std::invalid_argument);
}

TEST(Point, PreservesOrient3DSignAndCoplanarity) {
    const auto a = makePoint<3>({0, 0, 0});
    const auto b = makePoint<3>({1, 0, 0});
    const auto c = makePoint<3>({0, 1, 0});
    const auto d = makePoint<3>({0, 0, 1});
    EXPECT_DOUBLE_EQ(geometry::point_t<3>::orient3D(a, b, c, d), -1);
    EXPECT_DOUBLE_EQ(geometry::point_t<3>::orient3D(a, c, b, d), 1);
    EXPECT_DOUBLE_EQ(geometry::point_t<3>::orient3D(a, b, c, makePoint<3>({2, 2, 0})), 0);
}

static_assert(!HasNormal<geometry::triangle_t<2>>);
static_assert(HasNormal<geometry::triangle_t<3>>);
static_assert(!HasNormal<geometry::polygon_t<2>>);
static_assert(HasNormal<geometry::polygon_t<3>>);
static_assert(std::is_base_of_v<geometry::polygon_t<2>, geometry::triangle_t<2>>);
static_assert(std::is_base_of_v<geometry::polygon_t<3>, geometry::triangle_t<3>>);

template <std::size_t Dim>
concept HasTriangle = requires { typename geometry::triangle_t<Dim>; };
static_assert(HasTriangle<2> && HasTriangle<3>);
static_assert(!HasTriangle<1> && !HasTriangle<4>);

TEST(Triangle2D, PreservesVerticesSideLengthsAndArea) {
    const auto a = makePoint<2>({0, 0});
    const auto b = makePoint<2>({3, 0});
    const auto c = makePoint<2>({0, 4});
    const geometry::triangle_t<2> triangle(a, b, c);
    EXPECT_EQ(triangle.a(), a);
    EXPECT_EQ(triangle.b(), b);
    EXPECT_EQ(triangle.c(), c);
    EXPECT_EQ(triangle.numOfPoints(), 3U);
    EXPECT_EQ(triangle.point(1), b);
    EXPECT_EQ(triangle.points()[2], c);
    EXPECT_DOUBLE_EQ(triangle.lenAB(), 3);
    EXPECT_DOUBLE_EQ(triangle.lenBC(), 5);
    EXPECT_DOUBLE_EQ(triangle.lenCA(), 4);
    const geometry::polygon_t<2>& polygon = triangle;
    EXPECT_DOUBLE_EQ(polygon.area(), 6);
}

TEST(Triangle2D, AreaIsIndependentOfWindingAndVertexOrder) {
    const std::array<geometry::point_t<2>, 3> vertices{
        makePoint<2>({10, -7}), makePoint<2>({13, -7}), makePoint<2>({10, -3})};
    std::array<std::size_t, 3> order{0, 1, 2};
    do {
        const geometry::triangle_t<2> triangle(
            vertices[order[0]], vertices[order[1]], vertices[order[2]]);
        EXPECT_DOUBLE_EQ(triangle.area(), 6);
    } while (std::next_permutation(order.begin(), order.end()));
}

TEST(Triangle2D, RejectsCoincidentAndCollinearVertices) {
    const auto a = makePoint<2>({0, 0});
    const auto b = makePoint<2>({1, 1});
    EXPECT_THROW((geometry::triangle_t<2>(a, a, b)), std::invalid_argument);
    EXPECT_THROW((geometry::triangle_t<2>(a, b, b)), std::invalid_argument);
    EXPECT_THROW((geometry::triangle_t<2>(a, b, a)), std::invalid_argument);
    EXPECT_THROW((geometry::triangle_t<2>(a, b, makePoint<2>({2, 2}))),
                 std::invalid_argument);
    EXPECT_THROW((geometry::triangle_t<2>(a, makePoint<2>({0, 1}), makePoint<2>({0, 2}))),
                 std::invalid_argument);
}

TEST(Triangle3D, PreservesVerticesAreaAndNormal) {
    const auto a = makePoint<3>({0, 0, 0});
    const auto b = makePoint<3>({3, 0, 0});
    const auto c = makePoint<3>({0, 4, 0});
    const geometry::triangle_t<3> triangle(a, b, c);
    EXPECT_EQ(triangle.a(), a);
    EXPECT_EQ(triangle.b(), b);
    EXPECT_EQ(triangle.c(), c);
    EXPECT_EQ(triangle.numOfPoints(), 3U);
    EXPECT_DOUBLE_EQ(triangle.area(), 6);
    EXPECT_DOUBLE_EQ(triangle.lenAB(), 3);
    EXPECT_DOUBLE_EQ(triangle.lenBC(), 5);
    EXPECT_DOUBLE_EQ(triangle.lenCA(), 4);
    const geometry::polygon_t<3>& polygon = triangle;
    EXPECT_DOUBLE_EQ(polygon.area(), 6);
    EXPECT_TRUE(triangle.unitNormal().isApprox(geometry::vector_t(0, 0, 1)));
    EXPECT_THROW((geometry::triangle_t<3>(a, a, c)), std::invalid_argument);
}

TEST(Triangle, DeducesDimensionFromVertices) {
    const geometry::triangle_t plane(makePoint<2>({0, 0}),
        makePoint<2>({1, 0}), makePoint<2>({0, 1}));
    const geometry::triangle_t space(makePoint<3>({0, 0, 0}),
        makePoint<3>({1, 0, 0}), makePoint<3>({0, 1, 0}));
    static_assert(std::is_same_v<decltype(plane), const geometry::triangle_t<2>>);
    static_assert(std::is_same_v<decltype(space), const geometry::triangle_t<3>>);
    EXPECT_DOUBLE_EQ(plane.area(), 0.5);
    EXPECT_DOUBLE_EQ(space.area(), 0.5);
}


template <std::size_t Dim>
void checkPolygonGeometry() {
    auto point = [](double x, double y) {
        std::array<double, Dim> coordinates{};
        coordinates[0] = x;
        coordinates[1] = y;
        return geometry::point_t<Dim>(coordinates);
    };
    const auto a = point(0, 0), b = point(2, 0), c = point(1, 1);
    const auto d = point(2, 2), e = point(0, 2);
    const geometry::polygon_t<Dim> forward({a, b, c, d, e});
    const geometry::polygon_t<Dim> backward({e, d, c, b, a});
    EXPECT_EQ(forward.numOfPoints(), 5U);
    EXPECT_EQ(forward.points()[2], c);
    EXPECT_DOUBLE_EQ(forward.area(), 3);
    EXPECT_DOUBLE_EQ(backward.area(), 3);
    EXPECT_THROW((geometry::polygon_t<Dim>({a, b})), std::invalid_argument);
    EXPECT_THROW((geometry::polygon_t<Dim>({a, a, e})), std::invalid_argument);
    EXPECT_THROW((geometry::polygon_t<Dim>({a, point(1, 0), b})), std::invalid_argument);
    EXPECT_THROW((geometry::polygon_t<Dim>({a, d, e, b})), std::invalid_argument);
}

TEST(Polygon2D, ValidatesVerticesAndComputesConcaveArea) {
    checkPolygonGeometry<2>();
}

TEST(Polygon3D, PreservesValidationAndConcaveArea) {
    checkPolygonGeometry<3>();
    EXPECT_THROW((geometry::polygon_t<3>({makePoint<3>({0, 0, 0}),
        makePoint<3>({1, 0, 0}), makePoint<3>({1, 1, 0}), makePoint<3>({0, 1, 1})})),
        std::invalid_argument);
}

} // namespace
