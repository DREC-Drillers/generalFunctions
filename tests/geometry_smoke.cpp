#include <iostream>

import geometry;

int main() {
    const geometry::triangle_t triangle{
        {0, 0, 0},
        {3, 0, 0},
        {0, 4, 0}
    };

    // This 3-4-5 triangle has exactly representable expected results.
    if (triangle.area() != 6.0) {
        std::cerr << "Expected triangle area 6\n";
        return 1;
    }

    if (triangle.lenBC() != 5.0) {
        std::cerr << "Expected triangle hypotenuse 5\n";
        return 1;
    }

    return 0;
}

