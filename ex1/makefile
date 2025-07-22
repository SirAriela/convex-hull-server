CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# קבצי מקור
SRC = main.cpp mainj.cpp convex_hull.cpp convex_jarvis.cpp point.cpp
OBJ = $(SRC:.cpp=.o)
EXE1 = convex_hull_app
EXE2 = convex_jarvis_app

all: $(EXE1) $(EXE2)

$(EXE1): main.o convex_hull.o point.o
    $(CXX) $(CXXFLAGS) -o $@ $^

$(EXE2): mainj.o convex_jarvis.o point.o
    $(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
    $(CXX) $(CXXFLAGS) -c $<

clean:
    rm -f *.o $(EXE1) $(EXE2)