#include <boost/dynamic_bitset.hpp>
#include <bitset>

#include "types.h"

// Define a class called BloomFilter
class BloomFilter {
   // Define a private member called table, which is a dynamic bitset
   boost::dynamic_bitset<> table;
   // Define three private hash functions
   uint64_t hash_1(KEY_t) const;
   uint64_t hash_2(KEY_t) const;
   uint64_t hash_3(KEY_t) const;
public:
    // Define a public constructor that takes a long integer argument
    // representing the size of the bitset
    BloomFilter(long length) : table(length) {}
    // Define a public method called set that takes a key and sets the
    // corresponding bits in the bitset using the three hash functions
    void set(KEY_t);
    // Define a public method called is_set that takes a key and returns true
    // if the corresponding bits in the bitset are set, false otherwise
    bool is_set(KEY_t) const;
};



