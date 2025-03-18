#include <iostream>
#include <vector>
#include <algorithm>
#include <omp.h>

using namespace std;

void componentLabeling(const vector<vector<int>> &image) {
    int rows = image.size();
    if (rows == 0) return;
    int cols = image[0].size();

    // Initialize label array:
    // For 1 pixels, assign a unique label (here: i*cols + j).
    // For 0 pixels, label remains 0.
    vector<vector<int>> label(rows, vector<int>(cols, 0));
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (image[i][j] == 1) {
                label[i][j] = i * cols + j;  // unique initial label
            }
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        // Create a copy of label for synchronous updating.
        vector<vector<int>> newLabel = label;
        int changeFlag = 0;  // used for parallel reduction

        // Parallel update: each thread processes part of the image.
        #pragma omp parallel for reduction(||:changeFlag)
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                // Only update if this is a foreground pixel.
                if (image[i][j] == 1) {
                    int current = label[i][j];
                    int minLabel = current;
                    // Check top neighbor.
                    if (i > 0 && image[i - 1][j] == 1)
                        minLabel = min(minLabel, label[i - 1][j]);
                    // Check left neighbor.
                    if (j > 0 && image[i][j - 1] == 1)
                        minLabel = min(minLabel, label[i][j - 1]);
                    // Check bottom neighbor.
                    if (i < rows - 1 && image[i + 1][j] == 1)
                        minLabel = min(minLabel, label[i + 1][j]);
                    // Check right neighbor.
                    if (j < cols - 1 && image[i][j + 1] == 1)
                        minLabel = min(minLabel, label[i][j + 1]);

                    // If a smaller label was found, update the new label.
                    if (minLabel < current) {
                        newLabel[i][j] = minLabel;
                        changeFlag = 1;
                    }
                }
            }
        }
        // Copy newLabel back into label.
        label = newLabel;
        changed = (changeFlag != 0);
    }

    // Output the final label array.
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            cout << label[i][j] << "\t";
        }
        cout << "\n";
    }
}

int main() {
    // Example binary image: 0 represents background; 1 represents a pixel to be labeled.
    vector<vector<int>> image = {
        {0, 1, 0, 0, 1},
        {1, 1, 0, 1, 1},
        {0, 0, 0, 0, 0},
        {1, 0, 1, 1, 1}
    };

    componentLabeling(image);
    return 0;
}
