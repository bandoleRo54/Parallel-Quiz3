#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <omp.h>

using namespace std;

void initialize(int row, int col, vector<vector<float>> &mat) {
    for (int i = 0; i < row; i++){
        for (int j = 0; j < col; j++){
            if (i == 0 || j == 0 || j == col - 1) {
                mat[i][j] = 0.0f; // cold boundary
            } else if (i == row - 1) {
                mat[i][j] = 100.0f; // hot border
            } else if (i == 400 || j < 500) { // hot region condition (per your code)
                mat[i][j] = 100.0f;
            } else if (i == 512 && j == 512) {
                mat[i][j] = 100.0f; // hot center
            } else {
                mat[i][j] = 0.0f;
            }
        }
    }
}

int main(int argc, char* argv[]){
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <rows> <cols>" << endl;
        return 1;
    }

    int row = atoi(argv[1]);
    int col = atoi(argv[2]);
    int iter = 0;

    // Create two matrices with the given number of rows and columns.
    vector<vector<float>> matrix1(row, vector<float>(col, 0));
    vector<vector<float>> matrix2(row, vector<float>(col, 0));

    // Initialize both matrices.
    initialize(row, col, matrix1);
    initialize(row, col, matrix2);

    auto t_start = chrono::high_resolution_clock::now();

    while (true) {
        // First sweep: update matrix2 from matrix1
        #pragma omp parallel for collapse(2) schedule(static)
        for (int i = 1; i < row - 1; i++){
            for (int j = 1; j < col - 1; j++){
                // Skip fixed hot cells.
                if ((i == 500 && j < 400) || (i == 512 && j == 512))
                    continue;
                matrix2[i][j] = (matrix1[i+1][j] + matrix1[i-1][j] +
                                 matrix1[i][j+1] + matrix1[i][j-1] +
                                 4 * matrix1[i][j]) / 8.0f;
            }
        }
        // Implicit barrier at end of parallel region

        // Convergence check for matrix2
        int conv_flag = 0;
        #pragma omp parallel for collapse(2) reduction(+:conv_flag) schedule(static)
        for (int i = 1; i < row - 1; i++){
            for (int j = 1; j < col - 1; j++){
                if ((i == 500 && j < 400) || (i == 512 && j == 512))
                    continue;
                float conv = matrix2[i][j] - ((matrix2[i+1][j] + matrix2[i-1][j] +
                                               matrix2[i][j+1] + matrix2[i][j-1]) / 4.0f);
                if (fabs(conv) > 0.1f)
                    conv_flag++;
            }
        }
        if (conv_flag == 0)
            break;

        // Second sweep: update matrix1 from matrix2
        #pragma omp parallel for collapse(2) schedule(static)
        for (int i = 1; i < row - 1; i++){
            for (int j = 1; j < col - 1; j++){
                if ((i == 500 && j < 400) || (i == 512 && j == 512))
                    continue;
                matrix1[i][j] = (matrix2[i+1][j] + matrix2[i-1][j] +
                                 matrix2[i][j+1] + matrix2[i][j-1] +
                                 4 * matrix2[i][j]) / 8.0f;
            }
        }

        // Convergence check for matrix1
        conv_flag = 0;
        #pragma omp parallel for collapse(2) reduction(+:conv_flag) schedule(static)
        for (int i = 1; i < row - 1; i++){
            for (int j = 1; j < col - 1; j++){
                if ((i == 500 && j < 400) || (i == 512 && j == 512))
                    continue;
                float conv = matrix1[i][j] - ((matrix1[i+1][j] + matrix1[i-1][j] +
                                               matrix1[i][j+1] + matrix1[i][j-1]) / 4.0f);
                if (fabs(conv) > 0.1f)
                    conv_flag++;
            }
        }
        if (conv_flag == 0)
            break;

        iter++; // One full cycle (both sweeps)
    }

    auto t_end = chrono::high_resolution_clock::now();
    double exec_time = chrono::duration<double>(t_end - t_start).count();

    // Count hot cells (using parallel reduction)
    int hot_cells = 0;
    #pragma omp parallel for collapse(2) reduction(+:hot_cells) schedule(static)
    for (int i = 0; i < row; i++){
        for (int j = 0; j < col; j++){
            if (matrix1[i][j] > 50.0f)
                hot_cells++;
        }
    }

    cout << "N° Iteraciones: " << iter << endl;
    cout << "Tiempo de ejecucion: " << exec_time << " segundos" << endl;
    cout << "Num. de celdas calientes: " << hot_cells << endl;

    return 0;
}
