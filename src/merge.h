#include <cassert>
#include <queue>

#include "types.h"

using namespace std;

// Define the merge_entry structure which holds information about a run of entries
struct merge_entry {
    int precedence; // Precedence value to break ties when keys are equal
    entry_t *entries; // Pointer to the array of entries in the run
    long num_entries; // The number of entries in the run
    int current_index = 0; // The current index of the entry being processed in the run

    // Return the entry at the current_index
    entry_t head(void) const {return entries[current_index];}

    // Return true if the current_index is equal to the number of entries in the run, indicating that the run has been processed
    bool done(void) const {return current_index == num_entries;}

    // Overload the '>' operator to compare merge_entry objects
    bool operator>(const merge_entry& other) const {
        // Order first by keys, then by precedence
        if (head() == other.head()) { // If the keys are equal, compare based on precedence
            assert(precedence != other.precedence);
            return precedence > other.precedence;
        } else { // Otherwise, compare based on the keys
            return head() > other.head();
        }
    }
};

// Typedef for easier readability
typedef struct merge_entry merge_entry_t;

// Define the MergeContext class which handles merging of runs
class MergeContext {
    // Declare a priority queue to store merge_entry_t objects, sorted by keys and precedence
    priority_queue<merge_entry_t, vector<merge_entry_t>, greater<merge_entry_t>> queue;
public:
    // Add a run of entries to the MergeContext
    void add(entry_t *, long);

    // Get the next entry to be merged
    entry_t next(void);

    // Check if all runs have been processed
    bool done(void);
};
