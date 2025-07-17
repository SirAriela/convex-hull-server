#include "point.hpp"

// Constructor
Point::Point(float x, float y) : x(x), y(y) {}

// Getters
float Point::getX() const { 
    return x; 
}

float Point::getY() const { 
    return y; 
}

// Setters
void Point::setX(float newX) { 
    x = newX; 
}

void Point::setY(float newY) { 
    y = newY; 
}

// Calculate distance to another point
float Point::distanceTo(const Point& other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Display point
void Point::display() const {
    std::cout << "(" << x << ", " << y << ")";
}

// Operator overloads
bool Point::operator==(const Point& other) const {
    return x == other.x && y == other.y;
}

bool Point::operator!=(const Point& other) const {
    return !(*this == other);
} 