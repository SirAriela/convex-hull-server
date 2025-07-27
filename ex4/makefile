CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-psabi -I../ex3
PROFILE_FLAGS = -pg -O2

all: convex_hull_server convex_hull_client

convex_hull_server: convex_hull_server.o ../ex3/convex_hull.o ../ex3/point.o
	$(CXX) $(CXXFLAGS) $(PROFILE_FLAGS) -o $@ $^

convex_hull_client: convex_hull_client.o
	$(CXX) $(CXXFLAGS) -o $@ $^

convex_hull_server.o: convex_hull_server.cpp
	$(CXX) $(CXXFLAGS) -c convex_hull_server.cpp

convex_hull_client.o: convex_hull_client.cpp
	$(CXX) $(CXXFLAGS) -c convex_hull_client.cpp

../ex3/convex_hull.o: ../ex3/convex_hull.cpp
	$(CXX) $(CXXFLAGS) -c ../ex3/convex_hull.cpp -o ../ex3/convex_hull.o

../ex3/point.o: ../ex3/point.cpp
	$(CXX) $(CXXFLAGS) -c ../ex3/point.cpp -o ../ex3/point.o

clean:
	rm -f *.o convex_hull_server convex_hull_client *.gcov *.gcda *.gcno ../ex3/convex_hull.o ../ex3/point.o

.PHONY: all clean
