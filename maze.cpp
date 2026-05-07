/**
 * maze.cpp
 * --------
 * Perfect Maze Generator (DFS stack-based backtracking) + Backtracking Solver
 *
 * Representation:
 *   northWall[R][C]  -- 1 = north wall intact, 0 = wall removed (passage exists)
 *   eastWall[R][C]   -- 1 = east wall intact,  0 = wall removed (passage exists)
 *
 * Special rows/columns:
 *   Row 0 is a phantom row: northWall[0][j] forms the BOTTOM edge of the maze.
 *   eastWall[i][0]  forms the LEFT  edge of the maze.
 *
 * Actual maze cells occupy rows 1..R and columns 1..C (1-indexed internally),
 * but we expose them as 0..R-1, 0..C-1 to the user for clarity.
 *
 * Compile:  g++ -std=c++17 -o maze maze.cpp
 * Run:      ./maze [rows] [cols] [step-by-step: 0|1]
 */

#include <iostream>
#include <vector>
#include <stack>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>

// ─────────────────────────────────────────────
//  Configuration constants
// ─────────────────────────────────────────────
static const int DEFAULT_ROWS = 10;
static const int DEFAULT_COLS = 20;

// ANSI colour codes for terminal output
static const std::string RESET   = "\033[0m";
static const std::string RED     = "\033[31m";
static const std::string BLUE    = "\033[34m";
static const std::string GREEN   = "\033[32m";
static const std::string YELLOW  = "\033[33m";
static const std::string CYAN    = "\033[36m";
static const std::string BOLD    = "\033[1m";

// ─────────────────────────────────────────────
//  Data structures
// ─────────────────────────────────────────────
struct Cell {
    int row, col;
};

struct Maze {
    int R, C;                               // number of rows and columns (user-facing)

    // Wall arrays sized [R+1][C+1] to accommodate the phantom row/col
    // northWall[r][c] == 1 => wall above cell (r,c) is intact
    // eastWall [r][c] == 1 => wall right of cell (r,c) is intact
    std::vector<std::vector<char>> northWall;
    std::vector<std::vector<char>> eastWall;

    // visited[r][c] tracks which cells the DFS mouse has already eaten through
    std::vector<std::vector<bool>> visited;

    Maze(int rows, int cols)
        : R(rows), C(cols),
          northWall(rows + 1, std::vector<char>(cols + 1, 1)),
          eastWall (rows + 1, std::vector<char>(cols + 1, 1)),
          visited  (rows,     std::vector<bool>(cols, false))
    {}
};

// ─────────────────────────────────────────────
//  Helper: clear terminal screen
// ─────────────────────────────────────────────
void clearScreen() {
    std::cout << "\033[2J\033[H";
}

// ─────────────────────────────────────────────
//  Maze display
//
//  We draw the maze top-down.  For each cell we print:
//    - The north wall (top edge of cell)
//    - The east wall  (right edge of cell)
//  Plus a border for the outer walls.
//
//  Cell contents (for solver visualisation):
//    '.' = solution path  (red)
//    'x' = dead end visited  (blue)
//    ' ' = unvisited
//    'S' = start  (green)
//    'E' = end    (green)
// ─────────────────────────────────────────────
void drawMaze(const Maze& m,
              const std::vector<std::vector<char>>& cellState,
              bool showGeneration = false)
{
    // Top border
    std::cout << BOLD << "+";
    for (int c = 0; c < m.C; ++c) {
        std::cout << "---+";
    }
    std::cout << RESET << "\n";

    // Print rows from top (R-1) down to 0
    for (int r = m.R - 1; r >= 0; --r) {
        // --- Cell row ---
        // Left outer wall
        std::cout << BOLD << "|" << RESET;

        for (int c = 0; c < m.C; ++c) {
            // Cell content
            char ch = cellState[r][c];
            if (ch == 'S') {
                std::cout << GREEN << BOLD << " S " << RESET;
            } else if (ch == 'E') {
                std::cout << GREEN << BOLD << " E " << RESET;
            } else if (ch == '.') {
                std::cout << RED << " . " << RESET;
            } else if (ch == 'x') {
                std::cout << BLUE << " x " << RESET;
            } else if (ch == '*') {
                std::cout << YELLOW << " * " << RESET;   // generation frontier
            } else {
                std::cout << "   ";
            }

            // East wall of this cell
            // eastWall[r+1][c+1] in 1-indexed storage, but we stored 0-indexed after
            // adjusting: see removeWall().  Here we use r,c directly as stored.
            if (c == m.C - 1) {
                // Rightmost column: always a wall (outer boundary)
                std::cout << BOLD << "|" << RESET;
            } else {
                // eastWall[r][c]: 1 = wall present
                std::cout << (m.eastWall[r][c] ? BOLD + "|" + RESET : " ");
            }
        }
        std::cout << "\n";

        // --- Horizontal wall row (south walls of row r = north walls of row r-1) ---
        // For the bottom edge of the maze (r == 0), northWall[0][c] forms the border.
        std::cout << BOLD << "+";
        for (int c = 0; c < m.C; ++c) {
            // northWall[r][c] == 1 => south wall of cell (r,c) exists
            std::cout << (m.northWall[r][c] ? "---" : "   ") << "+";
        }
        std::cout << RESET << "\n";
    }
}

// ─────────────────────────────────────────────
//  Wall removal helpers
//
//  When the mouse moves from cell (r,c) to neighbour (nr,nc),
//  we erase the shared wall in both northWall and eastWall arrays.
// ─────────────────────────────────────────────
void removeWallBetween(Maze& m, int r, int c, int nr, int nc) {
    if (nr == r + 1) {
        // Moving NORTH: remove north wall of (r,c) == south wall of (nr,nc)
        m.northWall[r + 1][c] = 0;   // phantom-row trick: row r+1 is the "ceiling"
        // Actually in our 0-indexed scheme northWall[r][c] is the SOUTH wall of (r,c)
        // and the shared wall between (r,c) and (r+1,c) is northWall[r+1][c].
        // Let's clarify: northWall[r][c] = south wall of cell at row r.
        // Moving up: shared wall = northWall[r+1][c]
        m.northWall[r + 1][c] = 0;
    } else if (nr == r - 1) {
        // Moving SOUTH: shared wall = northWall[r][c]
        m.northWall[r][c] = 0;
    } else if (nc == c + 1) {
        // Moving EAST: shared wall = eastWall[r][c]
        m.eastWall[r][c] = 0;
    } else if (nc == c - 1) {
        // Moving WEST: shared wall = eastWall[r][nc] = eastWall[r][c-1]
        m.eastWall[r][c - 1] = 0;
    }
}

// ─────────────────────────────────────────────
//  Maze passage query
//
//  canMove(m, r, c, nr, nc) returns true if there is NO wall between
//  cell (r,c) and its neighbour (nr,nc).
// ─────────────────────────────────────────────
bool canMove(const Maze& m, int r, int c, int nr, int nc) {
    if (nr < 0 || nr >= m.R || nc < 0 || nc >= m.C) return false;

    if (nr == r + 1) return m.northWall[r + 1][c] == 0;  // north passage
    if (nr == r - 1) return m.northWall[r][c]     == 0;  // south passage
    if (nc == c + 1) return m.eastWall[r][c]      == 0;  // east passage
    if (nc == c - 1) return m.eastWall[r][c - 1]  == 0;  // west passage
    return false;
}

// ─────────────────────────────────────────────
//  MAZE GENERATOR  — DFS Stack-Based Backtracking
//
//  Algorithm:
//   1. Place the "mouse" in a random starting cell; mark it visited.
//   2. Push the starting cell onto the stack.
//   3. While the stack is not empty:
//      a. Look at the current cell's unvisited neighbours.
//      b. If there are unvisited neighbours:
//           - Choose one randomly.
//           - Eat through (remove) the shared wall.
//           - Mark the chosen neighbour visited.
//           - Push current cell and move to the neighbour.
//      c. If no unvisited neighbours: backtrack by popping the stack.
//   4. When the stack empties, every cell has been visited exactly once
//      → the maze is a perfect (spanning-tree) maze.
// ─────────────────────────────────────────────
void generateMaze(Maze& m, bool stepByStep = false) {
    // Direction vectors: N, S, E, W
    const int DR[] = { 1, -1,  0,  0 };
    const int DC[] = { 0,  0,  1, -1 };

    // Start the mouse at a random cell
    int startR = rand() % m.R;
    int startC = rand() % m.C;

    m.visited[startR][startC] = true;

    std::stack<Cell> dfsStack;
    dfsStack.push({startR, startC});

    std::vector<std::vector<char>> cellState(m.R, std::vector<char>(m.C, ' '));

    int stepCount = 0;

    while (!dfsStack.empty()) {
        Cell cur = dfsStack.top();
        int r = cur.row, c = cur.col;

        // Collect unvisited neighbours
        std::vector<int> dirs = {0, 1, 2, 3};
        std::shuffle(dirs.begin(), dirs.end(), std::mt19937(rand()));

        bool moved = false;
        for (int d : dirs) {
            int nr = r + DR[d];
            int nc = c + DC[d];

            if (nr >= 0 && nr < m.R && nc >= 0 && nc < m.C && !m.visited[nr][nc]) {
                // Eat through the wall
                removeWallBetween(m, r, c, nr, nc);
                m.visited[nr][nc] = true;
                dfsStack.push({nr, nc});
                moved = true;

                if (stepByStep) {
                    // Mark current cell for display
                    cellState[r][c]   = '*';
                    cellState[nr][nc] = '*';
                    clearScreen();
                    std::cout << BOLD << CYAN
                              << "  Generating maze... (step " << ++stepCount << ")\n"
                              << RESET;
                    drawMaze(m, cellState);
                    std::this_thread::sleep_for(std::chrono::milliseconds(60));
                    cellState[r][c]   = ' ';
                    cellState[nr][nc] = ' ';
                }
                break;
            }
        }

        if (!moved) {
            // Dead end during generation — backtrack
            dfsStack.pop();
        }
    }
}

// ─────────────────────────────────────────────
//  MAZE SOLVER  — Backtracking (Stack-Based DFS)
//
//  Algorithm:
//   1. Place the mouse at (0,0) (top-left = bottom-left in display).
//      Actually we use bottom-left as start, top-right as end.
//   2. Push start cell, mark it as "on path".
//   3. While stack is not empty and end not reached:
//      a. Try each direction in random order.
//      b. If the passage is open AND the neighbour is unvisited:
//           - Move there, push to stack, mark as path ('.').
//      c. If no valid move: backtrack (pop), mark current as dead-end ('x').
//   4. When end cell is reached, the stack contains the solution path.
// ─────────────────────────────────────────────
bool solveMaze(const Maze& m,
               std::vector<std::vector<char>>& cellState,
               bool stepByStep = false)
{
    const int DR[] = { 1, -1,  0,  0 };
    const int DC[] = { 0,  0,  1, -1 };

    // visited array for the solver (separate from generator's visited)
    std::vector<std::vector<bool>> solverVisited(m.R, std::vector<bool>(m.C, false));

    int endR = m.R - 1, endC = m.C - 1;

    std::stack<Cell> path;
    path.push({0, 0});
    solverVisited[0][0] = true;
    cellState[0][0] = '.';

    int stepCount = 0;

    while (!path.empty()) {
        Cell cur = path.top();
        int r = cur.row, c = cur.col;

        // Reached the end?
        if (r == endR && c == endC) {
            cellState[r][c] = '.';
            return true;
        }

        // Try neighbours
        bool moved = false;
        // Try all 4 directions (we can shuffle for variety, but a fixed order is fine)
        const int order[] = {0, 2, 1, 3};  // N, E, S, W — bias toward goal
        for (int i = 0; i < 4; ++i) {
            int d  = order[i];
            int nr = r + DR[d];
            int nc = c + DC[d];

            if (nr >= 0 && nr < m.R && nc >= 0 && nc < m.C
                && !solverVisited[nr][nc]
                && canMove(m, r, c, nr, nc))
            {
                solverVisited[nr][nc] = true;
                cellState[nr][nc] = '.';
                path.push({nr, nc});
                moved = true;

                if (stepByStep) {
                    clearScreen();
                    std::cout << BOLD << CYAN
                              << "  Solving maze... (step " << ++stepCount << ")\n"
                              << RESET;
                    drawMaze(m, cellState);
                    std::this_thread::sleep_for(std::chrono::milliseconds(80));
                }
                break;
            }
        }

        if (!moved) {
            // Backtrack: mark as dead end
            cellState[r][c] = 'x';
            path.pop();

            if (stepByStep) {
                clearScreen();
                std::cout << BOLD << CYAN
                          << "  Backtracking... (step " << ++stepCount << ")\n"
                          << RESET;
                drawMaze(m, cellState);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }

    return false;  // no path found (should never happen in a perfect maze)
}

// ─────────────────────────────────────────────
//  BONUS: Extra-wall eating (creates cycles)
//  Randomly removes 1-in-20 additional walls to
//  defeat the "shoulder-to-the-wall" rule.
// ─────────────────────────────────────────────
void eatExtraWalls(Maze& m, int probability = 20) {
    for (int r = 0; r < m.R; ++r) {
        for (int c = 0; c < m.C; ++c) {
            // Randomly eat north wall (not the bottom boundary)
            if (r + 1 < m.R && m.northWall[r + 1][c] == 1) {
                if (rand() % probability == 0) {
                    m.northWall[r + 1][c] = 0;
                }
            }
            // Randomly eat east wall (not the right boundary)
            if (c + 1 < m.C && m.eastWall[r][c] == 1) {
                if (rand() % probability == 0) {
                    m.eastWall[r][c] = 0;
                }
            }
        }
    }
}

// ─────────────────────────────────────────────
//  Entry point
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));

    int rows      = DEFAULT_ROWS;
    int cols      = DEFAULT_COLS;
    bool animated = false;
    bool bonus    = false;

    if (argc >= 2) rows      = std::atoi(argv[1]);
    if (argc >= 3) cols      = std::atoi(argv[2]);
    if (argc >= 4) animated  = std::atoi(argv[3]) != 0;
    if (argc >= 5) bonus     = std::atoi(argv[4]) != 0;

    if (rows < 2 || cols < 2) {
        std::cerr << "Maze must be at least 2x2.\n";
        return 1;
    }

    // ── 1. Create and generate the maze ──────────────────────
    Maze maze(rows, cols);
    generateMaze(maze, animated);

    // ── BONUS: eat extra walls to create cycles ───────────────
    if (bonus) {
        eatExtraWalls(maze, 20);
    }

    // ── 2. Display the generated maze ────────────────────────
    std::vector<std::vector<char>> cellState(rows, std::vector<char>(cols, ' '));
    cellState[0][0]             = 'S';   // start = bottom-left
    cellState[rows-1][cols-1]   = 'E';   // end   = top-right

    clearScreen();
    std::cout << BOLD << GREEN
              << "\n  ===  MAZE GENERATED  ===\n"
              << "  Start: bottom-left (S)   End: top-right (E)\n\n"
              << RESET;
    if (bonus) {
        std::cout << YELLOW << BOLD
                  << "  [BONUS] Extra walls eaten — cycles introduced!\n\n"
                  << RESET;
    }
    drawMaze(maze, cellState);

    std::cout << "\n  Press ENTER to start solver...";
    std::cin.get();

    // ── 3. Solve the maze ─────────────────────────────────────
    cellState[0][0]             = 'S';
    cellState[rows-1][cols-1]   = 'E';

    bool solved = solveMaze(maze, cellState, animated);

    // ── 4. Display the solved maze ────────────────────────────
    // Restore S/E markers on top of path
    cellState[0][0]           = 'S';
    cellState[rows-1][cols-1] = 'E';

    clearScreen();
    if (solved) {
        std::cout << BOLD << GREEN
                  << "\n  ===  MAZE SOLVED!  ===\n"
                  << "  Red (.) = solution path\n"
                  << "  Blue (x) = explored dead ends\n\n"
                  << RESET;
    } else {
        std::cout << BOLD << RED
                  << "\n  ===  NO SOLUTION FOUND  ===\n\n"
                  << RESET;
    }
    drawMaze(maze, cellState);
    std::cout << "\n";

    return 0;
}
