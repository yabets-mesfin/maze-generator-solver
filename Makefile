CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = maze
SRC    = maze.cpp

.PHONY: all clean run run-animated run-bonus

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# Default 10x20 maze, no animation
run: $(TARGET)
	./$(TARGET) 10 20 0 0

# Animated step-by-step generation and solving
run-animated: $(TARGET)
	./$(TARGET) 10 20 1 1

# Bonus mode: cycles created by eating extra walls
run-bonus: $(TARGET)
	./$(TARGET) 10 20 0 1

clean:
	rm -f $(TARGET)
