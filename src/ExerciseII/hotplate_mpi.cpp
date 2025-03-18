#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>

using namespace std;

// Initializes the local portion of the matrix using global indices.
// The local matrix has dimensions (local_rows+2) x global_cols, where row 0 and row local_rows+1 are ghost rows.
void initialize_local(int global_rows, int global_cols, int local_start, int local_rows, vector<vector<float>> &mat) {
    // Initialize only the "actual" rows (indices 1..local_rows)
    for (int i = 1; i <= local_rows; i++){
        int global_row = local_start + i - 1;
        for (int j = 0; j < global_cols; j++){
            // Apply the boundary conditions using global indices.
            if (global_row == 0 || j == 0 || j == global_cols - 1) {
                mat[i][j] = 0.0f; // cold boundary
            } else if (global_row == global_rows - 1) {
                mat[i][j] = 100.0f; // hot border
            } else if (global_row == 400 || j < 500) {
                mat[i][j] = 100.0f; // hot region (as in your original code)
            } else if (global_row == 512 && j == 512) {
                mat[i][j] = 100.0f; // hot center
            } else {
                mat[i][j] = 0.0f;
            }
        }
    }
}

int main(int argc, char* argv[]){
    MPI_Init(&argc, &argv);
    
    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    
    if(argc < 3) {
        if(rank == 0)
            cout << "Usage: " << argv[0] << " <rows> <cols>" << endl;
        MPI_Finalize();
        return 1;
    }
    
    int global_rows = atoi(argv[1]);
    int global_cols = atoi(argv[2]);
    
    // Distribute the global rows among processes.
    // Each process will get a contiguous block of rows.
    int rows_per_proc = global_rows / nprocs;
    int remainder = global_rows % nprocs;
    
    // Each process gets an extra row if its rank is less than remainder.
    int local_rows = rows_per_proc + (rank < remainder ? 1 : 0);
    
    // Compute the starting global row index for this process.
    int local_start;
    if(rank < remainder)
        local_start = rank * (rows_per_proc + 1);
    else
        local_start = rank * rows_per_proc + remainder;
    
    // Allocate local matrices with two extra ghost rows:
    // Row 0 and row local_rows+1 are used for ghost data.
    vector<vector<float>> local_matrix1(local_rows + 2, vector<float>(global_cols, 0));
    vector<vector<float>> local_matrix2(local_rows + 2, vector<float>(global_cols, 0));
    
    // Initialize the actual local subdomain (rows 1..local_rows) using the global index.
    initialize_local(global_rows, global_cols, local_start, local_rows, local_matrix1);
    initialize_local(global_rows, global_cols, local_start, local_rows, local_matrix2);
    
    // For processes that are at the global boundaries, set the ghost rows according to the fixed boundary conditions.
    if(rank == 0) {
        for (int j = 0; j < global_cols; j++){
            local_matrix1[0][j] = 0.0f;
            local_matrix2[0][j] = 0.0f;
        }
    }
    if(rank == nprocs - 1) {
        for (int j = 0; j < global_cols; j++){
            local_matrix1[local_rows+1][j] = 100.0f;
            local_matrix2[local_rows+1][j] = 100.0f;
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();
    
    bool global_done = false;
    int iter = 0;
    
    while (!global_done) {
        // --- Exchange ghost rows for matrix1 before first sweep ---
        // If not the top process, send first actual row upward and receive ghost row.
        if(rank > 0) {
            MPI_Sendrecv(&local_matrix1[1][0], global_cols, MPI_FLOAT, rank-1, 0,
                         &local_matrix1[0][0], global_cols, MPI_FLOAT, rank-1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        // If not the bottom process, send last actual row downward and receive ghost row.
        if(rank < nprocs - 1) {
            MPI_Sendrecv(&local_matrix1[local_rows][0], global_cols, MPI_FLOAT, rank+1, 0,
                         &local_matrix1[local_rows+1][0], global_cols, MPI_FLOAT, rank+1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        // --- First sweep: update local_matrix2 from local_matrix1 ---
        for (int i = 1; i <= local_rows; i++){
            int global_row = local_start + i - 1;
            // Skip updating the global boundaries.
            if (global_row == 0 || global_row == global_rows - 1)
                continue;
            for (int j = 1; j < global_cols - 1; j++){
                // Skip fixed hot cells.
                if ((global_row == 500 && j < 400) || (global_row == 512 && j == 512))
                    continue;
                local_matrix2[i][j] = (local_matrix1[i+1][j] + local_matrix1[i-1][j] +
                                       local_matrix1[i][j+1] + local_matrix1[i][j-1] +
                                       4 * local_matrix1[i][j]) / 8.0f;
            }
        }
        
        // --- Exchange ghost rows for matrix2 ---
        if(rank > 0) {
            MPI_Sendrecv(&local_matrix2[1][0], global_cols, MPI_FLOAT, rank-1, 0,
                         &local_matrix2[0][0], global_cols, MPI_FLOAT, rank-1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if(rank < nprocs - 1) {
            MPI_Sendrecv(&local_matrix2[local_rows][0], global_cols, MPI_FLOAT, rank+1, 0,
                         &local_matrix2[local_rows+1][0], global_cols, MPI_FLOAT, rank+1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        // --- Convergence check on local_matrix2 ---
        bool local_converged = true;
        for (int i = 1; i <= local_rows; i++){
            int global_row = local_start + i - 1;
            if (global_row == 0 || global_row == global_rows - 1)
                continue;
            for (int j = 1; j < global_cols - 1; j++){
                if ((global_row == 500 && j < 400) || (global_row == 512 && j == 512))
                    continue;
                float conv = local_matrix2[i][j] - ((local_matrix2[i+1][j] + local_matrix2[i-1][j] +
                                                     local_matrix2[i][j+1] + local_matrix2[i][j-1]) / 4.0f);
                if (fabs(conv) > 0.1f) {
                    local_converged = false;
                    break;
                }
            }
            if (!local_converged)
                break;
        }
        
        int local_flag = local_converged ? 1 : 0;
        int global_flag;
        MPI_Allreduce(&local_flag, &global_flag, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        if (global_flag == 1) {
            global_done = true;
            break;
        }
        
        // --- Exchange ghost rows for matrix2 before second sweep ---
        if(rank > 0) {
            MPI_Sendrecv(&local_matrix2[1][0], global_cols, MPI_FLOAT, rank-1, 0,
                         &local_matrix2[0][0], global_cols, MPI_FLOAT, rank-1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if(rank < nprocs - 1) {
            MPI_Sendrecv(&local_matrix2[local_rows][0], global_cols, MPI_FLOAT, rank+1, 0,
                         &local_matrix2[local_rows+1][0], global_cols, MPI_FLOAT, rank+1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        // --- Second sweep: update local_matrix1 from local_matrix2 ---
        for (int i = 1; i <= local_rows; i++){
            int global_row = local_start + i - 1;
            if (global_row == 0 || global_row == global_rows - 1)
                continue;
            for (int j = 1; j < global_cols - 1; j++){
                if ((global_row == 500 && j < 400) || (global_row == 512 && j == 512))
                    continue;
                local_matrix1[i][j] = (local_matrix2[i+1][j] + local_matrix2[i-1][j] +
                                       local_matrix2[i][j+1] + local_matrix2[i][j-1] +
                                       4 * local_matrix2[i][j]) / 8.0f;
            }
        }
        
        // --- Exchange ghost rows for matrix1 ---
        if(rank > 0) {
            MPI_Sendrecv(&local_matrix1[1][0], global_cols, MPI_FLOAT, rank-1, 0,
                         &local_matrix1[0][0], global_cols, MPI_FLOAT, rank-1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if(rank < nprocs - 1) {
            MPI_Sendrecv(&local_matrix1[local_rows][0], global_cols, MPI_FLOAT, rank+1, 0,
                         &local_matrix1[local_rows+1][0], global_cols, MPI_FLOAT, rank+1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        // --- Convergence check on local_matrix1 ---
        local_converged = true;
        for (int i = 1; i <= local_rows; i++){
            int global_row = local_start + i - 1;
            if (global_row == 0 || global_row == global_rows - 1)
                continue;
            for (int j = 1; j < global_cols - 1; j++){
                if ((global_row == 500 && j < 400) || (global_row == 512 && j == 512))
                    continue;
                float conv = local_matrix1[i][j] - ((local_matrix1[i+1][j] + local_matrix1[i-1][j] +
                                                     local_matrix1[i][j+1] + local_matrix1[i][j-1]) / 4.0f);
                if (fabs(conv) > 0.1f) {
                    local_converged = false;
                    break;
                }
            }
            if (!local_converged)
                break;
        }
        local_flag = local_converged ? 1 : 0;
        MPI_Allreduce(&local_flag, &global_flag, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        if (global_flag == 1) {
            global_done = true;
            break;
        }
        
        iter++;
    }
    
    // Count hot cells in the local domain.
    int local_hot = 0;
    for (int i = 1; i <= local_rows; i++){
        for (int j = 0; j < global_cols; j++){
            if (local_matrix1[i][j] > 50.0f)
                local_hot++;
        }
    }
    
    int global_hot = 0;
    MPI_Reduce(&local_hot, &global_hot, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    
    double t_end = MPI_Wtime();
    
    if(rank == 0) {
        cout << "N° Iteraciones: " << iter << endl;
		cout << "Tiempo de ejecucion: " << exec_time << " segundos" << endl;
		cout << "Num. de celdas calientes: " << hot_cells << endl;
    }
    
    MPI_Finalize();
    return 0;
}
