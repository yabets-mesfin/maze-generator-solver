# Maze Generator & Solver

A C++ terminal application that generates **perfect mazes** using **DFS stack-based backtracking** and solves them using a **backtracking solver**.

---

## How It Works

### Maze Representation

Each cell's walls are stored in two 2D arrays:

```cpp
char northWall[R][C];   // 1 = north wall intact, 0 = passage open
char eastWall[R][C];    // 1 = east wall intact,  0 = passage open
```

- `northWall[r][c]` describes the **south** wall of cell `(r,c)` (i.e. the wall between row `r` and row `r-1`).
- Row `0` is a **phantom row** — its `northWall` entries form the **bottom border** of the maze.
- `eastWall[r][0]` entries form the **left border** of the maze.

This means the program never needs a separate "southWall" or "westWall" array — they are implied by the adjacent cell's north/east wall.

---

### Maze Generation — DFS Stack-Based Backtracking

```
Start with ALL walls intact (a plain grid).
Place the "mouse" at a random cell and mark it visited.
Push it onto a stack.

WHILE the stack is not empty:
    Current = top of stack
    Find all unvisited neighbours of Current

    IF unvisited neighbours exist:
        Pick one at random
        REMOVE the shared wall  ← "eating" through the wall
        Mark the neighbour visited
        Push it onto the stack (move there)
    ELSE:
        POP the stack  ← backtrack to previous cell

WHEN stack is empty → every cell has been visited exactly once.
```

**Why is the result a "perfect" maze?**

Because the DFS visits every cell exactly once and removes exactly one wall per visit, the result is a **spanning tree** of the grid graph — meaning:
- Every cell is reachable (no isolated cells).
- There is exactly **one** unique path between any two cells.
- There are **no loops**.

---

### Maze Solver — Backtracking (Stack-Based DFS)

```
Place the mouse at (0,0) — bottom-left (START).
Push it onto the path stack, mark as "on path" (red dot).

WHILE stack is not empty AND end not reached:
    Current = top of stack

    IF current == (R-1, C-1):
        SOLVED! The stack contains the full path.

    Try each of 4 directions (N, E, S, W):
        IF passage is open AND neighbour is unvisited:
            Move there, mark as "." (red), push to stack
            BREAK

    IF no valid move found:
        Mark current as "x" (blue dead end)
        POP the stack  ← backtrack
```

The solver is guaranteed to find the path in a perfect maze because there is exactly one path between any two cells.

**Visual legend:**
| Symbol | Meaning |
|--------|---------|
| `S` | Start cell (0,0) — bottom-left |
| `E` | End cell (R-1, C-1) — top-right |
| `.` | Solution path (red) |
| `x` | Explored dead end (blue) |
| `*` | Active frontier during generation (yellow) |

---

## How to Compile and Run

### Requirements
- `g++` with C++17 support
- A terminal with ANSI colour support (standard on Ubuntu / VS Code terminal)

### Compile

```bash
g++ -std=c++17 -o maze maze.cpp
```

Or using the Makefile:

```bash
make
```

### Run

```bash
./maze [rows] [cols] [animated: 0|1] [bonus: 0|1]
```

| Argument | Default | Description |
|----------|---------|-------------|
| `rows`   | 10      | Number of maze rows |
| `cols`   | 20      | Number of maze columns |
| `animated` | 0   | 1 = show step-by-step animation |
| `bonus`  | 0       | 1 = eat extra walls to create cycles |

### Examples

```bash
# Default 10x20 maze
./maze

# Custom 15x30 maze
./maze 15 30

# Animated step-by-step (watch the mouse eat walls!)
./maze 10 20 1

# Bonus: maze with cycles (defeats shoulder-to-the-wall rule)
./maze 10 20 0 1
```

Using the Makefile shortcuts:

```bash
make run           # Default run
make run-animated  # Step-by-step animation
make run-bonus     # Cycle maze
```

---

## Bonus Features

### Step-by-Step Animation (`animated = 1`)

Watch the "mouse" eat through walls in real time during generation (yellow `*` shows the frontier). During solving, red dots advance and blue `x` marks backtrack in real time.

### Extra Wall Eating — Cycles (`bonus = 1`)

After generation, roughly 1-in-20 remaining walls are randomly removed. This introduces **cycles** into the maze, which defeats the classic **"shoulder-to-the-wall"** traversal rule described in the assignment addendum. The backtracking solver still finds a path, but there may now be multiple valid paths.

---

## Optional Improvements for Higher Marks

1. **Prim's or Kruskal's algorithm** — alternative perfect-maze generators that produce mazes with different "texture" (Prim's gives shorter dead ends; Kruskal's is more uniform).

2. **Breadth-First Search (BFS) solver** — guarantees the **shortest** path, unlike the DFS backtracking solver which finds *a* path.

3. **A\* solver** — uses a heuristic (Manhattan distance) to find the shortest path faster than BFS.

4. **Configurable start/end positions** — allow the user to specify start and end as command-line arguments, including interior positions.

5. **Save maze to file** — write the wall arrays and solution path to a `.txt` file for submission evidence.

6. **Coloured path animation** — use different ANSI colours for each "depth level" of the DFS to show how deep the recursion went.

---

## File Structure

```
maze/
├── maze.cpp      ← Full source: generator + solver + display
├── Makefile      ← Build shortcuts
└── README.md     ← This file
```
