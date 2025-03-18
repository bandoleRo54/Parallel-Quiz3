#include <mpi.h>
#include <iostream>
#include <cstdlib>
using namespace std;

int main(int argc, char* argv[]){
    MPI_Init(&argc, &argv);

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    int rows, cols;
    int *matrix = nullptr;  // Flattened matrix (stored in row-major order)
    int *vec = nullptr;
    int *result = nullptr;

    // Process 0 reads the input from command-line.
    if (rank == 0) {
        if(argc < 4) {
            cout << "Usage: " << argv[0] 
                 << " <rows> <cols> <matrix values>... <vector values>..." << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        rows = atoi(argv[1]);
        cols = atoi(argv[2]);
        
        // Allocate and read the matrix as a 1D array (flattened)
        matrix = new int[rows * cols];
        int arg_index = 3;
        for (int i = 0; i < rows * cols; i++){
            matrix[i] = atoi(argv[arg_index++]);
        }
        
        // Allocate and read the vector.
        vec = new int[cols];
        for (int i = 0; i < cols; i++){
            vec[i] = atoi(argv[arg_index++]);
        }
        
        // Allocate the result array.
        result = new int[rows];
    }

    // Broadcast the matrix dimensions to all processes.
    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Ensure all processes allocate the vector.
    if (rank != 0) {
        vec = new int[cols];
    }
    // Broadcast the vector from process 0 to all others.
    MPI_Bcast(vec, cols, MPI_INT, 0, MPI_COMM_WORLD);

    // Determine how many rows each process will handle.
    // We use MPI_Scatterv so that if rows % num_procs != 0, the extra rows are distributed.
    int *sendcounts = new int[num_procs];
    int *displs = new int[num_procs];
    for (int i = 0; i < num_procs; i++){
        // Each process gets (rows/num_procs) rows, plus one extra if i < rows % num_procs.
        int local_rows = rows / num_procs + (i < (rows % num_procs) ? 1 : 0);
        sendcounts[i] = local_rows * cols;  // number of matrix elements for process i.
    }
    displs[0] = 0;
    for (int i = 1; i < num_procs; i++){
        displs[i] = displs[i-1] + sendcounts[i-1];
    }
    // Determine local number of rows for this process.
    int local_rows = sendcounts[rank] / cols;
    
    // Allocate buffer for the local block of the matrix.
    int *local_matrix = new int[local_rows * cols];
    
    // Scatter the matrix rows among all processes.
    MPI_Scatterv(matrix, sendcounts, displs, MPI_INT,
                 local_matrix, sendcounts[rank], MPI_INT,
                 0, MPI_COMM_WORLD);
    
    // Each process computes its partial matrix-vector multiplication.
    int *local_result = new int[local_rows];
    for (int i = 0; i < local_rows; i++){
        local_result[i] = 0;
        for (int j = 0; j < cols; j++){
            local_result[i] += local_matrix[i * cols + j] * vec[j];
        }
    }
    
    // Prepare arrays for gathering the results.
    int *recvcounts = new int[num_procs];
    int *recvdispls = new int[num_procs];
    for (int i = 0; i < num_procs; i++){
        recvcounts[i] = sendcounts[i] / cols;  // Each process sends one integer per row.
    }
    recvdispls[0] = 0;
    for (int i = 1; i < num_procs; i++){
        recvdispls[i] = recvdispls[i-1] + recvcounts[i-1];
    }
    
    // Gather the partial results into the final result array at process 0.
    MPI_Gatherv(local_result, local_rows, MPI_INT,
                result, recvcounts, recvdispls, MPI_INT,
                0, MPI_COMM_WORLD);
    
    // Process 0 prints the final result.
    if (rank == 0){
        cout << "Result (size " << rows << "):" << endl << "{ ";
        for (int i = 0; i < rows; i++){
            cout << result[i] << " ";
        }
        cout << "}" << endl;
    }
    
    // Free dynamically allocated memory.
    if (rank == 0) {
        delete[] matrix;
        delete[] vec;
        delete[] result;
    } else {
        delete[] vec;
    }
    delete[] local_matrix;
    delete[] local_result;
    delete[] sendcounts;
    delete[] displs;
    delete[] recvcounts;
    delete[] recvdispls;
    
    MPI_Finalize();
    return 0;
}
