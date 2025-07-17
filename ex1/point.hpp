#ifndef POINT_H
#define POINT_H

#include <iostream>
#include <cmath>

class Point {
private:
    float x;
    float y;

public:
    // Constructor
    Point(float x = 0.0f, float y = 0.0f);
    
    // Getters
    float getX() const;
    float getY() const;
    
    // Setters
    void setX(float newX);
    void setY(float newY);
    
    // Utility functions
    float distanceTo(const Point& other) const;
    void display() const;
    
    // Operator overloads
    bool operator==(const Point& other) const;
    bool operator!=(const Point& other) const;
};

#endif // POINT_H 