#include <iostream>
#include <ostream>
#include "convex_jarvis.hpp"
#include "point.hpp"

int main() {
    int n;
    std::cout << "Enter number of points: ";
    std::cin >> n;
    
    if (n < 3) {
        std::cout << "At least 3 points are required to form a convex hull.\n";
        return 1;
    }
    
    std::vector<Point> points;
    std::cout << "Enter points as pairs of x y coordinates:\n";
    
    for (int i = 0; i < n; ++i) {
        float x, y;
        std::cout << "Point " << (i + 1) << ": ";
        std::cin >> x >> y;
        points.emplace_back(x, y);
    }
    
    // making the convex hull using Jarvis March
    ConvexJarvis cj(points);
    
    // find the convex hull points
    cj.findConvexJarvis();
    std::vector<Point> result = cj.chPoints; // chPoints is public in ConvexJarvis
    
    // print answer
    std::cout << "Convex Hull (Jarvis March):\n";
    for (const auto& p : result) {
        std::cout << "(" << p.getX() << ", " << p.getY() << ")\n";
    }
    
    // area
    double area = cj.polygonArea();
    std::cout << "area: " << area << std::endl;
    
    return 0;
}
