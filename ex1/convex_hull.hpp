#ifndef CONVEX_HULL_HPP
#define CONVEX_HULL_HPP

#include <iostream>
#include <vector>
#include "point.hpp"

class ConvexHull {
private:
    std::vector<Point> graph;    
    std::vector<Point> chPoints; 

public:
    ConvexHull(std::vector<Point> graph);
    int orientation(Point a, Point b, Point c);
    
    void findConvexHull(); 
    double polygonArea() const;

    const std::vector<Point>& getConvexHullPoints() const { return chPoints; }
};

#endif
