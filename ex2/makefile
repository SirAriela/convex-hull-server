SHELL := /bin/bash
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
PROFILE_FLAGS = -pg -O2
EX1_DIR = ../ex1
GRAHAM_EXEC = convex_hull
JARVIS_EXEC = convex_jarvis
SIZE = 100000

.PHONY: all clean stage2

all: $(GRAHAM_EXEC) $(JARVIS_EXEC)

$(GRAHAM_EXEC): $(EX1_DIR)/main.cpp $(EX1_DIR)/convex_hull.cpp $(EX1_DIR)/point.cpp
	$(CXX) $(CXXFLAGS) $(PROFILE_FLAGS) -I$(EX1_DIR) -o $@ $^

$(JARVIS_EXEC): $(EX1_DIR)/mainj.cpp $(EX1_DIR)/convex_jarvis.cpp $(EX1_DIR)/point.cpp
	$(CXX) $(CXXFLAGS) $(PROFILE_FLAGS) -I$(EX1_DIR) -o $@ $^

stage2: all
	@echo "=== Performance Comparison ($(SIZE) points) ==="
	@echo "Graham Scan:"
	@time bash -c "python3 -c \"print('$(SIZE)'); import random; [print(random.randint(0,1000), random.randint(0,1000)) for _ in range($(SIZE))]\" | ./$(GRAHAM_EXEC) >/dev/null"
	@echo ""
	@echo "Jarvis March:"
	@time bash -c "python3 -c \"print('$(SIZE)'); import random; [print(random.randint(0,1000), random.randint(0,1000)) for _ in range($(SIZE))]\" | ./$(JARVIS_EXEC) >/dev/null"
	@rm -f $(GRAHAM_EXEC) $(JARVIS_EXEC) gmon.out
	@echo "Files cleaned up."

clean:
	rm -f $(GRAHAM_EXEC) $(JARVIS_EXEC) gmon.out