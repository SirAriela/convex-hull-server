
#ifndef CONVEX_HULL_HPP
#define CONVEX_HULL

#include <iostream>
#include <vector>


struct Point {
    float x, y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

class ConvexHull {
private:
    std::vector<Point> graph;

public:
    ConvexHull(std::vector<Point> graph);
    
};

#endif