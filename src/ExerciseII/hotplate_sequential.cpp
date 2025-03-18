#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdlib>

using namespace std;

void initialize(int row, int col, vector<vector<float>>& arr) {
    for (int i = 0; i < row; i++){
        for (int j = 0; j < col; j++){
            if (i == 0 || j == 0 || j == (col - 1)) {
                arr[i][j] = 0.0f; // cold boundary
            } else if (i == (row - 1)) {
                arr[i][j] = 100.0f; // hot border
            } else if (i == 400 || j < 500) {
                arr[i][j] = 100.0f; // hot region (check logic if needed)
            } else if (i == 512 && j == 512) {
                arr[i][j] = 100.0f; // hot center
            } else {
                arr[i][j] = 0.0f; // cold by default
            }
        }
    }
}

void new_values(int row, int col, vector<vector<float>>& arr2, const vector<vector<float>>& arr1) {
    for (int i = 1; i < row - 1; i++){
        for (int j = 1; j < col - 1; j++){
            if ((i == 500 && j < 400) || (i == 512 && j == 512))
                continue;
            arr2[i][j] = (arr1[i+1][j] + arr1[i-1][j] +
                          arr1[i][j+1] + arr1[i][j-1] +
                          4 * arr1[i][j]) / 8.0f;
        }
    }
}

int check_convergence(int row, int col, const vector<vector<float>>& arr) {
    float convergence;
    for (int i = 1; i < row - 1; i++){
        for (int j = 1; j < col - 1; j++){
            if ((i == 500 && j < 400) || (i == 512 && j == 512))
                continue;
            convergence = arr[i][j] - ((arr[i+1][j] + arr[i-1][j] +
                                        arr[i][j+1] + arr[i][j-1]) / 4.0f);
            if (fabs(convergence) > 0.1f)
                return 1;
        }
    }
    return 0;
}

int main(int argc, char* argv[]){
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <rows> <cols>" << endl;
        return 1;
    }

    int iter = 0;
    int status1 = 1, status2 = 1;
    int hot_cells = 0;
    int row = atoi(argv[1]);
    int col = atoi(argv[2]);
    
    vector<vector<float>> matrix1(row, vector<float>(col, 0));
    vector<vector<float>> matrix2(row, vector<float>(col, 0));
    
    auto t_start = chrono::high_resolution_clock::now();

    initialize(row, col, matrix1);
    initialize(row, col, matrix2);

    while (true) {
        new_values(row, col, matrix2, matrix1);
        status1 = check_convergence(row, col, matrix2);
        iter++;
        if (status1 == 0)
            break;

        new_values(row, col, matrix1, matrix2);
        status2 = check_convergence(row, col, matrix1);
        iter++;
        if (status2 == 0)
            break;
    }

    for (int i = 0; i < row; i++){
        for (int j = 0; j < col; j++){
            if (matrix1[i][j] > 50.0f)
                hot_cells++;
        }
    }

    auto t_end = chrono::high_resolution_clock::now();
    double exec_time = chrono::duration<double>(t_end - t_start).count();

    cout << "N° Iteraciones: " << iter << endl;
    cout << "Tiempo de ejecucion: " << exec_time << " segundos" << endl;
    cout << "Num. de celdas calientes: " << hot_cells << endl;
    
    return 0;
}
