#include "convex_jarvis.hpp"
#include <algorithm>
#include <cmath>

// Constructor
ConvexJarvis::ConvexJarvis(std::vector<Point> graph) : graph(graph) {}

// Orientation of 3 points
int ConvexJarvis::orientation(Point a, Point b, Point c) {
    double v = a.getX() * (b.getY() - c.getY()) +
               b.getX() * (c.getY() - a.getY()) +
               c.getX() * (a.getY() - b.getY());
    if (v < 0) return -1; // Clockwise
    if (v > 0) return +1; // Counterclockwise
    return 0; // Collinear
}

// Jarvis March (Gift Wrapping) algorithm
void ConvexJarvis::findConvexJarvis() {
    int n = graph.size();
    chPoints.clear();
    
    if (n < 3) return;
    
    // Find the leftmost point
    int leftmost = 0;
    for (int i = 1; i < n; i++) {
        if (graph[i].getX() < graph[leftmost].getX() ||
            (graph[i].getX() == graph[leftmost].getX() && graph[i].getY() < graph[leftmost].getY())) {
            leftmost = i;
        }
    }
    
    // Start from leftmost point, keep moving counterclockwise
    int current = leftmost;
    do {
        // Add current point to result
        chPoints.push_back(graph[current]);
        
        // Find the most counterclockwise point from graph[current]
        int next = (current + 1) % n;
        
        for (int i = 0; i < n; i++) {
            // If i is more counterclockwise than current 'next', then update next
            int orient = orientation(graph[current], graph[i], graph[next]);
            
            if (orient == 1) {
                // i is more counterclockwise than next
                next = i;
            }
            else if (orient == 0) {
                // graph[current], graph[i] and graph[next] are collinear
                // Choose the farthest point
                if (graph[current].distanceTo(graph[i]) > graph[current].distanceTo(graph[next])) {
                    next = i;
                }
            }
        }
        
        current = next;
        
    } while (current != leftmost); // Keep going until we come back to start
}

// Calculate area of the convex hull polygon
double ConvexJarvis::polygonArea() const {
    double area = 0.0;
    int n = chPoints.size();
    
    if (n < 3) return 0.0;
    
    // Using shoelace formula
    for (int i = 0; i < n; ++i) {
        const Point& p1 = chPoints[i];
        const Point& p2 = chPoints[(i + 1) % n];
        area += (p1.getX() * p2.getY()) - (p2.getX() * p1.getY());
    }
    
    return std::abs(area) / 2.0;
}