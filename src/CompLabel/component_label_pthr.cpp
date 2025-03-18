#include <iostream>
#include <vector>
#include <algorithm>
#include <pthread.h>
#include <cstdlib>

using namespace std;

// Global shared variables.
static vector<vector<int>> image;
static vector<vector<int>> label;
static vector<vector<int>> newLabel;
static int rows, cols, numThreads;
static bool global_changed;   // Set to true if any thread detects a label change.
static bool done;             // Set to true to signal termination.
static pthread_barrier_t barrier;
static pthread_mutex_t changed_mutex;

// Structure to pass thread-specific data.
struct ThreadData {
    int thread_id;
};

//
// Worker thread function.
// Each thread processes a subset of rows of the image.
//
void* worker(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    int tid = data->thread_id;
    // Compute row boundaries for this thread.
    int start = (rows * tid) / numThreads;
    int end = (rows * (tid + 1)) / numThreads;
    bool local_changed;

    while (true) {
        local_changed = false;
        // Phase 1: Compute new labels for the assigned rows.
        for (int i = start; i < end; i++) {
            for (int j = 0; j < cols; j++) {
                if (image[i][j] == 1) {  // Process only foreground pixels.
                    int current = label[i][j];
                    int minLabel = current;
                    // Check top neighbor.
                    if (i > 0 && image[i - 1][j] == 1)
                        minLabel = min(minLabel, label[i - 1][j]);
                    // Check left neighbor.
                    if (j > 0 && image[i][j - 1] == 1)
                        minLabel = min(minLabel, label[i][j - 1]);
                    // Check bottom neighbor.
                    if (i < rows - 1 && image[i + 1][j] == 1)
                        minLabel = min(minLabel, label[i + 1][j]);
                    // Check right neighbor.
                    if (j < cols - 1 && image[i][j + 1] == 1)
                        minLabel = min(minLabel, label[i][j + 1]);

                    newLabel[i][j] = minLabel;
                    if (minLabel < current) {
                        local_changed = true;
                    }
                } else {
                    // For background pixels.
                    newLabel[i][j] = 0;
                }
            }
        }

        // If any change occurred in this thread, update the global flag.
        if (local_changed) {
            pthread_mutex_lock(&changed_mutex);
            global_changed = true;
            pthread_mutex_unlock(&changed_mutex);
        }

        // Barrier: Wait for all threads to finish updating newLabel.
        pthread_barrier_wait(&barrier);

        // Phase 2: Copy newLabel back into label for the assigned rows.
        for (int i = start; i < end; i++) {
            for (int j = 0; j < cols; j++) {
                label[i][j] = newLabel[i][j];
            }
        }

        // Barrier: Ensure all threads have copied their updates.
        pthread_barrier_wait(&barrier);

        // Phase 3: Termination check.
        // Only thread 0 performs the check and resets global_changed.
        if (tid == 0) {
            if (!global_changed)
                done = true;
            else {
                done = false;
                global_changed = false;  // Reset for the next iteration.
            }
        }
        // Barrier: Broadcast the termination decision to all threads.
        pthread_barrier_wait(&barrier);
        if (done)
            break;
    }
    pthread_exit(nullptr);
}

//
// Main function sets up the image, initializes shared data structures,
// creates threads, and then prints the final labeled image.
//
int main() {
    // Example binary image:
    // 0 represents background; 1 represents a foreground pixel.
    image = {
        {0, 1, 0, 0, 1},
        {1, 1, 0, 1, 1},
        {0, 0, 0, 0, 0},
        {1, 0, 1, 1, 1}
    };

    rows = image.size();
    cols = image[0].size();
    numThreads = 4;  // For example, we use 4 threads.

    // Allocate and initialize label and newLabel arrays.
    label.resize(rows, vector<int>(cols, 0));
    newLabel.resize(rows, vector<int>(cols, 0));
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (image[i][j] == 1) {
                label[i][j] = i * cols + j; // Unique initial label.
            }
        }
    }

    global_changed = false;
    done = false;

    // Initialize the barrier and mutex.
    pthread_barrier_init(&barrier, nullptr, numThreads);
    pthread_mutex_init(&changed_mutex, nullptr);

    // Create worker threads.
    vector<pthread_t> threads(numThreads);
    vector<ThreadData> threadData(numThreads);
    for (int t = 0; t < numThreads; t++) {
        threadData[t].thread_id = t;
        int rc = pthread_create(&threads[t], nullptr, worker, &threadData[t]);
        if (rc) {
            cerr << "Error: unable to create thread, " << rc << endl;
            exit(-1);
        }
    }

    // Join threads.
    for (int t = 0; t < numThreads; t++) {
        pthread_join(threads[t], nullptr);
    }

    // Clean up the barrier and mutex.
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&changed_mutex);

    // Print the final labeled image.
    cout << "Labeled Image:" << endl;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cout << label[i][j] << "\t";
        }
        cout << "\n";
    }

    return 0;
}
