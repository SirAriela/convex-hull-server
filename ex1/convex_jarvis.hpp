#pragma once

#include "point.hpp"
#include <vector>

class ConvexJarvis {
public:
    // Constructor
    ConvexJarvis(std::vector<Point> graph);

    // Jarvis March (Gift Wrapping) algorithm
    void findConvexJarvis();

    // Calculate area of the convex hull polygon
    double polygonArea() const;

    // Points of the convex hull
    std::vector<Point> chPoints;

private:
    std::vector<Point> graph;

    // Orientation of 3 points
    int orientation(Point a, Point b, Point c);
};
    