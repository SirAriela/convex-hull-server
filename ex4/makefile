CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-psabi -I../ex3
PROFILE_FLAGS = -pg -O2
SRC = convex_hull_server.cpp
OBJ = $(SRC:.cpp=.o)
EXE = convex_hull_server

all: $(EXE)

$(EXE): $(OBJ) ../ex3/convex_hull.o ../ex3/point.o
	$(CXX) $(CXXFLAGS) $(PROFILE_FLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

../ex3/convex_hull.o: ../ex3/convex_hull.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

../ex3/point.o: ../ex3/point.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXE) *.gcov *.gcda *.gcno ../ex3/convex_hull.o ../ex3/point.o
