#include <iostream>
#include <ostream>
#include <string>
#include <sstream>
#include "convex_hull.hpp"
#include "point.hpp"

int main() {
    std::vector<Point> points;
    ConvexHull ch(points);
    std::string command;
    
    std::cout << "=== Interactive Convex Hull Calculator ===" << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "  Newgraph n     - Create new graph with n points" << std::endl;
    std::cout << "  CH             - Calculate and display convex hull" << std::endl;
    std::cout << "  Newpoint x y   - Add a new point" << std::endl;
    std::cout << "  Removepoint x y - Remove a point" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Enter command: ";
    
    while (std::getline(std::cin, command)) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "Newgraph") {
            int n;
            iss >> n;
            
            if (n < 3) {
                std::cout << "At least 3 points are required to form a convex hull." << std::endl;
                continue;
            }
            
            points.clear();
            std::cout << "Creating new graph with " << n << " points. Please enter " << n << " points (x y format):" << std::endl;
            
            // Read exactly n points from subsequent lines
            for (int i = 0; i < n; ++i) {
                std::string pointLine;
                if (std::getline(std::cin, pointLine)) {
                    std::istringstream pointIss(pointLine);
                    float x, y;
                    if (pointIss >> x >> y) {
                        points.emplace_back(x, y);
                        std::cout << "Point " << (i + 1) << "/" << n << " added: (" << x << ", " << y << ")" << std::endl;
                    } else {
                        std::cout << "Error reading point " << (i + 1) << ". Please enter valid coordinates (x y format)." << std::endl;
                        break;
                    }
                } else {
                    std::cout << "Error reading point " << (i + 1) << ". Please enter valid coordinates (x y format)." << std::endl;
                    break;
                }
            }
            
            if (points.size() == static_cast<size_t>(n)) {
                ch.setGraph(points);
                ch.findConvexHull();
                std::cout << "Graph created successfully with " << n << " points!" << std::endl;
            } else {
                std::cout << "Failed to create graph. Only " << points.size() << " points were read instead of " << n << "." << std::endl;
            }
            
        } else if (cmd == "CH") {
            std::cout << "Calculating convex hull..." << std::endl;
            ch.findConvexHull();
            ch.printConvexHull();
            
        } else if (cmd == "Newpoint") {
            float x, y;
            iss >> x >> y;
            Point newPoint(x, y);
            ch.addPoint(newPoint);
            ch.findConvexHull();
            std::cout << "Point (" << x << ", " << y << ") added successfully!" << std::endl;
            
        } else if (cmd == "Removepoint") {
            float x, y;
            iss >> x >> y;
            Point pointToRemove(x, y);
            ch.removePoint(pointToRemove);
            ch.findConvexHull();
            std::cout << "Point (" << x << ", " << y << ") removed successfully!" << std::endl;
            
        } else if (cmd == "Exit") {
            std::cout << "Exiting program..." << std::endl;
            break;
        } else {
            // Ignore unknown commands
            std::cout << "Unknown command. Available commands: Newgraph, CH, Newpoint, Removepoint" << std::endl;
        }
        
        std::cout << "Enter command: ";
    }
    
    return 0;
}
