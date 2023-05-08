#include <set>
#include <vector>

#include "types.h"

using namespace std;

// The Buffer class represents an in-memory buffer for the LSM tree,
// storing key-value pairs as they are inserted
class Buffer {
public:
    int max_size; // Maximum number of entries the buffer can hold
    set<entry_t> entries; // A sorted set of entries in the buffer

    // Constructor for the Buffer class, initializing its maximum size
    Buffer(int max_size) : max_size(max_size) {};

    // Searches the buffer for a key and returns a pointer to the value if found,
    // otherwise returns a nullptr
    VAL_t * get(KEY_t) const;

    // Searches the buffer for entries within a specified key range and returns
    // a pointer to a vector of entries within the range
    vector<entry_t> * range(KEY_t, KEY_t) const;

    // Inserts a key-value pair into the buffer, returning true if successful
    // or false if the buffer is full
    bool put(KEY_t, VAL_t val);

    // Empties the buffer, removing all entries
    void empty(void);
};
