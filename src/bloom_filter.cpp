#include "bloom_filter.h"

// Hash functions taken from https://gist.github.com/badboy/6267743
// and modified for the C++ environment.

// First hash function
uint64_t BloomFilter::hash_1(KEY_t k) const {
    uint64_t key;

    // Initialize key with input k
    key = k;

    // Bit manipulation operations
    key = ~key + (key<<15);
    key = key ^ (key>>12);
    key = key + (key<<2);
    key = key ^ (key>>4);
    key = key * 2057;
    key = key ^ (key>>16);

    // Modulo operation to get an index within the table size
    return key % table.size();
}

// Second hash function
uint64_t BloomFilter::hash_2(KEY_t k) const {
    uint64_t key;

    // Initialize key with input k
    key = k;

    // Bit manipulation operations
    key = (key+0x7ed55d16) + (key<<12);
    key = (key^0xc761c23c) ^ (key>>19);
    key = (key+0x165667b1) + (key<<5);
    key = (key+0xd3a2646c) ^ (key<<9);
    key = (key+0xfd7046c5) + (key<<3);
    key = (key^0xb55a4f09) ^ (key>>16);

    // Modulo operation to get an index within the table size
    return key % table.size();
}

// Third hash function
uint64_t BloomFilter::hash_3(KEY_t k) const {
    uint64_t key;

    // Initialize key with input k
    key = k;

    // Bit manipulation operations
    key = (key^61) ^ (key>>16);
    key = key + (key<<3);
    key = key ^ (key>>4);
    key = key * 0x27d4eb2d;
    key = key ^ (key>>15);

    // Modulo operation to get an index within the table size
    return key % table.size();
}

// Set a bit in the filter for the given key
void BloomFilter::set(KEY_t key) {
    // Set bits at the indices returned by the three hash functions
    table.set(hash_1(key));
    table.set(hash_2(key));
    table.set(hash_3(key));
}

// Check if a bit is set in the filter for the given key
bool BloomFilter::is_set(KEY_t key) const {
    // Check if bits are set at the indices returned by the three hash functions
    return (table.test(hash_1(key))
         && table.test(hash_2(key))
         && table.test(hash_3(key)));
}