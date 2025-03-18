#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

vector<vector<int>> sequentialComponentLabeling(const vector<vector<int>>& image) {
    int rows = image.size();
    if (rows == 0) return {};
    int cols = image[0].size();

    // Initialize the label array:
    // For foreground pixels (1), assign a unique label (e.g., i * cols + j).
    // Background pixels (0) remain labeled as 0.
    vector<vector<int>> label(rows, vector<int>(cols, 0));
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (image[i][j] == 1) {
                label[i][j] = i * cols + j; // Unique initial label.
            }
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        // Create a copy of the label array to store updated labels.
        vector<vector<int>> newLabel = label;
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                // Process only the foreground pixels.
                if (image[i][j] == 1) {
                    int current = label[i][j];
                    int minLabel = current;
                    // Check the top neighbor.
                    if (i > 0 && image[i - 1][j] == 1)
                        minLabel = min(minLabel, label[i - 1][j]);
                    // Check the left neighbor.
                    if (j > 0 && image[i][j - 1] == 1)
                        minLabel = min(minLabel, label[i][j - 1]);
                    // Check the bottom neighbor.
                    if (i < rows - 1 && image[i + 1][j] == 1)
                        minLabel = min(minLabel, label[i + 1][j]);
                    // Check the right neighbor.
                    if (j < cols - 1 && image[i][j + 1] == 1)
                        minLabel = min(minLabel, label[i][j + 1]);

                    // If a smaller label is found, update it.
                    if (minLabel < current) {
                        newLabel[i][j] = minLabel;
                        changed = true;
                    }
                }
            }
        }
        // Update the label array.
        label = newLabel;
    }

    return label;
}

int main() {
    // Example binary image: 0 represents background; 1 represents a foreground pixel.
    vector<vector<int>> image = {
        {0, 1, 0, 0, 1},
        {1, 1, 0, 1, 1},
        {0, 0, 0, 0, 0},
        {1, 0, 1, 1, 1}
    };

    vector<vector<int>> result = sequentialComponentLabeling(image);

    // Output the final labeled image.
    cout << "Labeled Image:" << endl;
    for (const auto &row : result) {
        for (int label : row) {
            cout << label << "\t";
        }
        cout << "\n";
    }

    return 0;
}
