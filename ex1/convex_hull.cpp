#include "convex_hull.hpp"
#include <algorithm>
#include <cmath>

// Constructor
ConvexHull::ConvexHull(std::vector<Point> graph) : graph(graph) {}

// Orientation of 3 points
int ConvexHull::orientation(Point a, Point b, Point c) {
    double v = a.getX() * (b.getY() - c.getY()) +
               b.getX() * (c.getY() - a.getY()) +
               c.getX() * (a.getY() - b.getY());
    if (v < 0) return -1;
    if (v > 0) return +1;
    return 0;
}


void ConvexHull::findConvexHull() {
    int n = graph.size();
    chPoints.clear();

    if (n < 3) return;

    Point p0 = *std::min_element(graph.begin(), graph.end(), [](Point a, Point b) {
        return std::make_pair(a.getY(), a.getX()) < std::make_pair(b.getY(), b.getX());
    });

    std::sort(graph.begin(), graph.end(), [this, &p0](const Point& a, const Point& b) {
        int o = orientation(p0, a, b);
        if (o == 0)
            return p0.distanceTo(a) < p0.distanceTo(b);
        return o < 0;
    });

    std::vector<Point> stack;
    for (int i = 0; i < n; ++i) {
        while (stack.size() > 1 &&
               orientation(stack[stack.size() - 2], stack.back(), graph[i]) >= 0)
            stack.pop_back();
        stack.push_back(graph[i]);
    }

    if (stack.size() >= 3)
        chPoints = stack;
}


double ConvexHull::polygonArea() const {
    double area = 0.0;
    int n = chPoints.size();
    for (int i = 0; i < n; ++i) {
        const Point& p1 = chPoints[i];
        const Point& p2 = chPoints[(i + 1) % n];
        area += (p1.getX() * p2.getY()) - (p2.getX() * p1.getY());
    }
    return std::abs(area) / 2.0;
}
