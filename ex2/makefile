SHELL := /bin/bash
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
PROFILE_FLAGS = -pg -O2
EX1_DIR = ../ex1
GRAHAM_EXEC = convex_hull
JARVIS_EXEC = convex_jarvis

.PHONY: all clean stage2 profile_graham profile_jarvis

all: $(GRAHAM_EXEC) $(JARVIS_EXEC)

$(GRAHAM_EXEC): $(EX1_DIR)/main.cpp $(EX1_DIR)/convex_hull.cpp $(EX1_DIR)/point.cpp
	$(CXX) $(CXXFLAGS) $(PROFILE_FLAGS) -I$(EX1_DIR) -o $@ $^

$(JARVIS_EXEC): $(EX1_DIR)/main.cpp $(EX1_DIR)/convex_jarvis.cpp $(EX1_DIR)/point.cpp
	$(CXX) $(CXXFLAGS) $(PROFILE_FLAGS) -I$(EX1_DIR) -o $@ $^

large_input.txt:
	@echo "1000000" > large_input.txt
	@python3 -c "import random; [print(random.randint(0,1000), random.randint(0,1000)) for _ in range(10000)]" >> large_input.txt

stage2: all large_input.txt
	@./$(GRAHAM_EXEC) < large_input.txt >/dev/null
	@time ./$(GRAHAM_EXEC) < large_input.txt >/dev/null
	@./$(JARVIS_EXEC) < large_input.txt >/dev/null
	@time ./$(JARVIS_EXEC) < large_input.txt >/dev/null

clean:
	rm -f $(GRAHAM_EXEC) $(JARVIS_EXEC) gmon.out large_input.txt