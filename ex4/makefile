CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

all: convex_hull_server

convex_hull_server: convex_hull_server.cpp ../ex3/convex_hull.cpp ../ex3/point.cpp
	$(CXX) $(CXXFLAGS) -I../ex3 -o $@ $^

clean:
	rm -f convex_hull_server
