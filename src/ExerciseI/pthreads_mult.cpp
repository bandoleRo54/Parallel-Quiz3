#include <iostream>
#include <cstdlib>
#include <pthread.h>
using namespace std;

struct ThreadData {
    int thread_id;      
    int num_threads;    
    int rows;           
    int cols;           
    int **matrix;       
    int *vec;           
    int *result;        
};

void* thread_multiply(void *arg) {
    ThreadData *data = (ThreadData*) arg;
    int rows_per_thread = data->rows / data->num_threads;
    int start_row = data->thread_id * rows_per_thread;
    int end_row = (data->thread_id == data->num_threads - 1) 
                  ? data->rows 
                  : start_row + rows_per_thread;
    
    for (int i = start_row; i < end_row; i++) {
        data->result[i] = 0;
        for (int j = 0; j < data->cols; j++) {
            data->result[i] += data->matrix[i][j] * data->vec[j];
        }
    }
    
    pthread_exit(NULL);
}

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
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    
    for (int i = 0; i < num_threads; i++){
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].rows = rows;
        thread_data[i].cols = cols;
        thread_data[i].matrix = matrix;
        thread_data[i].vec = vec;
        thread_data[i].result = result;
        
        int rc = pthread_create(&threads[i], NULL, thread_multiply, (void*)&thread_data[i]);
        if (rc){
            cerr << "Error: Unable to create thread, " << rc << endl;
            exit(-1);
        }
    }
    
    for (int i = 0; i < num_threads; i++){
        pthread_join(threads[i], NULL);
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
