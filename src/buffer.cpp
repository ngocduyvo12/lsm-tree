// Include required header files
#include <iostream>
#include <tuple>
#include "buffer.h"

// Use the standard namespace
using namespace std;

// Function to get a value from the buffer by key
VAL_t * Buffer::get(KEY_t key) const {
    // Declare necessary variables
    entry_t search_entry;
    set<entry_t>::iterator entry;
    VAL_t *val;

    // Set the search_entry key
    search_entry.key = key;

    // Find the entry with the given key
    entry = entries.find(search_entry);

    // If the entry is not found, return nullptr
    if (entry == entries.end()) {
        return nullptr;
    } else {
        // If the entry is found, allocate memory for val and return it
        val = new VAL_t;
        *val = entry->val;
        return val;
    }
}

// Function to get a range of entries within the specified key range
vector<entry_t> * Buffer::range(KEY_t start, KEY_t end) const {
    // Declare necessary variables
    entry_t search_entry;
    set<entry_t>::iterator subrange_start, subrange_end;

    // Set the search_entry key to the start of the range
    search_entry.key = start;
    subrange_start = entries.lower_bound(search_entry);

    // Set the search_entry key to the end of the range
    search_entry.key = end;
    subrange_end = entries.upper_bound(search_entry);

    // Return a new vector containing the entries within the range
    return new vector<entry_t>(subrange_start, subrange_end);
}

// Function to put an entry in the buffer
bool Buffer::put(KEY_t key, VAL_t val) {
    // Declare necessary variables
    entry_t entry;
    set<entry_t>::iterator it;
    bool found;

    // If the buffer is full, return false
    if (entries.size() == max_size) {
        return false;
    } else {
        // Set the entry's key and value
        entry.key = key;
        entry.val = val;

        // Attempt to insert the entry into the buffer
        tie(it, found) = entries.insert(entry);

        // If the entry already exists, update its value
        if (found == false) {
            entries.erase(it);
            entries.insert(entry);
        }

        // Return true, indicating successful insertion or update
        return true;
    }
}

// Function to clear the buffer
void Buffer::empty(void) {
    entries.clear();
}