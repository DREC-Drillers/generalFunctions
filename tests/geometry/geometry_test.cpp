#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

import geometry;

namespace {
using namespace geometry;
constexpr double inf = std::numeric_limits<double>::infinity();
constexpr double nan = std::numeric_limits<double>::quiet_NaN();

void expectVector(const vector_t &value, double x, double y, double z) {
  EXPECT_NEAR(value.x(), x, 1e-12);
  EXPECT_NEAR(value.y(), y, 1e-12);
  EXPECT_NEAR(value.z(), z, 1e-12);
}

TEST(Vector, ZeroUsesStrictAbsoluteThreshold) {
  for (double value : {0.0, -0.0, EPSILON / 2, -EPSILON / 2})
    EXPECT_TRUE(vector_t(value, 0, 0).isZero());
  for (double value : {EPSILON, -EPSILON, 1.0, inf, -inf, nan})
    EXPECT_FALSE(vector_t(value, 0, 0).isZero());
}

TEST(Point, CoordinatesAndExactEquality) {
  const point_t p{1, -2, 3};
  EXPECT_DOUBLE_EQ(p.x(), 1);
  EXPECT_DOUBLE_EQ(p.y(), -2);
  EXPECT_DOUBLE_EQ(p.z(), 3);
  EXPECT_TRUE(p == point_t(1, -2, 3));
  for (const point_t &q : {point_t{2, -2, 3}, {1, 2, 3}, {1, -2, 4}})
    EXPECT_FALSE(p == q);
  EXPECT_FALSE(p == point_t(1 + EPSILON / 2, -2, 3));
}

TEST(Point, RejectsEachNonfiniteCoordinate) {
  for (double invalid : {inf, -inf, nan}) {
    EXPECT_THROW((point_t{invalid, 0, 0}), std::invalid_argument);
    EXPECT_THROW((point_t{0, invalid, 0}), std::invalid_argument);
    EXPECT_THROW((point_t{0, 0, invalid}), std::invalid_argument);
  }
}

TEST(Point, ApproximateEqualityIncludesBoundaryAndChecksEveryCoordinate) {
  const point_t p{0, 0, 0};
  EXPECT_TRUE(p.isApprox({EPSILON, -EPSILON, EPSILON}));
  EXPECT_TRUE(p.isApprox(p, 0));
  EXPECT_TRUE(p.isApprox({1, -1, 1}, 1));
  for (const point_t &q :
       {point_t{2 * EPSILON, 0, 0}, {0, 2 * EPSILON, 0}, {0, 0, 2 * EPSILON}}) {
    EXPECT_FALSE(p.isApprox(q));
    EXPECT_FALSE(p.isApprox(q, 0));
  }
  for (double invalid : {-1.0, inf, -inf, nan})
    EXPECT_THROW(p.isApprox(p, invalid), std::invalid_argument);
}

TEST(Point, EuclideanDistanceIsSymmetricAndAvoidsIntermediateOverflow) {
  const point_t a{1, 2, 3}, b{4, 6, 15};
  EXPECT_DOUBLE_EQ(a.distance(a), 0);
  EXPECT_DOUBLE_EQ(a.distance(b), 13);
  EXPECT_DOUBLE_EQ(b.distance(a), 13);
  EXPECT_NEAR(point_t(0, 0, 0).distance({3e150, 4e150, 0}) / 1e150, 5, 1e-12);
}

TEST(Vector, ConstructorsCoordinatesAndEquality) {
  const vector_t v{point_t{1, 2, 3}, point_t{4, -2, 8}};
  expectVector(v, 3, -4, 5);
  EXPECT_TRUE(v == vector_t(3, -4, 5));
  for (const vector_t &w : {vector_t{2, -4, 5}, {3, 4, 5}, {3, -4, 6}})
    EXPECT_FALSE(v == w);
  EXPECT_FALSE(vector_t(0, 0, 0) == vector_t(EPSILON / 2, 0, 0));
}

TEST(Vector, ApproximateEqualityAndInvalidTolerances) {
  const vector_t v{0, 0, 0};
  EXPECT_TRUE(v.isApprox({EPSILON, -EPSILON, EPSILON}));
  EXPECT_TRUE(v.isApprox(v, 0));
  EXPECT_TRUE(v.isApprox({1, -1, 1}, 1));
  for (const vector_t &w :
       {vector_t{2 * EPSILON, 0, 0}, {0, 2 * EPSILON, 0}, {0, 0, 2 * EPSILON}})
    EXPECT_FALSE(v.isApprox(w));
  for (double invalid : {-1.0, inf, -inf, nan})
    EXPECT_THROW(v.isApprox(v, invalid), std::invalid_argument);
  EXPECT_FALSE(v.isApprox({nan, 0, 0}));
}

TEST(Vector, LengthZeroAndNormalization) {
  const vector_t v{3, -4, 12};
  EXPECT_DOUBLE_EQ(v.length(), 13);
  const auto unit = v.normalized();
  expectVector(unit, 3.0 / 13, -4.0 / 13, 12.0 / 13);
  EXPECT_NEAR(unit.length(), 1, 1e-12);
  expectVector(v, 3, -4, 12);
  EXPECT_THROW(vector_t(0, 0, 0).normalized(), std::invalid_argument);
  expectVector(vector_t(1e-150, 0, 0).normalized(), 1, 0, 0);
  EXPECT_NEAR(vector_t(3e150, 4e150, 0).length() / 1e150, 5, 1e-12);
  for (const vector_t &bad : {vector_t{inf, 0, 0},
                              {0, -inf, 0},
                              {0, 0, nan},
                              {1.7e308, 1.7e308, 1.7e308}})
    EXPECT_THROW(bad.normalized(), std::invalid_argument);
}

TEST(Vector, ArithmeticAndCompoundOperatorsReturnSelf) {
  vector_t v{1, -2, 3};
  const vector_t w{-4, 5, 2};
  expectVector(v + w, -3, 3, 5);
  expectVector(v - w, 5, -7, 1);
  expectVector(v * -2, -2, 4, -6);
  expectVector(v * 0, 0, 0, 0);
  EXPECT_EQ(&(v += w), &v);
  expectVector(v, -3, 3, 5);
  EXPECT_EQ(&(v -= w), &v);
  expectVector(v, 1, -2, 3);
  v += v;
  expectVector(v, 2, -4, 6);
  v -= v;
  expectVector(v, 0, 0, 0);
  expectVector(w, -4, 5, 2);
}

TEST(Vector, DotAndCrossProducts) {
  const vector_t v{1, 2, 3}, w{4, -5, 6};
  EXPECT_DOUBLE_EQ(v.dot(w), 12);
  EXPECT_DOUBLE_EQ(w.dot(v), 12);
  EXPECT_DOUBLE_EQ(v.dot(v), 14);
  expectVector(v.cross(w), 27, 6, -13);
  expectVector(w.cross(v), -27, -6, 13);
  expectVector(v.cross(v), 0, 0, 0);
  EXPECT_DOUBLE_EQ(v.cross(w).dot(v), 0);
  EXPECT_DOUBLE_EQ(v.dot({0, 0, 0}), 0);
}

TEST(Segment, EndpointsLengthDirectionAndDegeneracy) {
  const point_t a{1, 2, 3}, b{4, 6, 15};
  const segment_t s{a, b};
  EXPECT_TRUE(s.a() == a);
  EXPECT_TRUE(s.b() == b);
  EXPECT_DOUBLE_EQ(s.length(), 13);
  expectVector(s.direction(), 3, 4, 12);
  EXPECT_THROW((segment_t{a, a}), std::invalid_argument);
  EXPECT_THROW((segment_t{{0, 0, 0}, {EPSILON, EPSILON, EPSILON}}),
               std::invalid_argument);
  EXPECT_NO_THROW((segment_t{{0, 0, 0}, {2 * EPSILON, 0, 0}}));
}

struct IntersectionCase {
  point_t a, b, c, d;
  bool intersects;
};
class SegmentIntersection : public testing::TestWithParam<IntersectionCase> {};
TEST_P(SegmentIntersection,
       IsSymmetricAndIndependentOfOrientationAndNormalScale) {
  const auto &p = GetParam();
  for (bool reverseFirst : {false, true}) {
    const segment_t first{reverseFirst ? p.b : p.a, reverseFirst ? p.a : p.b};
    for (bool reverseSecond : {false, true}) {
      const segment_t second{reverseSecond ? p.d : p.c,
                             reverseSecond ? p.c : p.d};
      for (double scale : {1.0, -1.0, 7.0}) {
        EXPECT_EQ(first.isIntersectsCoplanar(second, {0, 0, scale}),
                  p.intersects);
        EXPECT_EQ(second.isIntersectsCoplanar(first, {0, 0, scale}),
                  p.intersects);
      }
    }
  }
}
INSTANTIATE_TEST_SUITE_P(
    PlanarCases, SegmentIntersection,
    testing::Values(
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {2, -1, 0}, {2, 1, 0}, true},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {0, 1, 0}, {4, 1, 0}, false},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {5, -1, 0}, {5, 1, 0}, false},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {4, 0, 0}, {5, 1, 0}, true},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {2, 0, 0}, {2, 1, 0}, true},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {2, 0, 0}, {6, 0, 0}, true},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {1, 0, 0}, {3, 0, 0}, true},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {0, 0, 0}, {4, 0, 0}, true},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {4, 0, 0}, {6, 0, 0}, true},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {5, 0, 0}, {6, 0, 0}, false},
        IntersectionCase{{0, 0, 0}, {4, 0, 0}, {-2, 0, 0}, {-1, 0, 0}, false},
        IntersectionCase{{0, 0, 0},
                         {4, 0, 0},
                         {-EPSILON / 2, 0, 0},
                         {-EPSILON / 2, 1, 0},
                         true},
        IntersectionCase{{0, 0, 0},
                         {4, 0, 0},
                         {4 + EPSILON / 2, 0, 0},
                         {4 + EPSILON / 2, 1, 0},
                         true}));

TEST(Segment, IntersectsInTranslatedTiltedPlane) {
  const segment_t a{{1, 2, 3}, {3, 4, 3}}, b{{1, 4, 5}, {3, 2, 1}};
  EXPECT_TRUE(a.isIntersectsCoplanar(b, {1, -1, 1}));
}

TEST(Segment, RejectsInvalidNormalAndEachNoncoplanarEndpoint) {
  const segment_t base{{0, 0, 0}, {2, 0, 0}};
  const segment_t planar{{1, -1, 0}, {1, 1, 0}};
  for (const vector_t &normal :
       {vector_t{0, 0, 0}, {nan, 0, 1}, {0, inf, 1}, {1, 0, 0}})
    EXPECT_THROW(base.isIntersectsCoplanar(planar, normal),
                 std::invalid_argument);
  EXPECT_THROW(
      base.isIntersectsCoplanar(segment_t({1, -1, 1}, {1, 1, 0}), {0, 0, 1}),
      std::invalid_argument);
  EXPECT_THROW(
      base.isIntersectsCoplanar(segment_t({1, -1, 0}, {1, 1, 1}), {0, 0, 1}),
      std::invalid_argument);
}

TEST(Polygon, RejectsFewerThanThreeVertices) {
  EXPECT_THROW(polygon_t({}), std::invalid_argument);
  EXPECT_THROW(polygon_t({{0, 0, 0}}), std::invalid_argument);
  EXPECT_THROW(polygon_t({{0, 0, 0}, {1, 0, 0}}), std::invalid_argument);
}

TEST(Polygon, RejectsDuplicateAdjacentAndCollinearVerticesIncludingWraparound) {
  for (const std::vector<point_t> &vertices :
       {std::vector<point_t>{{0, 0, 0}, {0, 0, 0}, {1, 1, 0}},
        {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 0, 0}},
        {{0, 0, 0}, {EPSILON / 2, 0, 0}, {1, 1, 0}},
        {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}},
        {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {1, 1, 0}},
        {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {-1, 0, 0}}})
    EXPECT_THROW((polygon_t{vertices}), std::invalid_argument);
}

TEST(Polygon, RejectsNoncoplanarVerticesAndAcceptsSmallPlaneDeviation) {
  EXPECT_THROW((polygon_t{{{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 1}}}),
               std::invalid_argument);
  EXPECT_THROW((polygon_t{{{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, EPSILON}}}),
               std::invalid_argument);
  EXPECT_NO_THROW(
      (polygon_t{{{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, EPSILON / 4}}}));
}

TEST(Polygon, RejectsCrossingAndTouchingNonadjacentEdges) {
  EXPECT_THROW((polygon_t{{{0, 0, 0}, {2, 2, 0}, {0, 2, 0}, {2, 0, 0}}}),
               std::invalid_argument);
  EXPECT_THROW(
      (polygon_t{{{0, 0, 0}, {4, 0, 0}, {4, 4, 0}, {2, 0, 0}, {0, 4, 0}}}),
      std::invalid_argument);
  EXPECT_THROW(
      (polygon_t{{{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 0, 0}, {0, 2, 0}}}),
      std::invalid_argument);
}

TEST(Polygon, PreservesVerticesAndComputesAreaForEitherWinding) {
  std::vector<point_t> vertices{{1, 2, 3}, {5, 2, 3}, {5, 5, 3}, {1, 5, 3}};
  const polygon_t polygon{vertices};
  EXPECT_EQ(polygon.numOfPoints(), 4U);
  EXPECT_EQ(polygon.points(), vertices);
  for (std::size_t i = 0; i < vertices.size(); ++i) {
    EXPECT_TRUE(polygon.point(i) == vertices[i]);
    EXPECT_EQ(&polygon.point(i), &polygon.points()[i]);
  }
  EXPECT_DOUBLE_EQ(polygon.area(), 12);
  expectVector(polygon.unitNormal(), 0, 0, 1);
  std::reverse(vertices.begin(), vertices.end());
  const polygon_t reversed{vertices};
  EXPECT_DOUBLE_EQ(reversed.area(), 12);
  expectVector(reversed.unitNormal(), 0, 0, -1);
}

TEST(Polygon, ConcaveAreaIsIndependentOfStartingVertexAndWinding) {
  std::vector<point_t> vertices{
      {0, 0, 0}, {4, 0, 0}, {4, 3, 0}, {2, 2, 0}, {0, 3, 0}};
  for (int winding = 0; winding < 2; ++winding) {
    for (std::size_t start = 0; start < vertices.size(); ++start) {
      const polygon_t polygon{vertices};
      EXPECT_DOUBLE_EQ(polygon.area(), 10);
      EXPECT_NEAR(polygon.unitNormal().length(), 1, 1e-12);
      std::rotate(vertices.begin(), vertices.begin() + 1, vertices.end());
    }
    std::reverse(vertices.begin(), vertices.end());
  }
}

TEST(Polygon, AreaAndNormalInVerticalAndTiltedPlanes) {
  const polygon_t vertical{{{2, 1, 3}, {2, 4, 3}, {2, 4, 7}, {2, 1, 7}}};
  EXPECT_DOUBLE_EQ(vertical.area(), 12);
  expectVector(vertical.unitNormal(), 1, 0, 0);
  const polygon_t tilted{{{1, 2, 3}, {3, 2, 5}, {3, 5, 5}, {1, 5, 3}}};
  EXPECT_NEAR(tilted.area(), 6 * std::sqrt(2.0), 1e-12);
  expectVector(tilted.unitNormal(), -1 / std::sqrt(2.0), 0, 1 / std::sqrt(2.0));
}

TEST(Triangle, VerticesSideLengthsAreaAndWinding) {
  const point_t a{1, 2, 3}, b{4, 2, 3}, c{1, 6, 3};
  const triangle_t triangle{a, b, c};
  EXPECT_EQ(triangle.numOfPoints(), 3U);
  EXPECT_TRUE(triangle.a() == a);
  EXPECT_TRUE(triangle.b() == b);
  EXPECT_TRUE(triangle.c() == c);
  EXPECT_DOUBLE_EQ(triangle.lenAB(), 3);
  EXPECT_DOUBLE_EQ(triangle.lenBC(), 5);
  EXPECT_DOUBLE_EQ(triangle.lenCA(), 4);
  EXPECT_DOUBLE_EQ(triangle.area(), 6);
  expectVector(triangle.unitNormal(), 0, 0, 1);
  const triangle_t reversed{a, c, b};
  EXPECT_DOUBLE_EQ(reversed.area(), 6);
  expectVector(reversed.unitNormal(), 0, 0, -1);
  EXPECT_THROW((triangle_t{a, a, c}), std::invalid_argument);
  EXPECT_THROW((triangle_t{{0, 0, 0}, {1, 1, 1}, {2, 2, 2}}),
               std::invalid_argument);
}
}
