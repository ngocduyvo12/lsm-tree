#include <queue>

#include "run.h"

// The Level class represents a level in the LSM tree, storing runs of key-value pairs
class Level {
public:
    int max_runs; // Maximum number of runs allowed in the level
    long max_run_size; // Maximum size of a run in the level
    std::deque<Run> runs; // A deque of runs in the level

    // Constructor for the Level class, initializing the maximum number of runs
    // and the maximum run size
    Level(int n, long s) : max_runs(n), max_run_size(s) {}

    // Returns the number of available spots for runs in the level
    bool remaining(void) const {return max_runs - runs.size();}
};
