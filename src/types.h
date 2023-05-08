#ifndef TYPES_H
#define TYPES_H

// Define KEY_t and VAL_t as int32_t types to store keys and values in the LSM tree
typedef int32_t KEY_t;
typedef int32_t VAL_t;

// Define the maximum and minimum values for keys
#define KEY_MAX 2147483647
#define KEY_MIN -2147483648

// Define the maximum, minimum, and tombstone values for values (VAL_t)
#define VAL_MAX 2147483647
#define VAL_MIN -2147483647
#define VAL_TOMBSTONE -2147483648

// Define a struct called 'entry' to represent key-value pairs in the LSM tree
struct entry {
    KEY_t key; // Key of the entry
    VAL_t val; // Value of the entry

    // Define comparison operators for entry structs based on their keys
    bool operator==(const entry& other) const {return key == other.key;}
    bool operator<(const entry& other) const {return key < other.key;}
    bool operator>(const entry& other) const {return key > other.key;}
};

// Define entry_t as an alias for the 'entry' struct
typedef struct entry entry_t;

#endif
