#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
using namespace std;

int main(int argc, char* argv[]){
	srand(time(0));
	int rows = atoi(argv[1]);
	int cols = atoi(argv[2]);
	const char *funct_to_run = argv[3];
	int nthreads = atoi(argv[4]);
	string run;
	int num;

	ostringstream cmd;

	if (strcmp(funct_to_run, "seq") == 0) {
		run = "./run_seq";
	} else if (strcmp(funct_to_run, "pthr") == 0) {
		run = "./run_pthr";
	} else if (strcmp(funct_to_run, "omp") == 0) {
		run = "./run_omp";
	} else if (strcmp(funct_to_run, "mpi") == 0) {
		run = "./run_mpi";
	}

	if (strcmp(funct_to_run, "mpi") != 0) {
		cmd << run << " " << rows << " " << cols << " ";
	} else {
		cmd << "mpirun -n " << nthreads << " " << rows << " " << cols << " ";
	}

	int i, j;
	cout << "Matrix (" << rows << "x" << cols << "):" << endl;
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			num = rand() % 100; // Random number between 0-99
			cmd << num << " ";
			cout << num << " ";
        	}
		cout << endl;
	}
	cout << endl;

	cout << "Vector (" << cols << "):" << endl;
	for (i = 0; i < cols; i++) {
		num = rand() % 10;
		cmd << num << " ";
		cout << num << " ";
	}
	cout << endl << endl;

	string command = cmd.str();
	system(command.c_str());

	return 0;
}
