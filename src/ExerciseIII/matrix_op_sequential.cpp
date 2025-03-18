#include <iostream>
#include <cstdlib>
#include <vector>
using namespace std;

int main(int argc, char *argv[]) {
    // Check if enough arguments are provided
    if (argc < 5) {
        cerr << "Usage: " << argv[0] 
             << " rows1 cols1 rows2 cols2 <matrix1 elements> <matrix2 elements>" << endl;
        return 1;
    }
    
    // Parse dimensions from command-line arguments
    int r1 = atoi(argv[1]);
    int c1 = atoi(argv[2]);
    int r2 = atoi(argv[3]);
    int c2 = atoi(argv[4]);
    
    // Calculate the expected total number of arguments:
    // 1 (program name) + 4 (dimensions) + (r1*c1) + (r2*c2) matrix elements.
    int expectedArgs = 1 + 4 + (r1 * c1) + (r2 * c2);
    if (argc != expectedArgs) {
        cerr << "Error: Expected " << (expectedArgs - 1)
             << " arguments but got " << (argc - 1) << endl;
        return 1;
    }
    
    // Read Matrix 1 elements into a 2D vector
    vector<vector<int>> mat1(r1, vector<int>(c1));
    int index = 5;
    for (int i = 0; i < r1; i++) {
        for (int j = 0; j < c1; j++) {
            mat1[i][j] = atoi(argv[index++]);
        }
    }
    
    // Read Matrix 2 elements into a 2D vector
    vector<vector<int>> mat2(r2, vector<int>(c2));
    for (int i = 0; i < r2; i++) {
        for (int j = 0; j < c2; j++) {
            mat2[i][j] = atoi(argv[index++]);
        }
    }
    
    // Matrix addition is defined only if both matrices have the same dimensions.
    if (r1 == r2 && c1 == c2) {
        vector<vector<int>> sum(r1, vector<int>(c1, 0));
        for (int i = 0; i < r1; i++){
            for (int j = 0; j < c1; j++){
                sum[i][j] = mat1[i][j] + mat2[i][j];
            }
        }
        cout << "Sum of Matrix 1 and Matrix 2:" << endl;
        for (int i = 0; i < r1; i++){
            for (int j = 0; j < c1; j++){
                cout << sum[i][j] << " ";
            }
            cout << endl;
        }
        cout << endl;
    } else {
        cout << "Matrix addition is not possible due to different dimensions." << endl << endl;
    }
    
    // Matrix multiplication is defined if the number of columns in Matrix 1 equals the number of rows in Matrix 2.
    if (c1 == r2) {
        vector<vector<int>> product(r1, vector<int>(c2, 0));
        for (int i = 0; i < r1; i++){
            for (int j = 0; j < c2; j++){
                for (int k = 0; k < c1; k++){
                    product[i][j] += mat1[i][k] * mat2[k][j];
                }
            }
        }
        cout << "Product of Matrix 1 and Matrix 2:" << endl;
        for (int i = 0; i < r1; i++){
            for (int j = 0; j < c2; j++){
                cout << product[i][j] << " ";
            }
            cout << endl;
        }
        cout << endl;
    } else {
        cout << "Matrix multiplication is not possible (columns in Matrix 1 must equal rows in Matrix 2)." 
             << endl << endl;
    }
    
    // Compute and display the transpose of Matrix 1
    vector<vector<int>> transpose1(c1, vector<int>(r1, 0));
    for (int i = 0; i < r1; i++){
        for (int j = 0; j < c1; j++){
            transpose1[j][i] = mat1[i][j];
        }
    }
    cout << "Transpose of Matrix 1:" << endl;
    for (int i = 0; i < c1; i++){
        for (int j = 0; j < r1; j++){
            cout << transpose1[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
    
    // Compute and display the transpose of Matrix 2
    vector<vector<int>> transpose2(c2, vector<int>(r2, 0));
    for (int i = 0; i < r2; i++){
        for (int j = 0; j < c2; j++){
            transpose2[j][i] = mat2[i][j];
        }
    }
    cout << "Transpose of Matrix 2:" << endl;
    for (int i = 0; i < c2; i++){
        for (int j = 0; j < r2; j++){
            cout << transpose2[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
    
    return 0;
}
