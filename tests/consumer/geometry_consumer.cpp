#include <array>

import geometry;

int main() {
    const geometry::point_t<3> lower(std::array<double, 3>{0, 0, 0});
    const geometry::point_t<3> upper(std::array<double, 3>{1, 1, 1});
    const geometry::axisAlignedBoundingBox_t box(lower, upper);
    return box.overlaps(box) && box.center()[0] == 0.5 ? 0 : 1;
}
