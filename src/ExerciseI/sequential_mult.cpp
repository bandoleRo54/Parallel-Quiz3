#include <iostream>
#include <cstdlib>
#include <string>
using namespace std;

int main(int argc, char* argv[]){
	int AN = atoi(argv[1]);
	int AM = atoi(argv[2]);

	int i, j;
	int A[AN][AM];
	int v[AM];
	
	int arg_index = 3;
	for (i = 0; i < AN; i++){
		for (j = 0; j < AM; j++) {
			A[i][j] = atoi(argv[arg_index++]);
		}
	}
	for (i = 0; i < AM; i++){ 
		v[i] = atoi(argv[arg_index++]);
	}

	int Ax[AN] = {0};
	
	for (i = 0; i < AN; i++){
		for (j = 0; j < AM; j++) {
			Ax[i] += A[i][j] * v[j];
		}
	}

	cout << "Result (size " << AN << "):" << endl
	     << "{ ";
	for (i = 0; i < AN; i++){
		cout << Ax[i] << " ";
	}
	cout << "}" << endl;

	return 0;
}
