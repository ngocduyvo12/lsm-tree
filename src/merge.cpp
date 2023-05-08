#include <cassert>
#include <iostream>

#include "merge.h"

// The add function adds a batch of entries (a run) to the MergeContext.
void MergeContext::add(entry_t *entries, long num_entries) {
    merge_entry_t merge_entry;

    if (num_entries > 0) { // Check if there are any entries to add
        merge_entry.entries = entries; // Set the entries pointer to the given entries
        merge_entry.num_entries = num_entries; // Set the number of entries
        merge_entry.precedence = queue.size(); // Set the precedence based on the current queue size
        queue.push(merge_entry); // Add the merge_entry to the priority queue
    }
}

// The next function returns the next entry to be merged based on the merge_entry_t structures in the priority queue.
entry_t MergeContext::next(void) {
    merge_entry_t current, next;
    entry_t entry;

    current = queue.top(); // Get the top element from the priority queue
    next = current;

    // While loop ensures that only the most recent value for a given key is returned
    while (next.head().key == current.head().key && !queue.empty()) {
        queue.pop(); // Remove the top element from the priority queue

        next.current_index++; // Increment the current_index of the next merge_entry_t
        if (!next.done()) { // Check if there are more entries in the next merge_entry_t
            queue.push(next); // Push the updated merge_entry_t back into the priority queue
        }

        next = queue.top(); // Update the next merge_entry_t with the new top element from the priority queue
    }

    return current.head(); // Return the most recent value for the given key
}

// The done function checks if all entries have been merged and returns true if the priority queue is empty.
bool MergeContext::done(void) {
    return queue.empty();
}
