
#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <iostream>
#include <vector>
#include "point.hpp"

class Graph {
private:
    std::vector<Point> graph;

public:
    Graph(std::vector<Point> graph);
    
};

#endif