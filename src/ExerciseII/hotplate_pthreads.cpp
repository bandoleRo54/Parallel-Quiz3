#include <iostream>
#include <pthread.h>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <chrono>

using namespace std;

// Global simulation parameters and matrices.
int row, col;
int iter = 0;
bool global_done = false;
int num_threads;
vector<bool> convergence_flags; // one per thread
pthread_barrier_t barrier;

vector<vector<float>> matrix1;
vector<vector<float>> matrix2;

// Structure passed to each thread.
struct ThreadData {
    int tid;
    int start; // first row (inclusive) this thread will update
    int end;   // last row (inclusive) this thread will update
};

// Initialization function (same as before).
void initialize(int row, int col, vector<vector<float>> &mat) {
    for (int i = 0; i < row; i++){
        for (int j = 0; j < col; j++){
            if (i == 0 || j == 0 || j == col - 1) {
                mat[i][j] = 0.0f; // cold boundary
            } else if (i == row - 1) {
                mat[i][j] = 100.0f; // hot border
            } else if (i == 400 || j < 500) { // hot region condition (as in your original code)
                mat[i][j] = 100.0f;
            } else if (i == 512 && j == 512) {
                mat[i][j] = 100.0f; // hot center
            } else {
                mat[i][j] = 0.0f;
            }
        }
    }
}

// Thread worker function.
void* thread_func(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    bool local_converged;
    
    while (true) {
        // --- First sweep: update matrix2 from matrix1 ---
        for (int i = data->start; i <= data->end; i++){
            // Skip global boundary rows.
            if(i == 0 || i == row - 1)
                continue;
            for (int j = 1; j < col - 1; j++){
                // Skip fixed hot cells.
                if ((i == 500 && j < 400) || (i == 512 && j == 512))
                    continue;
                matrix2[i][j] = (matrix1[i+1][j] + matrix1[i-1][j] +
                                 matrix1[i][j+1] + matrix1[i][j-1] +
                                 4 * matrix1[i][j]) / 8.0f;
            }
        }
        pthread_barrier_wait(&barrier);
        
        // --- Convergence check on matrix2 ---
        local_converged = true;
        for (int i = data->start; i <= data->end; i++){
            if(i == 0 || i == row - 1)
                continue;
            for (int j = 1; j < col - 1; j++){
                if ((i == 500 && j < 400) || (i == 512 && j == 512))
                    continue;
                float conv = matrix2[i][j] - ((matrix2[i+1][j] + matrix2[i-1][j] +
                                               matrix2[i][j+1] + matrix2[i][j-1]) / 4.0f);
                if (fabs(conv) > 0.1f) {
                    local_converged = false;
                    break;
                }
            }
            if (!local_converged)
                break;
        }
        convergence_flags[data->tid] = local_converged;
        pthread_barrier_wait(&barrier);
        
        // Thread 0 aggregates the convergence results.
        if(data->tid == 0) {
            bool all_converged = true;
            for (int t = 0; t < num_threads; t++){
                if (!convergence_flags[t]){
                    all_converged = false;
                    break;
                }
            }
            global_done = all_converged;
        }
        pthread_barrier_wait(&barrier);
        if (global_done)
            break;
        
        // --- Second sweep: update matrix1 from matrix2 ---
        for (int i = data->start; i <= data->end; i++){
            if(i == 0 || i == row - 1)
                continue;
            for (int j = 1; j < col - 1; j++){
                if ((i == 500 && j < 400) || (i == 512 && j == 512))
                    continue;
                matrix1[i][j] = (matrix2[i+1][j] + matrix2[i-1][j] +
                                 matrix2[i][j+1] + matrix2[i][j-1] +
                                 4 * matrix2[i][j]) / 8.0f;
            }
        }
        pthread_barrier_wait(&barrier);
        
        // --- Convergence check on matrix1 ---
        local_converged = true;
        for (int i = data->start; i <= data->end; i++){
            if(i == 0 || i == row - 1)
                continue;
            for (int j = 1; j < col - 1; j++){
                if ((i == 500 && j < 400) || (i == 512 && j == 512))
                    continue;
                float conv = matrix1[i][j] - ((matrix1[i+1][j] + matrix1[i-1][j] +
                                               matrix1[i][j+1] + matrix1[i][j-1]) / 4.0f);
                if (fabs(conv) > 0.1f) {
                    local_converged = false;
                    break;
                }
            }
            if (!local_converged)
                break;
        }
        convergence_flags[data->tid] = local_converged;
        pthread_barrier_wait(&barrier);
        
        if(data->tid == 0) {
            bool all_converged = true;
            for (int t = 0; t < num_threads; t++){
                if (!convergence_flags[t]){
                    all_converged = false;
                    break;
                }
            }
            global_done = all_converged;
            iter++; // Count one full cycle (both sweeps)
        }
        pthread_barrier_wait(&barrier);
        if (global_done)
            break;
    }
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <rows> <cols> <num_threads>" << endl;
        return 1;
    }
    
    row = atoi(argv[1]);
    col = atoi(argv[2]);
    num_threads = atoi(argv[3]);
    
    // Allocate the matrices.
    matrix1.resize(row, vector<float>(col, 0));
    matrix2.resize(row, vector<float>(col, 0));
    
    // Initialize both matrices.
    initialize(row, col, matrix1);
    initialize(row, col, matrix2);
    
    convergence_flags.resize(num_threads, false);
    pthread_barrier_init(&barrier, NULL, num_threads);
    
    // Create thread data and spawn threads.
    vector<pthread_t> threads(num_threads);
    vector<ThreadData> thread_data(num_threads);
    
    // Divide the interior rows (1 .. row-2) among threads.
    int interior_rows = row - 2;
    int rows_per_thread = interior_rows / num_threads;
    int remainder = interior_rows % num_threads;
    int current_start = 1;
    for (int t = 0; t < num_threads; t++){
        thread_data[t].tid = t;
        int extra = (t < remainder) ? 1 : 0;
        thread_data[t].start = current_start;
        thread_data[t].end = current_start + rows_per_thread + extra - 1;
        current_start = thread_data[t].end + 1;
    }
    
    auto t_start = chrono::high_resolution_clock::now();
    
    for (int t = 0; t < num_threads; t++){
        pthread_create(&threads[t], NULL, thread_func, (void*) &thread_data[t]);
    }
    for (int t = 0; t < num_threads; t++){
        pthread_join(threads[t], NULL);
    }
    
    auto t_end = chrono::high_resolution_clock::now();
    double exec_time = chrono::duration<double>(t_end - t_start).count();
    
    int hot_cells = 0;
    for (int i = 0; i < row; i++){
        for (int j = 0; j < col; j++){
            if (matrix1[i][j] > 50.0f)
                hot_cells++;
        }
    }
    
    cout << "N° Iteraciones: " << iter << endl;
    cout << "Tiempo de ejecucion: " << exec_time << " segundos" << endl;
    cout << "Num. de celdas calientes: " << hot_cells << endl;
    
    pthread_barrier_destroy(&barrier);
    return 0;
}
