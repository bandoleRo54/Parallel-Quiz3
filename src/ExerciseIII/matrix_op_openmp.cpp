#include <iostream>
#include <cstdlib>
#include <vector>
#include <omp.h>

using namespace std;

int main(int argc, char* argv[]){
    // Check if enough arguments are provided.
    // Expected: program r1 c1 r2 c2 <Matrix A elements> <Matrix B elements>
    if (argc < 5) {
        cerr << "Usage: " << argv[0] 
             << " r1 c1 r2 c2 <Matrix A elements> <Matrix B elements>" << endl;
        return 1;
    }
    
    // Parse matrix dimensions from command-line arguments.
    int r1 = atoi(argv[1]);
    int c1 = atoi(argv[2]);
    int r2 = atoi(argv[3]);
    int c2 = atoi(argv[4]);
    
    // Expected total arguments = 1 (program name) + 4 (dimensions) + (r1*c1 + r2*c2) numbers.
    int expectedArgs = 1 + 4 + (r1 * c1) + (r2 * c2);
    if (argc != expectedArgs) {
        cerr << "Error: Expected " << (expectedArgs - 1)
             << " arguments but got " << (argc - 1) << endl;
        return 1;
    }
    
    // Read matrix A and matrix B from the command-line arguments.
    vector<vector<int>> A(r1, vector<int>(c1));
    vector<vector<int>> B(r2, vector<int>(c2));
    int index = 5;
    
    // Fill Matrix A.
    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < c1; j++) {
            A[i][j] = atoi(argv[index++]);
        }
    }
    // Fill Matrix B.
    for (int i = 0; i < r2; i++) {
        for (int j = 0; j < c2; j++) {
            B[i][j] = atoi(argv[index++]);
        }
    }
        
    // Variables to hold the results.
    vector<vector<int>> addResult;   // For matrix addition (A + B)
    vector<vector<int>> mulResult;   // For matrix multiplication (A x B)
    vector<vector<int>> transResult; // For transpose of matrix A
    
    // Use OpenMP parallel sections to perform tasks concurrently.
    #pragma omp parallel sections
    {
        // Section for Matrix Addition.
        #pragma omp section
        {
            if (r1 == r2 && c1 == c2) {
                addResult.resize(r1, vector<int>(c1, 0));
                for (int i = 0; i < r1; i++){
                    for (int j = 0; j < c1; j++){
                        addResult[i][j] = A[i][j] + B[i][j];
                    }
                }
            }
        }
        
        // Section for Matrix Multiplication.
        #pragma omp section
        {
            if (c1 == r2) {
                mulResult.resize(r1, vector<int>(c2, 0));
                for (int i = 0; i < r1; i++){
                    for (int j = 0; j < c2; j++){
                        int sum = 0;
                        for (int k = 0; k < c1; k++){
                            sum += A[i][k] * B[k][j];
                        }
                        mulResult[i][j] = sum;
                    }
                }
            }
        }
        
        // Section for Matrix Transpose (of Matrix A).
        #pragma omp section
        {
            transResult.resize(c1, vector<int>(r1, 0));
            for (int i = 0; i < r1; i++){
                for (int j = 0; j < c1; j++){
                    transResult[j][i] = A[i][j];
                }
            }
        }
    } // End of parallel sections.
    
    // Output the results.
    
    // Matrix Addition Result.
    cout << "Result of Matrix Addition (A + B):" << endl;
    if (r1 == r2 && c1 == c2) {
        for (int i = 0; i < r1; i++){
            for (int j = 0; j < c1; j++){
                cout << addResult[i][j] << " ";
            }
            cout << endl;
        }
    } else {
        cout << "Matrix addition not possible due to dimension mismatch." << endl;
    }
    cout << endl;
    
    // Matrix Multiplication Result.
    cout << "Result of Matrix Multiplication (A x B):" << endl;
    if (c1 == r2) {
        for (int i = 0; i < r1; i++){
            for (int j = 0; j < c2; j++){
                cout << mulResult[i][j] << " ";
            }
            cout << endl;
        }
    } else {
        cout << "Matrix multiplication not possible (A's columns must equal B's rows)." << endl;
    }
    cout << endl;
    
    // Matrix Transpose Result.
    cout << "Transpose of Matrix A:" << endl;
    for (int i = 0; i < c1; i++){
        for (int j = 0; j < r1; j++){
            cout << transResult[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
    
    return 0;
}
