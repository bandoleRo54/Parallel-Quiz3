#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, numProcs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    // Global image dimensions.
    int globalRows, cols;
    vector<int> globalImage; // flattened global image (row-major)

    // Only rank 0 initializes the global image.
    if (rank == 0) {
        // Example image (4 rows x 5 columns):
        // { {0,1,0,0,1},
        //   {1,1,0,1,1},
        //   {0,0,0,0,0},
        //   {1,0,1,1,1} }
        globalRows = 4;
        cols = 5;
        globalImage = {
            0, 1, 0, 0, 1,
            1, 1, 0, 1, 1,
            0, 0, 0, 0, 0,
            1, 0, 1, 1, 1
        };
    }
    
    // Broadcast globalRows and cols so that all processes know the image dimensions.
    MPI_Bcast(&globalRows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Determine number of rows per process using block partitioning.
    int rowsPerProc = globalRows / numProcs;
    int remainder = globalRows % numProcs;
    // Processes with rank < remainder get one extra row.
    int localRows = (rank < remainder) ? rowsPerProc + 1 : rowsPerProc;

    // Compute send counts and displacements for scattering rows.
    vector<int> sendCounts(numProcs), displs(numProcs);
    if (rank == 0) {
        int offset = 0;
        for (int i = 0; i < numProcs; i++) {
            int rows_i = (i < remainder) ? rowsPerProc + 1 : rowsPerProc;
            sendCounts[i] = rows_i * cols;
            displs[i] = offset;
            offset += sendCounts[i];
        }
    }
    
    // Each process allocates storage for its subimage.
    vector<int> localImage(localRows * cols);
    // Allocate arrays for label and newLabel; these are stored in flattened row-major order.
    vector<int> localLabel(localRows * cols, 0);
    vector<int> localNewLabel(localRows * cols, 0);
    
    // Scatter the global image to all processes.
    MPI_Scatterv(globalImage.data(), sendCounts.data(), displs.data(), MPI_INT,
                 localImage.data(), localRows * cols, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Determine the global row index of the first local row.
    int startRow;
    if (rank < remainder) {
        startRow = rank * (rowsPerProc + 1);
    } else {
        startRow = remainder * (rowsPerProc + 1) + (rank - remainder) * rowsPerProc;
    }
    
    // Initialize the label array.
    // For a foreground pixel (value 1), the label is set to its global index: (globalRow * cols + col).
    for (int i = 0; i < localRows; i++) {
        for (int j = 0; j < cols; j++) {
            int idx = i * cols + j;
            if (localImage[idx] == 1) {
                int globalRow = startRow + i;
                localLabel[idx] = globalRow * cols + j;
            } else {
                localLabel[idx] = 0;
            }
        }
    }
    
    // Buffers for halo exchange (boundary rows).
    vector<int> recvTopHalo(cols, 0), recvBottomHalo(cols, 0);
    
    bool globalChanged = true;
    while (globalChanged) {
        bool localChanged = false;
        
        // Exchange boundary rows of localLabel with neighboring processes.
        // Exchange top boundary:
        if (rank > 0) {
            MPI_Sendrecv(&localLabel[0], cols, MPI_INT, rank - 1, 0,
                         recvTopHalo.data(), cols, MPI_INT, rank - 1, 1,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        // Exchange bottom boundary:
        if (rank < numProcs - 1) {
            MPI_Sendrecv(&localLabel[(localRows - 1) * cols], cols, MPI_INT, rank + 1, 1,
                         recvBottomHalo.data(), cols, MPI_INT, rank + 1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        // Update each pixel in the local subimage.
        for (int i = 0; i < localRows; i++) {
            for (int j = 0; j < cols; j++) {
                int idx = i * cols + j;
                if (localImage[idx] == 1) { // Process only foreground pixels.
                    int current = localLabel[idx];
                    int minLabel = current;
                    
                    // Check left neighbor (same row).
                    if (j > 0 && localImage[idx - 1] == 1)
                        minLabel = min(minLabel, localLabel[idx - 1]);
                    
                    // Check right neighbor (same row).
                    if (j < cols - 1 && localImage[idx + 1] == 1)
                        minLabel = min(minLabel, localLabel[idx + 1]);
                    
                    // Check top neighbor.
                    if (i == 0) {
                        // Use halo row from the upper neighbor.
                        if (rank > 0 && recvTopHalo[j] != 0)
                            minLabel = min(minLabel, recvTopHalo[j]);
                    } else {
                        if (localImage[idx - cols] == 1)
                            minLabel = min(minLabel, localLabel[idx - cols]);
                    }
                    
                    // Check bottom neighbor.
                    if (i == localRows - 1) {
                        // Use halo row from the lower neighbor.
                        if (rank < numProcs - 1 && recvBottomHalo[j] != 0)
                            minLabel = min(minLabel, recvBottomHalo[j]);
                    } else {
                        if (localImage[idx + cols] == 1)
                            minLabel = min(minLabel, localLabel[idx + cols]);
                    }
                    
                    localNewLabel[idx] = minLabel;
                    if (minLabel < current)
                        localChanged = true;
                } else {
                    localNewLabel[idx] = 0;
                }
            }
        }
        
        // Check globally if any process had an update.
        int localChangedInt = localChanged ? 1 : 0;
        int globalChangedInt = 0;
        MPI_Allreduce(&localChangedInt, &globalChangedInt, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
        globalChanged = (globalChangedInt != 0);
        
        // Copy the new labels into localLabel for the next iteration.
        localLabel = localNewLabel;
    }
    
    // Gather the labeled subimages back to rank 0.
    vector<int> globalLabel;
    if (rank == 0) {
        globalLabel.resize(globalRows * cols);
    }
    MPI_Gatherv(localLabel.data(), localRows * cols, MPI_INT,
                globalLabel.data(), sendCounts.data(), displs.data(), MPI_INT,
                0, MPI_COMM_WORLD);
    
    // Rank 0 prints the final labeled image.
    if (rank == 0) {
        cout << "Labeled Image:" << endl;
        for (int i = 0; i < globalRows; i++) {
            for (int j = 0; j < cols; j++) {
                cout << globalLabel[i * cols + j] << "\t";
            }
            cout << "\n";
        }
    }
    
    MPI_Finalize();
    return 0;
}
