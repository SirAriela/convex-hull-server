CXX = g++
CXXFLAGS = -Wall -std=c++11 -pthread -g
EX3_DIR = ../ex3

# קבצי מקור
SERVER_SRC = convex_hull_reactor_server.cpp reactor.cpp $(EX3_DIR)/convex_hull.cpp $(EX3_DIR)/point.cpp
CLIENT_SRC = convex_hull_client_reactor.cpp

# קבצי יעד
SERVER_BIN = server
CLIENT_BIN = client

# קבצי אובייקט
SERVER_OBJ = convex_hull_reactor_server.o reactor.o $(EX3_DIR)/convex_hull.o $(EX3_DIR)/point.o
CLIENT_OBJ = convex_hull_client_reactor.o

# יעדים ראשיים
all: $(SERVER_BIN) $(CLIENT_BIN)

# בניית השרת
$(SERVER_BIN): $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# בניית הלקוח
$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# בניית קבצי האובייקט
convex_hull_reactor_server.o: convex_hull_reactor_server.cpp reactor.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

reactor.o: reactor.cpp reactor.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

convex_hull_client_reactor.o: convex_hull_client_reactor.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# כללי הידור לקבצי ex3
$(EX3_DIR)/convex_hull.o: $(EX3_DIR)/convex_hull.cpp $(EX3_DIR)/convex_hull.hpp $(EX3_DIR)/point.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EX3_DIR)/point.o: $(EX3_DIR)/point.cpp $(EX3_DIR)/point.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ניקוי
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) *.o $(EX3_DIR)/*.o

# דיבוג
debug: CXXFLAGS += -DDEBUG
debug: all

# יעדים שאינם קבצים
.PHONY: all clean debug server client help

# עזרה
help:
	@echo "Available targets:"
	@echo "  all     - Build server and client (default)"
	@echo "  server  - Build server only"
	@echo "  client  - Build client only"
	@echo "  clean   - Remove all build files"
	@echo "  debug   - Build with debug symbols"
	@echo "  help    - Show this help"