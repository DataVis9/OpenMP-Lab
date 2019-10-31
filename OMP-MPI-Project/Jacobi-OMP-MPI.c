//Jacobi - MPI + OpenMP.c

//Ian Spryn, Mitchell Harvey, Emily Wasylenko, Kaileigh MacLeod ~ COMP 322.A



#include <mpi.h>

#include <omp.h>

#include <stdio.h>

#include <math.h>



/*

Get current time

*/

#ifndef _TIMER_H

#define _TIMER_H_



#include <sys/time.h>



#define GET_TIME(now){ \

	struct timeval t; \

	gettimeofday(&t, NULL); \

	now = t.tv_sec + t.tv_usec/1000000.0; \

}



#endif



double* Jacobi(int maxRows, int maxCols, int initTemperature);

double DoOnePass(double* nodeArray, double* nodeArrayNext, int maxRows, int maxCols);

void SetCell(double *array, int maxCols, int row, int col, double f);

double GetCell(double *array, int maxCols, int row, int col);

void ToPPM(double* masterArray, double timeTaken, int maxRows, int maxCols);



MPI_Status status;



int i, j, rank, world_size;



int main() {

	const int maxRows = 1000;

	const int maxCols = 1500;

	const double epsilon = 0.001;

	int tempCold = 0; //cold blue

	int tempMedium = 128; //warm pruple

	int tempHot = 255; //hot red

	int rank, world_size;

	int nodeRows, extraRows, extraTop, extraBottom; //special cases for sharing top and bottom rows

	double masterMaxDiff, diff, startTime, endTime, timeTaken = 0;

	double *masterArray, *nodeArray, *nodeArrayExtraParts; //arrays for holding the Jacobi board



	MPI_Status status;

    	MPI_Init(NULL, NULL);

    	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    	MPI_Comm_size (MPI_COMM_WORLD, &world_size);



	nodeRows = maxRows / world_size;



	if (rank == 0) {

		GET_TIME(startTime);



		//Initialize the masterArray and set its values

		masterArray = Jacobi(maxRows, maxCols, tempCold);

		//right side

		for (i = 0; i < maxRows; i++) {

			SetCell(masterArray, maxCols, i, maxCols - 1, tempHot);

		}



		//top side

		for (j = 0; j < maxCols; j++) {

			SetCell(masterArray, maxCols, 0, j, tempHot);

		}



		for (i = 1; i < maxRows - 1; i++) {

			for (j = 1; j < maxCols - 1; j++) {

				SetCell(masterArray, maxCols, i, j, tempMedium);

			}

		}





	}



	//initialize each node's array

	nodeArray = (double*)malloc(nodeRows * maxCols * sizeof(double));



	//send chunks of the master array to all of the nodes

	MPI_Scatter(masterArray, (nodeRows * maxCols), MPI_DOUBLE, nodeArray, (nodeRows * maxCols), MPI_DOUBLE, 0, MPI_COMM_WORLD);



	//if we are not the node with the top part of the board

	if (rank > 0) {

		extraTop = 1;

	}

	//if we are not hte node with the bottom part of the board

	if (rank < world_size - 1) {

		extraBottom = 1;

	}

	extraRows = extraTop + extraBottom; //for middle node boards



	//recreate the node's array size with the special cases taken into consideration

	nodeArrayExtraParts = (double*)malloc((extraRows + nodeRows) * maxCols * sizeof(double));



	for (i = 0; i < nodeRows * maxCols; i++) {

		nodeArrayExtraParts[i + (extraTop * maxCols)] = nodeArray[i]; //set the new Array to the node's array, but with the correct padding on the top if necessary

	}

	free(nodeArray); //replace it with the extra parts

	nodeArray = nodeArrayExtraParts;

	nodeArrayExtraParts = NULL; //else we may get weird results



	masterMaxDiff = epsilon + 1;

	while (masterMaxDiff >= epsilon) {

		//only send to the node above me if I am NOT the top node

		if (rank > 0) {

			//send the next to first row of nodeArray to the above neighbor

			MPI_Send((nodeArray + maxCols), maxCols, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD);

		}

		//only send and receive to the node below me if I am NOT the bottom node

		if (rank < world_size - 1) {

			//Receive from the neighbor below

			MPI_Recv((nodeArray + ((nodeRows + extraTop) * maxCols)), maxCols, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &status);

			//send to the neighbor below

			MPI_Send((nodeArray + (nodeRows + extraTop - 1) * maxCols), maxCols, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD);

		}



		//only receive from above if I am NOT the top node

		if (rank > 0) {

			MPI_Recv(nodeArray, maxCols, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &status);

		}



		//create memory for the board's next generation

		double* nodeArrayNext = (double*)malloc((nodeRows + extraRows) * maxCols * sizeof(double));

		for (i = 0; i < (nodeRows + extraRows) * maxCols; i++) {

			nodeArrayNext[i] = nodeArray[i];

		}

		double diff = DoOnePass(nodeArray, nodeArrayNext, nodeRows + extraRows, maxCols);

		

		MPI_Allreduce(&diff, &masterMaxDiff, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);

		free(nodeArray); //remove old data

		nodeArray = nodeArrayNext;

		}



		//merge arrays back into the master array

		MPI_Gather((nodeArray + extraTop * maxCols), (nodeRows * maxCols), MPI_DOUBLE, masterArray, (nodeRows * maxCols), MPI_DOUBLE, 0, MPI_COMM_WORLD);



		if (rank == 0) {

			GET_TIME(endTime);

			timeTaken = endTime - startTime;

			ToPPM(masterArray, timeTaken, maxRows, maxCols);

		}



	MPI_Finalize();

	

	return 0;



}



//create declare and initialize a new array

double* Jacobi(int maxRows, int maxCols, int initTemperature) {

	double *array = (double*)malloc(maxRows * maxCols * sizeof(double));

	for (i = 0; i < maxRows * maxCols; i++) {

		array[i] = initTemperature;

	}

	return array;

}



//Generate the next generation of a board

double DoOnePass(double* nodeArray, double* nodeArrayNext, int maxRows, int maxCols) {



	double diff = 0;



#pragma omp parallel num_threads(4) shared(diff, nodeArray, nodeArrayNext, maxRows, maxCols) private(i, j)

{

	double average, localDiff, localMaxDiff = 0;

#pragma omp for schedule(dynamic, 1)

	for (i = 1; i < maxRows - 1; i++) {

		for (j = 1; j < maxCols - 1; j++) {

			average = (GetCell(nodeArray, maxCols, i, j - 1) + GetCell(nodeArray, maxCols, i, j + 1) + GetCell(nodeArray, maxCols, i - 1, j) + GetCell(nodeArray, maxCols, i + 1, j)) / 4.0f;

			localDiff = fabs(GetCell(nodeArray, maxCols, i, j) - average);

			if (localDiff > localMaxDiff) {

				localMaxDiff = localDiff;

			}

			SetCell(nodeArrayNext, maxCols, i, j, average);

		}

	}



#pragma omp critical

	{

		if (localMaxDiff > diff) {

			diff = localMaxDiff;

		}

	}

}

	return diff;

}



//set a cell in the array to a given value

void SetCell(double *array, int maxCols, int row, int col, double f) {

	array[row * maxCols + col] = f;

}



//get cell in the array

double GetCell(double *array, int maxCols, int row, int col) {

	return array[row * maxCols + col];

}



//Create a PPM file using all of the individual boards pieced together

void ToPPM(double* masterArray, double timeTaken, int maxRows, int maxCols) {

	printf("P3\n");

	printf("%d %d\n", maxCols, maxRows);

	printf("#Ian Spryn, Mitchell Harvey, Emily Wasylenko, Kaileigh MacLeod ~ COMP322.A MPP Capstone\n");

	printf("#Time taken: %f\n", timeTaken); //output the time taken

	printf("255\n");

	for (i = 0; i < maxRows * maxCols; i++) {

		int red = (int)ceil(masterArray[i]);

		int blue = 255 - (int)ceil(masterArray[i]);

		printf("%d 0 %d\n", red, blue);

	}

}