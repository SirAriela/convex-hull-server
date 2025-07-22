CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -fprofile-arcs -ftest-coverage

# קבצי מקור
SRC = main.cpp convex_hull.cpp convex_jarvis.cpp point.cpp
OBJ = $(SRC:.cpp=.o)
EXE = convex_hull_app

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

coverage: convex_hull_app
	./convex_hull_app < test1.txt
	./convex_hull_app < test2.txt || true
	@echo "=== Coverage Summary ==="
	@gcov main.cpp 2>&1 | grep -A1 "File 'main\.cpp'"
	@gcov convex_hull.cpp 2>&1 | grep -A1 "File 'convex_hull\.cpp'"

clean:
	rm -f *.o $(EXE) *.gcov *.gcda *.gcno
