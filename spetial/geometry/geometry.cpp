module;

#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <vector>

export module geometry;

namespace geometry {

constexpr double EPSILON = 1e-9;

bool isDoubleZero(double num) { return std::fabs(num) < EPSILON; }

}

export namespace geometry {

template <std::size_t Dim>
class point_t {
    static_assert(Dim > 0, "Point dimension must be positive");

private:
    std::array<double, Dim> coordinates_;

public:
    explicit point_t(std::array<double, Dim> coordinates)
        : coordinates_(coordinates) {
        for (double coordinate : coordinates_) {
            if (!std::isfinite(coordinate)) {
                throw std::invalid_argument("Point coordinates must be finite");
            }
        }
    }

    double operator[](std::size_t index) const { return coordinates_[index]; }
    double x() const { return coordinates_[0]; }
    double y() const requires (Dim >= 2) { return coordinates_[1]; }
    double z() const requires (Dim >= 3) { return coordinates_[2]; }

    point_t<Dim - 1> project(std::size_t droppedAxis) const requires (Dim > 1);

    bool operator==(const point_t& otherPoint) const = default;

    bool isApprox(const point_t& otherPoint, double tolerance = EPSILON) const {
        if (!std::isfinite(tolerance) || tolerance < 0.0) {
            throw std::invalid_argument(
                "Comparison tolerance must be finite and non-negative");
        }
        for (std::size_t i = 0; i < Dim; ++i) {
            if (std::fabs(coordinates_[i] - otherPoint.coordinates_[i]) > tolerance) {
                return false;
            }
        }
        return true;
    }

    double distance(const point_t& otherPoint) const {
        double result = 0.0;
        for (std::size_t i = 0; i < Dim; ++i) {
            result = std::hypot(result, coordinates_[i] - otherPoint.coordinates_[i]);
        }
        return result;
    }

    double signedDistanceToPlane(const point_t<3>& a, const point_t<3>& b,
                                 const point_t<3>& c);

    static double orient2D(const point_t& a, const point_t& b,
                           const point_t& c) requires (Dim == 2);

    static double orient3D(const point_t& a, const point_t& b,
                           const point_t& c, const point_t& d) requires (Dim == 3);
};

class axisAlignedBoundingBox_t {
public:
    axisAlignedBoundingBox_t(const point_t<3>& min, const point_t<3>& max);

    const point_t<3>& min() const noexcept;
    const point_t<3>& max() const noexcept;

    bool overlaps(const axisAlignedBoundingBox_t& other) const noexcept;
    void expand(const axisAlignedBoundingBox_t& other) noexcept;
    point_t<3> center() const noexcept;

private:
    point_t<3> min_;
    point_t<3> max_;
};

class vector_t {
    double x_ = NAN;
    double y_ = NAN;
    double z_ = NAN;

public:
    vector_t(double x, double y, double z) : x_(x), y_(y), z_(z) {};
    vector_t(const point_t<3> &origin, const point_t<3> &end);

    double x() const { return x_; };
    double y() const { return y_; };
    double z() const { return z_; };

    double length() const;
    bool isZero() const { return geometry::isDoubleZero(length()); }
    vector_t normalized() const;

    bool operator==(const vector_t &otherVector) const;
    bool isApprox(const vector_t &otherVector, double tolerance = EPSILON) const;

    vector_t& operator+=(const vector_t &otherVector);
    vector_t& operator-=(const vector_t &otherVector);
    vector_t operator*(double number) const;

    vector_t cross(const vector_t &otherVector) const;
    double dot(const vector_t &otherVector) const;
};

template <std::size_t Dim>
point_t<Dim - 1> point_t<Dim>::project(std::size_t droppedAxis) const requires (Dim > 1) {
    if (droppedAxis >= Dim) {
        throw std::out_of_range("Projection axis is out of range");
    }
    std::array<double, Dim - 1> coordinates{};
    for (std::size_t source = 0, target = 0; source < Dim; ++source) {
        if (source != droppedAxis) {
            coordinates[target++] = coordinates_[source];
        }
    }
    return point_t<Dim - 1>(coordinates);
}

template <std::size_t Dim>
double point_t<Dim>::orient3D(const point_t& a, const point_t& b,
                            const point_t& c, const point_t& d) requires (Dim == 3) {
    const vector_t ba{b, a};
    const vector_t ca{c, a};
    const vector_t da{d, a};
    return ba.cross(ca).dot(da);
}

vector_t operator+(const vector_t& x, const vector_t& y);
vector_t operator-(const vector_t& x, const vector_t& y);


class segment_t {

private:
    point_t<3> a_;
    point_t<3> b_;

    double signedDistanceToLine(const point_t<3>& p, const vector_t& planeNormal) const;
    bool projectionFallsOnSegment(const point_t<3>& p) const;

public:
    segment_t(const point_t<3> &a, const point_t<3> &b);

    const point_t<3> &a() const { return a_; };
    const point_t<3> &b() const { return b_; };

    double length() const { return a_.distance(b_); };
    vector_t direction() const;

    bool isIntersectsCoplanar(const segment_t& other, const vector_t& planeNormal) const;
};

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
class polygon_t {
private:
    std::vector<point_t<Dim>> points_;

    vector_t findNormal() const;
    void validateCoplanarity() const requires (Dim == 3);
    void validateSelfIntersections() const;

public:
    polygon_t(std::vector<point_t<Dim>> points);
    virtual ~polygon_t() = default;

    polygon_t(const polygon_t&) = default;
    polygon_t(polygon_t&&) = default;
    polygon_t& operator=(const polygon_t&) = delete;
    polygon_t& operator=(polygon_t&&) = delete;

    std::size_t numOfPoints() const { return points_.size(); }
    const std::vector<point_t<Dim>>& points() const { return points_; }
    const point_t<Dim>& point(std::size_t index) const { return points_[index]; }

    double area() const;
    vector_t unitNormal() const requires (Dim == 3);
};

template <std::size_t Dim>
    requires (Dim == 2 || Dim == 3)
class triangle_t : public polygon_t<Dim> {
public:
    triangle_t(const point_t<Dim>& firstPoint,
               const point_t<Dim>& secondPoint,
               const point_t<Dim>& thirdPoint);

    const point_t<Dim>& a() const { return this->point(0); }
    const point_t<Dim>& b() const { return this->point(1); }
    const point_t<Dim>& c() const { return this->point(2); }

    double lenAB() const;
    double lenBC() const;
    double lenCA() const;
};

} // namespace geometry
