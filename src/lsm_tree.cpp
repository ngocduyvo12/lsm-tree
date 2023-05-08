#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <chrono>

#include "lsm_tree.h"
#include "merge.h"
#include "sys.h"

using namespace std;

// Overloading the output stream operator to write an entry_t object to the stream
ostream& operator<<(ostream& stream, const entry_t& entry) {
    stream.write((char *)&entry.key, sizeof(KEY_t));
    stream.write((char *)&entry.val, sizeof(VAL_t));
    return stream;
}

// Overloading the input stream operator to read an entry_t object from the stream
istream& operator>>(istream& stream, entry_t& entry) {
    stream.read((char *)&entry.key, sizeof(KEY_t));
    stream.read((char *)&entry.val, sizeof(VAL_t));
    return stream;
}

/*
 * LSM Tree
 */

// LSMTree constructor, initializes the LSM tree parameters
LSMTree::LSMTree(int buffer_max_entries, int depth, int fanout,
                 int num_threads, float bf_bits_per_entry) :
                 bf_bits_per_entry(bf_bits_per_entry),
                 buffer(buffer_max_entries),
                 worker_pool(num_threads)
{
    long max_run_size;

    max_run_size = buffer_max_entries;

    // Create levels for the LSM tree with their corresponding sizes
    while ((depth--) > 0) {
        levels.emplace_back(fanout, max_run_size);
        max_run_size *= fanout;
    }
}

// This function merges the runs in the current level down to the next level of the LSM tree
// to create space for new entries. It follows the size-tiered compaction strategy.
void LSMTree::merge_down(vector<Level>::iterator current) {
    vector<Level>::iterator next;
    MergeContext merge_ctx;
    entry_t entry;

    // Check if the iterator is within the bounds of the levels vector
    assert(current >= levels.begin());

    // If there is remaining space in the current level, no need to merge
    if (current->remaining() > 0) {
        return;
    }
    // If the current level is the last one, there is no more space to merge down
    else if (current >= levels.end() - 1) {
        die("No more space in tree.");
    }
    // Otherwise, set the next level as the target for merging
    else {
        next = current + 1;
    }

    /*
     * If the next level does not have space for the current level,
     * recursively merge the next level downwards to create some
     */
    if (next->remaining() == 0) {
        merge_down(next);
        assert(next->remaining() > 0);
    }

    /*
     * Merge all runs in the current level into the first
     * run in the next level
     */
    for (auto& run : current->runs) {
        merge_ctx.add(run.map_read(), run.size);
    }

    // Create a new run in the next level to store the merged entries
    next->runs.emplace_front(next->max_run_size, bf_bits_per_entry);
    next->runs.front().map_write();

    // Iterate through the merged entries and insert them into the new run
    while (!merge_ctx.done()) {
        entry = merge_ctx.next();

        // If we're not in the final level and the entry is not a tombstone,
        // insert it into the next level's run
        if (!(next == levels.end() - 1 && entry.val == VAL_TOMBSTONE)) {
            next->runs.front().put(entry);
        }
    }

    // Unmap the newly created run in the next level
    next->runs.front().unmap();

    // Unmap the runs in the current level
    for (auto& run : current->runs) {
        run.unmap();
    }

    /*
     * Clear the current level to delete the old (now
     * redundant) entry files.
     */
    current->runs.clear();
}

// The put function inserts a key-value pair into the LSM tree.
void LSMTree::put(KEY_t key, VAL_t val) {
    /*
     * Insert the key into the buffer and check if the buffer is full
     */
    bool bufferFull = !buffer.put(key, val);

    // If the buffer is not full, return
    if (!bufferFull) {
        return;
    }

    // Merge down the runs in the first level if it's full
    merge_down(levels.begin());

    // Create a new run in the first level to store the buffer's entries
    levels.front().runs.emplace_front(levels.front().max_run_size, bf_bits_per_entry);
    levels.front().runs.front().map_write();

    // Iterate through the buffer's entries and insert them into the new run
    for (const auto& entry : buffer.entries) {
        levels.front().runs.front().put(entry);
    }

    // Unmap the newly created run in the first level
    levels.front().runs.front().unmap();

    /*
     * Empty the buffer and insert the key/value pair
     */

    // Empty the buffer
    buffer.empty();
    
    // Insert the key-value pair into the now-empty buffer
    assert(buffer.put(key, val));
}

// The get_run function retrieves the run at the specified index in the LSM tree.
Run * LSMTree::get_run(int index) {
    // Iterate through the levels of the LSM tree
    for (const auto& level : levels) {
        // If the index is within the current level's runs, return the run at that index
        if (index < level.runs.size()) {
            return (Run *) &level.runs[index];
        } else {
            // Otherwise, decrement the index by the current level's run size
            // to search for the run in the subsequent levels
            index -= level.runs.size();
        }
    }

    // If the function reaches this point, no run was found at the specified index
    // Return a nullptr to indicate that the run was not found
    return nullptr;
};

/*
 * LSMTree::get function retrieves the value associated with a given key.
 *
 * Parameters:
 * - KEY_t key: the key for which the associated value should be retrieved.
 *
 * Steps:
 * 1. Search the buffer for the key and return its value if found.
 * 2. If the key is not in the buffer, search for the key in the runs using multiple threads.
 * 3. If the key is found in a run, output the associated value.
 * 4. If the key is not found, output an empty line.
 */
void LSMTree::get(KEY_t key) {
    VAL_t *buffer_val;
    VAL_t latest_val;
    int latest_run;
    SpinLock lock;
    atomic<int> counter;

    // Step 1: Search buffer
    buffer_val = buffer.get(key);

    if (buffer_val != nullptr) {
        if (*buffer_val != VAL_TOMBSTONE) cout << *buffer_val;
        cout << endl;
        delete buffer_val;
        return;
    }

    // Step 2: Search runs using multiple threads
    counter = 0;
    latest_run = -1;

    worker_task search = [&] {
        int current_run;
        Run *run;
        VAL_t *current_val;

        current_run = counter++;

        if (latest_run >= 0 || (run = get_run(current_run)) == nullptr) {
            // Stop search if we discovered a key in another run, or
            // if there are no more runs to search
            return;
        } else if ((current_val = run->get(key)) == nullptr) {
            // Couldn't find the key in the current run, so we need
            // to keep searching.
            search();
        } else {
            // Update val if the run is more recent than the
            // last, then stop searching since there's no need
            // to search later runs.
            lock.lock();

            if (latest_run < 0 || current_run < latest_run) {
                latest_run = current_run;
                latest_val = *current_val;
            }

            lock.unlock();
            delete current_val;
        }
    };

    worker_pool.launch(search);
    worker_pool.wait_all();

    // Step 3: Output the associated value if the key is found
    if (latest_run >= 0 && latest_val != VAL_TOMBSTONE) cout << latest_val;
    
    // Step 4: Output an empty line if the key is not found
    cout << endl;
}

void LSMTree::range(KEY_t start, KEY_t end) {
    // Declare variables needed for the range query
    vector<entry_t> *buffer_range;
    map<int, vector<entry_t> *> ranges;
    SpinLock lock;
    atomic<int> counter;
    MergeContext merge_ctx;
    entry_t entry;

    // Check if the range is valid, if not, print an empty line and return
    if (end <= start) {
        cout << endl;
        return;
    } else {
        // Convert to inclusive bound
        end -= 1;
    }

    /*
     * Search buffer for the specified range and store the result in 'ranges'
     * This is done by calling buffer.range(start, end) and inserting the result
     * into the 'ranges' map with the key '0'. This indicates that the buffer has the highest priority 
     * (i.e., the most recent data) when merging the results later
     */
    ranges.insert({0, buffer.range(start, end)});

    /*
     * Prepare a worker task for searching runs for the specified range.
     * The worker task will be executed in parallel using worker threads.
     */
    counter = 0;

    worker_task search = [&] {
        int current_run;
        Run *run;

        // Get the next run index to be searched
        current_run = counter++;

        // If the run exists, search the run for the specified range
        if ((run = get_run(current_run)) != nullptr) {
            // Lock the 'ranges' map to ensure thread safety
            lock.lock();
            // Insert the subrange result from the run into the 'ranges' map
            ranges.insert({current_run + 1, run->range(start, end)});
            // Unlock the 'ranges' map
            lock.unlock();
            // Continue searching for more runs if they exist
            search();
        }
    };

    // Launch the parallel search using worker threads
    worker_pool.launch(search);
    // Wait for all worker threads to complete their tasks
    worker_pool.wait_all();

    /*
     * Merge the resulting ranges from both buffer and runs using MergeContext.
     * This step is performed to combine the results in the correct order.
     */
    for (const auto& kv : ranges) {
        merge_ctx.add(kv.second->data(), kv.second->size());
    }

    // Print the merged key-value pairs, excluding tombstones (deleted keys)
    while (!merge_ctx.done()) {
        entry = merge_ctx.next();
        if (entry.val != VAL_TOMBSTONE) {
            cout << entry.key << ":" << entry.val;
            if (!merge_ctx.done()) cout << " ";
        }
    }

    // Print a newline character to indicate the end of the output
    cout << endl;

    /*
     * Cleanup subrange vectors by deallocating memory for each vector in 'ranges'
     */
    for (auto& range : ranges) {
        delete range.second;
    }
}

// Deletes a key-value pair from the LSM tree by inserting a tombstone value
void LSMTree::del(KEY_t key) {
    put(key, VAL_TOMBSTONE);
}

// Loads an LSM tree from a file
void LSMTree::load(string file_path) {
    ifstream stream;
    entry_t entry;
    
    // Remove trailing quote from file_path, if present.
    if (!file_path.empty() && file_path.back() == '"') {
        file_path.pop_back();
    }
    
    // Open the input file stream for the specified file path.
    stream.open(file_path, ifstream::binary);
    
    // Check if the input file stream was successfully opened.
    if (stream.is_open()) {
        // Iterate through all entries in the input file.
        while (stream >> entry) {
            // For each entry, insert its key and value into the LSM tree.
            put(entry.key, entry.val);
        }
    } else {
        // If the input file stream could not be opened, print an error message and exit.
        die("Could not locate file '" + file_path + "'.");
    }
}

void LSMTree::printStats() {
    int logicalPairs = 0;

    // Print Logical Pairs per level.
    // This part of the function prints the number of valid key-value pairs
    // present in each level of the LSM tree.
    cout << "Logical Pairs: ";
    for (int levelIdx = 0; levelIdx < levels.size(); levelIdx++) {
        Level& level = levels[levelIdx];
        int levelKeyCount = 0;

        // Iterate through all runs in the current level.
        // A level can have multiple runs, so we need to process each run.
        for (const Run& run : level.runs) {
            // Iterate through all entries in the current run.
            // Each run contains multiple entries, and we need to process
            // each entry individually to determine if it's a valid pair.
            for (const entry_t& entry : run.entries) {
                // If the entry's value is not a tombstone, increment the key count.
                // Tombstone values are used to represent deleted keys,
                // so they are not considered valid key-value pairs.
                if (entry.val != VAL_TOMBSTONE) {
                    levelKeyCount++;
                    logicalPairs++;
                }
            }
        }

        // Print the key count for the current level.
        // This line outputs the number of valid key-value pairs in the current level.
        cout << "LVL" << (levelIdx + 1) << ": " << levelKeyCount;
        if (levelIdx < levels.size() - 1) {
            cout << ", ";
        } else {
            cout << endl;
        }
    }

    // Include buffer entries in the total logical pairs count.
    // The buffer contains key-value pairs that haven't been merged into the LSM tree yet,
    // so we need to count those as well.
    for (const entry_t& entry : buffer.entries) {
        if (entry.val != VAL_TOMBSTONE) {
            logicalPairs++;
        }
    }

    // Print the total number of logical pairs.
    // This line outputs the combined total of valid key-value pairs across all levels and the buffer.
    cout << "Total Logical Pairs: " << logicalPairs << endl;

    // Print the key, value, and level information for each entry in the LSM tree.
    // This part of the function iterates through each level and its runs in the LSM tree,
    // printing the key-value-level information for each non-tombstone entry.
    for (int levelIdx = 0; levelIdx < levels.size(); levelIdx++) {
        Level& level = levels[levelIdx];
        for (const Run& run : level.runs) {
            for (const entry_t& entry : run.entries) {
                if (entry.val != VAL_TOMBSTONE) {
                    cout << entry.key << ":" << entry.val << ":L" << (levelIdx + 1) << " ";
                }
            }
        }
    }

    // Print buffer entries.
    // This part of the function prints the key-value information for each non-tombstone
    // entry present in the buffer.
    for (const entry_t& entry : buffer.entries) {
        if (entry.val != VAL_TOMBSTONE) {
            cout << entry.key << ":" << entry.val << ":Buffer ";
        }
    }
    cout << endl;

}

void LSMTree::put_metrics(string file_path) {
    using namespace std::chrono;

    // Write latency
    auto start = high_resolution_clock::now();
    load(file_path);
    auto end = high_resolution_clock::now();
    auto write_latency = duration_cast<microseconds>(end - start).count();
    std::cout << "Write latency: " << write_latency << " us" << std::endl;

}

void LSMTree::range_metrics(KEY_t start, KEY_t end) {
    using namespace std::chrono;

    // Read latency
    auto start_time = high_resolution_clock::now();
    range(start, end);
    auto end_time = high_resolution_clock::now();
    auto read_latency = duration_cast<microseconds>(end_time - start_time).count();
    std::cout << "Read latency: " << read_latency << " us" << std::endl;

}
