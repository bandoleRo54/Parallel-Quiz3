#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
using namespace std;

int main(int argc, char* argv[]){
	srand(time(0));
	int rows1 = atoi(argv[1]);
	int cols1 = atoi(argv[2]);
	int rows2 = atoi(argv[3]);
	int cols2 = atoi(argv[4]);
	const char *funct_to_run = argv[5];
	int nthreads = atoi(argv[6]);
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
		cmd << run << " " << rows1 << " " << cols1 << " "
		    	   	  << rows2 << " " << cols2 << " ";
	} else {
		cmd << "mpirun -n " << nthreads << " " << run << " " << rows1 << " " << cols1 << " "
								     << rows2 << " " << cols2 << " ";
	}

	int i, j;
	cout << "Matrix 1 (" << rows1 << "x" << cols1 << "):" << endl;
	for (i = 0; i < rows1; i++) {
		for (j = 0; j < cols1; j++) {
			num = rand() % 100; // Random number between 0-99
			cmd << num << " ";
			cout << num << " ";
        	}
		cout << endl;
	}
	cout << endl;

	cout << "Matrix 2 (" << rows2 << "x" << cols2 << "):" << endl;
	for (i = 0; i < rows2; i++) {
		for (j = 0; j < cols2; j++) {
			num = rand() % 100; // Random number between 0-99
			cmd << num << " ";
			cout << num << " ";
        	}
		cout << endl;
	}
	cout << endl;

	string command = cmd.str();
	system(command.c_str());

	return 0;
}
