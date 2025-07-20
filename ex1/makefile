CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# קבצי מקור
SRC = main.cpp convex_hull.cpp point.cpp
OBJ = $(SRC:.cpp=.o)
EXE = convex_hull_app

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o $(EXE)
