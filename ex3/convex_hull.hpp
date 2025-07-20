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
    
    // New methods for interactive functionality
    void addPoint(const Point& point);
    void removePoint(const Point& point);
    void setGraph(const std::vector<Point>& newGraph);
    const std::vector<Point>& getGraph() const { return graph; }
    void printConvexHull() const;
};

#endif
