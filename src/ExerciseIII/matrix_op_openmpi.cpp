#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <vector>

using namespace std;

int main(int argc, char* argv[]){
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // We require at least 3 processes for our 3 tasks.
    if (size < 3) {
        if (rank == 0)
            cerr << "This program requires at least 3 MPI processes." << endl;
        MPI_Finalize();
        return 1;
    }
    
    // All processes will have these dimensions.
    int dims[4]; // dims[0]=r1, dims[1]=c1, dims[2]=r2, dims[3]=c2;
    int r1, c1, r2, c2;
    int sizeA, sizeB;  // Number of elements in matrices A and B.
    
    // Process 0 reads the command-line arguments.
    vector<int> A; // Matrix A stored as a 1D array (row-major)
    vector<int> B; // Matrix B stored as a 1D array (row-major)
    if (rank == 0) {
        if (argc < 5) {
            cerr << "Usage: " << argv[0] 
                 << " r1 c1 r2 c2 <Matrix A elements> <Matrix B elements>" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        r1 = atoi(argv[1]);
        c1 = atoi(argv[2]);
        r2 = atoi(argv[3]);
        c2 = atoi(argv[4]);
        dims[0] = r1; dims[1] = c1; dims[2] = r2; dims[3] = c2;
        
        sizeA = r1 * c1;
        sizeB = r2 * c2;
        
        // Check that the number of elements provided is correct.
        if (argc != 1 + 4 + sizeA + sizeB) {
            cerr << "Error: Expected " << (1+4+sizeA+sizeB - 1)
                 << " arguments but got " << (argc - 1) << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        A.resize(sizeA);
        B.resize(sizeB);
        int index = 5;
        for (int i = 0; i < sizeA; i++) {
            A[i] = atoi(argv[index++]);
        }
        for (int i = 0; i < sizeB; i++) {
            B[i] = atoi(argv[index++]);
        }
    }
    
    // Broadcast matrix dimensions to all processes.
    MPI_Bcast(dims, 4, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0) {
        r1 = dims[0]; c1 = dims[1]; r2 = dims[2]; c2 = dims[3];
        sizeA = r1 * c1;
        sizeB = r2 * c2;
    }
    
    // Processes 1 and 2 allocate space for the matrices they will receive.
    if (rank == 1) {
        A.resize(sizeA);
        B.resize(sizeB);
    } else if (rank == 2) {
        A.resize(sizeA);
    }
    
    // Process 0 sends data to processes 1 and 2.
    if (rank == 0) {
        // Send A and B to Process 1 for multiplication.
        MPI_Send(A.data(), sizeA, MPI_INT, 1, 10, MPI_COMM_WORLD);
        MPI_Send(B.data(), sizeB, MPI_INT, 1, 11, MPI_COMM_WORLD);
        // Send A to Process 2 for transpose.
        MPI_Send(A.data(), sizeA, MPI_INT, 2, 20, MPI_COMM_WORLD);
    }
    
    // Containers for results.
    vector<int> addResult;   // Matrix addition result (dimensions r1 x c1)
    vector<int> mulResult;   // Matrix multiplication result (dimensions r1 x c2)
    vector<int> transResult; // Transpose of A (dimensions c1 x r1)
    
    if (rank == 0) {
        // Process 0 computes Matrix Addition if possible.
        if (r1 == r2 && c1 == c2) {
            addResult.resize(sizeA);
            for (int i = 0; i < sizeA; i++) {
                addResult[i] = A[i] + B[i];
            }
        }
        // (Multiplication and Transpose results will be received from Processes 1 and 2.)
    } else if (rank == 1) {
        // Process 1: Matrix Multiplication (if defined: c1 must equal r2).
        int mul_possible = (c1 == r2) ? 1 : 0;
        // Data already received from Process 0.
        if (mul_possible) {
            // Result dimensions: r1 x c2.
            mulResult.resize(r1 * c2, 0);
            for (int i = 0; i < r1; i++) {
                for (int j = 0; j < c2; j++) {
                    int sum = 0;
                    for (int k = 0; k < c1; k++) {
                        sum += A[i * c1 + k] * B[k * c2 + j];
                    }
                    mulResult[i * c2 + j] = sum;
                }
            }
        }
        // Send a flag indicating whether multiplication was performed.
        MPI_Send(&mul_possible, 1, MPI_INT, 0, 100, MPI_COMM_WORLD);
        // If multiplication was performed, send the result.
        if (mul_possible) {
            MPI_Send(mulResult.data(), r1 * c2, MPI_INT, 0, 101, MPI_COMM_WORLD);
        }
    } else if (rank == 2) {
        // Process 2: Compute the transpose of matrix A.
        // Transpose dimensions: c1 x r1.
        transResult.resize(sizeA); // same number of elements as A
        for (int i = 0; i < r1; i++) {
            for (int j = 0; j < c1; j++) {
                transResult[j * r1 + i] = A[i * c1 + j];
            }
        }
        // Send the transpose result to Process 0.
        MPI_Send(transResult.data(), sizeA, MPI_INT, 0, 200, MPI_COMM_WORLD);
    }
    
    // Process 0 collects and prints the results.
    if (rank == 0) {        
        // Print Matrix Addition result.
        cout << "Result of Matrix Addition (A + B):" << endl;
        if (r1 == r2 && c1 == c2) {
            for (int i = 0; i < r1; i++){
                for (int j = 0; j < c1; j++){
                    cout << addResult[i * c1 + j] << " ";
                }
                cout << endl;
            }
        } else {
            cout << "Matrix addition not possible due to dimension mismatch." << endl;
        }
        cout << endl;
        
        // Receive and print Matrix Multiplication result.
        int mul_possible;
        MPI_Recv(&mul_possible, 1, MPI_INT, 1, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        cout << "Result of Matrix Multiplication (A x B):" << endl;
        if (mul_possible) {
            mulResult.resize(r1 * c2);
            MPI_Recv(mulResult.data(), r1 * c2, MPI_INT, 1, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = 0; i < r1; i++){
                for (int j = 0; j < c2; j++){
                    cout << mulResult[i * c2 + j] << " ";
                }
                cout << endl;
            }
        } else {
            cout << "Matrix multiplication not possible (A's columns must equal B's rows)." << endl;
        }
        cout << endl;
        
        // Receive and print the Transpose of Matrix A.
        transResult.resize(sizeA);
        MPI_Recv(transResult.data(), sizeA, MPI_INT, 2, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        cout << "Transpose of Matrix A:" << endl;
        // Transpose dimensions: c1 x r1.
        for (int i = 0; i < c1; i++){
            for (int j = 0; j < r1; j++){
                cout << transResult[i * r1 + j] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
    
    MPI_Finalize();
    return 0;
}
