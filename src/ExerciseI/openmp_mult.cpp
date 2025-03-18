#include <iostream>
#include <cstdlib>
#include <omp.h>

using namespace std;

int main(int argc, char* argv[]){
    if(argc < 4) {
        cout << "Usage: " << argv[0] << " <rows> <cols> <matrix values>... <vector values>..." << endl;
        return 1;
    }
    
    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);
    int **matrix = new int*[rows];
    for(int i = 0; i < rows; i++){
        matrix[i] = new int[cols];
    }
    
    int *vec = new int[cols];
    int *result = new int[rows];
    int arg_index = 3;
    for (int i = 0; i < rows; i++){
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = atoi(argv[arg_index++]);
        }
    }
    
    for (int j = 0; j < cols; j++){
        vec[j] = atoi(argv[arg_index++]);
    }
    
    int num_threads = 4;
    omp_set_num_threads(num_threads);
    
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < rows; i++){
        int temp = 0;
        for (int j = 0; j < cols; j++){
            temp += matrix[i][j] * vec[j];
        }
        result[i] = temp;
    }
    
    cout << "Result (size " << rows << "):" << endl << "{ ";
    for (int i = 0; i < rows; i++){
        cout << result[i] << " ";
    }
    cout << "}" << endl;
    
    for (int i = 0; i < rows; i++){
        delete[] matrix[i];
    }
    delete[] matrix;
    delete[] vec;
    delete[] result;
    
    return 0;
}