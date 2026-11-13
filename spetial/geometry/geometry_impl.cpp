module;

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <string>
#include <utility>

module geometry;

namespace geometry {

template <std::size_t Dim>
double point_t<Dim>::orient2D(const point_t& a, const point_t& b,
                       const point_t& c) requires (Dim == 2) {
    return (b.x() - a.x()) * (c.y() - a.y()) -
           (b.y() - a.y()) * (c.x() - a.x());
}

template double point_t<2>::orient2D(const point_t<2>& a,
                                      const point_t<2>& b,
                                      const point_t<2>& c);

double
point_t::signedDistanceToPlane(const point_t<3>& a,
                               const point_t<3>& b, const point_t<3>& c) {
    const auto normal = vector_t(a, b).cross(vector_t(a, c));
    if (normal.length() == 0.0) {
        return 0.0;
    }

    return vector_t(point, a).dot(normal.normalized());
}

segment_t::segment_t(const point_t<3> &a, const point_t<3> &b)
    : a_(a),
      b_(b) {
    if (a == b) {
        throw std::invalid_argument
            ("The line segment must be constructed using two distinct points.");
    }
}

vector_t
segment_t::direction() const {
    vector_t dir{a_, b_};
    return dir;
}

double
segment_t::signedDistanceToLine(const point_t<3>& p, const vector_t& planeNormal) const {
    vector_t lineDir = direction();
    double dist = (lineDir.cross(vector_t{a_, p})).dot(planeNormal) / lineDir.length();

    return dist;
}

bool
segment_t::projectionFallsOnSegment(const point_t<3>& p) const {
    vector_t lineDir = direction();
    double length = lineDir.length();

    vector_t unitDir = lineDir.normalized();
    double projection = vector_t{a_, p}.dot(unitDir);

    return (projection >= 0.0 || isDoubleZero(projection)) &&
           (projection <= length || isDoubleZero(projection - length));
}

bool
segment_t::isIntersectsCoplanar(const segment_t& other, const vector_t& planeNormal) const {
    const vector_t unitNormal = planeNormal.normalized();

    if (std::abs(vector_t{a_, b_}.dot(unitNormal)) > EPSILON ||
        std::abs(vector_t{a_, other.a_}.dot(unitNormal)) > EPSILON ||
        std::abs(vector_t{a_, other.b_}.dot(unitNormal)) > EPSILON) {
        throw std::invalid_argument(
            "Segment endpoints must lie in a common plane with the supplied normal");
    }

    double signedDistA = other.signedDistanceToLine(a_, unitNormal);
    double signedDistB = other.signedDistanceToLine(b_, unitNormal);
    double signedDistC = signedDistanceToLine(other.a_, unitNormal);
    double signedDistD = signedDistanceToLine(other.b_, unitNormal);

    return (signedDistC * signedDistD < 0 && signedDistA * signedDistB < 0) ||
           (isDoubleZero(signedDistC) && projectionFallsOnSegment(other.a_)) ||
           (isDoubleZero(signedDistD) && projectionFallsOnSegment(other.b_)) ||
           (isDoubleZero(signedDistA) && other.projectionFallsOnSegment(a_)) ||
           (isDoubleZero(signedDistB) && other.projectionFallsOnSegment(b_));
}

vector_t::vector_t(const point_t<3> &origin, const point_t<3> &end) {
    x_ = end.x() - origin.x();
    y_ = end.y() - origin.y();
    z_ = end.z() - origin.z();
}

bool
vector_t::operator==(const vector_t &otherVector) const {
    return x_ == otherVector.x_ && y_ == otherVector.y_ && z_ == otherVector.z_;
}

bool
vector_t::isApprox(const vector_t &otherVector, double tolerance) const {
    if (!std::isfinite(tolerance) || tolerance < 0.0) {
        throw std::invalid_argument("Comparison tolerance must be finite and non-negative");
    }
    return std::fabs(x_ - otherVector.x_) <= tolerance &&
           std::fabs(y_ - otherVector.y_) <= tolerance &&
           std::fabs(z_ - otherVector.z_) <= tolerance;
}

double
vector_t::length() const {
    return std::hypot(x_, y_, z_);
}

vector_t
vector_t::normalized() const {
    double len = length();
    if (!std::isfinite(len)) {
        throw std::invalid_argument("Vector length must be finite for normalization");
    }
    if (len == 0.0) {
        throw std::invalid_argument("Cannot normalize a zero vector");
    }

    vector_t norm{x_ / len, y_ / len, z_ / len};
    return norm;
}

vector_t
vector_t::cross(const vector_t &otherVector) const {
    return vector_t(
        y_ * otherVector.z_ - z_ * otherVector.y_,
        z_ * otherVector.x_ - x_ * otherVector.z_,
        x_ * otherVector.y_ - y_ * otherVector.x_
    );
}

double
vector_t::dot(const vector_t &otherVector) const {
    return x() * otherVector.x() + y() * otherVector.y() + z() * otherVector.z();
}

vector_t&
vector_t::operator+=(const vector_t &otherVector) {
    x_ += otherVector.x();
    y_ += otherVector.y();
    z_ += otherVector.z();

    return *this;
}

vector_t
operator+(const vector_t& x, const vector_t& y) {
    vector_t tmp{x};
    tmp += y;
    return tmp;
}

vector_t&
vector_t::operator-=(const vector_t &otherVector) {
    x_ -= otherVector.x();
    y_ -= otherVector.y();
    z_ -= otherVector.z();

    return *this;
}

vector_t
operator-(const vector_t& x, const vector_t& y) {
    vector_t tmp{x};
    tmp -= y;
    return tmp;
}

vector_t
vector_t::operator*(double number) const {
    vector_t newVec{x() * number, y() * number, z() * number};
    return newVec;
}


template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
triangle_t<Dim>::triangle_t(const point_t<Dim>& firstPoint,
                            const point_t<Dim>& secondPoint,
                            const point_t<Dim>& thirdPoint)
    : polygon_t<Dim>({firstPoint, secondPoint, thirdPoint}) {}

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
double triangle_t<Dim>::lenAB() const { return a().distance(b()); }

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
double triangle_t<Dim>::lenBC() const { return b().distance(c()); }

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
double triangle_t<Dim>::lenCA() const { return c().distance(a()); }

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
vector_t polygon_t<Dim>::findNormal() const {
    const auto& firstVertex = point(0);
    std::size_t numOfVertDiffFromFirst = 1;
    while (numOfVertDiffFromFirst < numOfPoints() &&
           point(numOfVertDiffFromFirst) == firstVertex) {
        ++numOfVertDiffFromFirst;
    }
    if (numOfVertDiffFromFirst == numOfPoints()) {
        return vector_t(0, 0, 0);
    }

    for (std::size_t i = numOfVertDiffFromFirst + 1; i < numOfPoints(); ++i) {
        if constexpr (Dim == 2) {
            const double orientation = point_t<2>::orient2D(
                firstVertex, point(numOfVertDiffFromFirst), point(i));
            if (orientation != 0.0) {
                return vector_t(0, 0, orientation > 0.0 ? 1 : -1);
            }
        } else {
            const vector_t direction(firstVertex, point(numOfVertDiffFromFirst));
            const auto normal = direction.cross(vector_t(firstVertex, point(i)));
            if (normal.length() != 0.0) {
                return normal.normalized();
            }
        }
    }
    return vector_t(0, 0, 0);
}

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
void polygon_t<Dim>::validateCoplanarity() const requires (Dim == 3) {
    const auto normal = findNormal();
    for (std::size_t i = 1; i < numOfPoints(); ++i) {
        if (!isDoubleZero(vector_t(point(0), point(i)).dot(normal))) {
            throw std::invalid_argument("Polygon vertices must lie in the same plane");
        }
    }
}

namespace {

bool doSegmentsIntersect2D(const point_t<2>& a, const point_t<2>& b,
                          const point_t<2>& c, const point_t<2>& d) {
    auto containsProjection = [](const point_t<2>& first, const point_t<2>& second,
                                 const point_t<2>& point) {
        const double length = first.distance(second);
        const double projection = ((point.x() - first.x()) * (second.x() - first.x()) +
                                   (point.y() - first.y()) * (second.y() - first.y())) / length;
        return (projection >= 0 || isDoubleZero(projection)) &&
               (projection <= length || isDoubleZero(projection - length));
    };
    const double abC = point_t<2>::orient2D(a, b, c) / a.distance(b);
    const double abD = point_t<2>::orient2D(a, b, d) / a.distance(b);
    const double cdA = point_t<2>::orient2D(c, d, a) / c.distance(d);
    const double cdB = point_t<2>::orient2D(c, d, b) / c.distance(d);
    return (abC * abD < 0 && cdA * cdB < 0) ||
           (isDoubleZero(abC) && containsProjection(a, b, c)) ||
           (isDoubleZero(abD) && containsProjection(a, b, d)) ||
           (isDoubleZero(cdA) && containsProjection(c, d, a)) ||
           (isDoubleZero(cdB) && containsProjection(c, d, b));
}

} // namespace

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
void polygon_t<Dim>::validateSelfIntersections() const {
    const auto normal = findNormal();
    if (normal.length() == 0.0) {
        return;
    }

    std::vector<point_t<Dim>> vertices;
    for (const auto& vertex : points_) {
        if (vertices.empty() || !vertices.back().isApprox(vertex)) {
            vertices.push_back(vertex);
        }
    }
    if (vertices.size() > 1 && vertices.front().isApprox(vertices.back())) {
        vertices.pop_back();
    }

    const std::size_t count = vertices.size();
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t nextI = (i + 1) % count;
        for (std::size_t j = i + 1; j < count; ++j) {
            const std::size_t nextJ = (j + 1) % count;
            if (nextI == j || nextJ == i) {
                continue;
            }
            bool intersects;
            if constexpr (Dim == 2) {
                intersects = doSegmentsIntersect2D(vertices[i], vertices[nextI],
                                                   vertices[j], vertices[nextJ]);
            } else {
                const segment_t first(vertices[i], vertices[nextI]);
                const segment_t second(vertices[j], vertices[nextJ]);
                intersects = first.isIntersectsCoplanar(second, normal);
            }
            if (intersects) {
                throw std::invalid_argument("Polygon edges must not intersect");
            }
        }
    }
}

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
polygon_t<Dim>::polygon_t(std::vector<point_t<Dim>> points) : points_(std::move(points)) {
    if (points_.size() < 3) {
        throw std::invalid_argument("The polygon must have at least three vertices");
    }
    if constexpr (Dim == 3) {
        validateCoplanarity();
    }
    validateSelfIntersections();
}

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
double polygon_t<Dim>::area() const {
    if constexpr (Dim == 2) {
        double twiceArea = 0;
        for (std::size_t i = 1; i + 1 < numOfPoints(); ++i) {
            twiceArea += point_t<2>::orient2D(point(0), point(i), point(i + 1));
        }
        return std::fabs(twiceArea) / 2;
    } else {
        vector_t crossSum(0, 0, 0);
        for (std::size_t i = 1; i + 1 < numOfPoints(); ++i) {
            crossSum += vector_t(point(0), point(i)).cross(vector_t(point(0), point(i + 1)));
        }
        return crossSum.length() / 2;
    }
}

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
vector_t polygon_t<Dim>::unitNormal() const requires (Dim == 3) {
    const auto normal = findNormal();
    if (normal.length() == 0.0) {
        throw std::invalid_argument("A degenerate polygon has no unique plane normal");
    }
    return normal;
}

template class polygon_t<2>;
template class polygon_t<3>;
template class triangle_t<2>;
template class triangle_t<3>;

axisAlignedBoundingBox_t::axisAlignedBoundingBox_t(
    const point_t<3>& min, const point_t<3>& max) : min_(min), max_(max) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (min_[axis] > max_[axis]) {
            throw std::invalid_argument("axisAlignedBoundingBox_t: min must be <= max");
        }
    }
}

const point_t<3>& axisAlignedBoundingBox_t::min() const noexcept { return min_; }

const point_t<3>& axisAlignedBoundingBox_t::max() const noexcept { return max_; }

bool axisAlignedBoundingBox_t::overlaps(const axisAlignedBoundingBox_t& other) const noexcept {
    for (std::size_t axis = 0; axis < 3; ++axis) {
        if (max_[axis] < other.min_[axis] || other.max_[axis] < min_[axis]) {
            return false;
        }
    }
    return true;
}

void axisAlignedBoundingBox_t::expand(const axisAlignedBoundingBox_t& other) noexcept {
    std::array<double, 3> lower{};
    std::array<double, 3> upper{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        lower[axis] = std::min(min_[axis], other.min_[axis]);
        upper[axis] = std::max(max_[axis], other.max_[axis]);
    }
    min_ = point_t<3>(lower);
    max_ = point_t<3>(upper);
}

point_t<3> axisAlignedBoundingBox_t::center() const noexcept {
    std::array<double, 3> result{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        const double lower = min_[axis];
        const double upper = max_[axis];
        result[axis] = lower <= 0 && upper >= 0 ? (lower + upper) / 2
                                               : lower + (upper - lower) / 2;
    }
    return point_t<3>(result);
}

} // namespace geometry
