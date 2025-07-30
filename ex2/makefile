SHELL := /bin/bash
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pg
EX1_DIR = ../ex1
GRAHAM_EXEC = convex_hull
JARVIS_EXEC = convex_jarvis

.PHONY: all clean stage2

all: $(GRAHAM_EXEC) $(JARVIS_EXEC)

$(GRAHAM_EXEC): $(EX1_DIR)/main.cpp $(EX1_DIR)/convex_hull.cpp $(EX1_DIR)/point.cpp
	$(CXX) $(CXXFLAGS) -I$(EX1_DIR) -o $@ $^

$(JARVIS_EXEC): $(EX1_DIR)/mainj.cpp $(EX1_DIR)/convex_jarvis.cpp $(EX1_DIR)/point.cpp
	$(CXX) $(CXXFLAGS) -I$(EX1_DIR) -o $@ $^

stage2: all
	@echo "=== Profiling Comparison ==="
	@time ./$(GRAHAM_EXEC) < test.txt >/dev/null 2>&1
	@gprof ./$(GRAHAM_EXEC) gmon.out > graham_profile.txt
	@time ./$(JARVIS_EXEC) < test.txt >/dev/null 2>&1
	@gprof ./$(JARVIS_EXEC) gmon.out > jarvis_profile.txt
	@echo "Profiles saved: graham_profile.txt, jarvis_profile.txt"

clean:
	rm -f $(GRAHAM_EXEC) $(JARVIS_EXEC) gmon.out *_profile.txt
