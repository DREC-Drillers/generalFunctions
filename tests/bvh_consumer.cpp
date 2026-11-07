#include <cstddef>
#include <vector>
#include "spetial/bvh.hpp"
int main() {
    const geometry::axisAlignedBoundingBox_t box(geometry::point_t<3>({0, 0, 0}), geometry::point_t<3>({1, 1, 1}));
    const general_functions::Bvh index({box});
    std::size_t hits = 0;
    index.query(box, [&](std::size_t id) { hits += id == 0; return true; });
    return hits == 1 ? 0 : 1;
}
