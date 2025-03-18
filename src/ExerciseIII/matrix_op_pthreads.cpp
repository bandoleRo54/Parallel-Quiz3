#include <iostream>
#include <cstdlib>
#include <vector>
#include <pthread.h>

using namespace std;

// Data structure to hold matrix dimensions, input matrices, and result matrices.
struct ThreadData {
    int r1, c1, r2, c2;
    vector<vector<int>> A;
    vector<vector<int>> B;
    vector<vector<int>> addResult;   // Result for A + B
    vector<vector<int>> mulResult;   // Result for A x B
    vector<vector<int>> transResult; // Transpose of A
};

// Task 1: Matrix Addition Thread Function
// Computes addResult = A + B if dimensions match.
void* additionTask(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    if (data->r1 == data->r2 && data->c1 == data->c2) {
        data->addResult.resize(data->r1, vector<int>(data->c1, 0));
        for (int i = 0; i < data->r1; i++) {
            for (int j = 0; j < data->c1; j++) {
                data->addResult[i][j] = data->A[i][j] + data->B[i][j];
            }
        }
    }
    pthread_exit(NULL);
}

// Task 2: Matrix Multiplication Thread Function
// Computes mulResult = A x B if A's columns equal B's rows.
void* multiplicationTask(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    if (data->c1 == data->r2) {
        data->mulResult.resize(data->r1, vector<int>(data->c2, 0));
        for (int i = 0; i < data->r1; i++) {
            for (int j = 0; j < data->c2; j++) {
                int sum = 0;
                for (int k = 0; k < data->c1; k++) {
                    sum += data->A[i][k] * data->B[k][j];
                }
                data->mulResult[i][j] = sum;
            }
        }
    }
    pthread_exit(NULL);
}

// Task 3: Matrix Transpose Thread Function
// Computes transResult = transpose of Matrix A.
void* transposeTask(void* arg) {
    ThreadData* data = (ThreadData*) arg;
    data->transResult.resize(data->c1, vector<int>(data->r1, 0));
    for (int i = 0; i < data->r1; i++) {
        for (int j = 0; j < data->c1; j++) {
            data->transResult[j][i] = data->A[i][j];
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    // Check if enough arguments are provided.
    // Expected: program r1 c1 r2 c2 <A-elements> <B-elements>
    if (argc < 5) {
        cerr << "Usage: " << argv[0] 
             << " r1 c1 r2 c2 <Matrix A elements> <Matrix B elements>" << endl;
        return 1;
    }
    
    // Parse dimensions from command-line arguments.
    int r1 = atoi(argv[1]);
    int c1 = atoi(argv[2]);
    int r2 = atoi(argv[3]);
    int c2 = atoi(argv[4]);
    
    // Total expected arguments: 1 (program name) + 4 (dimensions) + (r1*c1 + r2*c2) numbers.
    int expectedArgs = 1 + 4 + (r1 * c1) + (r2 * c2);
    if (argc != expectedArgs) {
        cerr << "Error: Expected " << (expectedArgs - 1)
             << " arguments but got " << (argc - 1) << endl;
        return 1;
    }
    
    // Read Matrix A and Matrix B from the arguments.
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
    
    // Prepare the shared data for threads.
    ThreadData data;
    data.r1 = r1; data.c1 = c1; data.r2 = r2; data.c2 = c2;
    data.A = A;
    data.B = B;
    
    // Create three threads, one per task.
    pthread_t tid_add, tid_mul, tid_trans;
    int rc;
    
    rc = pthread_create(&tid_add, NULL, additionTask, (void*)&data);
    if(rc) {
        cerr << "Error: Unable to create addition thread (" << rc << ")." << endl;
        exit(-1);
    }
    
    rc = pthread_create(&tid_mul, NULL, multiplicationTask, (void*)&data);
    if(rc) {
        cerr << "Error: Unable to create multiplication thread (" << rc << ")." << endl;
        exit(-1);
    }
    
    rc = pthread_create(&tid_trans, NULL, transposeTask, (void*)&data);
    if(rc) {
        cerr << "Error: Unable to create transpose thread (" << rc << ")." << endl;
        exit(-1);
    }
    
    // Wait for all threads to complete.
    pthread_join(tid_add, NULL);
    pthread_join(tid_mul, NULL);
    pthread_join(tid_trans, NULL);
    
    // Display the results.
    // Task 1: Matrix Addition Result
    cout << "Result of Matrix Addition (A + B):" << endl;
    if (r1 == r2 && c1 == c2) {
        for (int i = 0; i < r1; i++){
            for (int j = 0; j < c1; j++){
                cout << data.addResult[i][j] << " ";
            }
            cout << endl;
        }
    } else {
        cout << "Matrix addition not possible due to dimension mismatch." << endl;
    }
    cout << endl;
    
    // Task 2: Matrix Multiplication Result
    cout << "Result of Matrix Multiplication (A x B):" << endl;
    if (c1 == r2) {
        for (int i = 0; i < r1; i++){
            for (int j = 0; j < c2; j++){
                cout << data.mulResult[i][j] << " ";
            }
            cout << endl;
        }
    } else {
        cout << "Matrix multiplication not possible (A's columns must equal B's rows)." << endl;
    }
    cout << endl;
    
    // Task 3: Transpose of Matrix A
    cout << "Transpose of Matrix A:" << endl;
    for (int i = 0; i < data.c1; i++){
        for (int j = 0; j < data.r1; j++){
            cout << data.transResult[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
    
    return 0;
}
