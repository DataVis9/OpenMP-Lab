//Texas-MPI-OMP.c
// COMP322.A ~ Texas Hold'Em Driver for OMP & MPI
// Team F: Ian Spryn, Emily Wasylenko, Mitchell Harvey
// This function written by Emily Wasylenko




#include <stdio.h>

#include <stdlib.h>

#include "POKER_HAND_ADT.h"

#include <omp.h>

#include <mpi.h>

using namespace std;





//member variables

int source, myRank, nodeSize = 0;





/************************************************

 ** This is a driver for our Poker Hand ADT.

 ** It generates every possible combination of

 ** 7 cards and prints the probability of each

 ** of the 10 poker hands in Texas Hold'Em. 

 ************************************************/

int main() {



    printf("*** Calculate Probability for all 10 Texas Hold'Em Hands ***\n\n");



    MPI_Init(NULL, NULL);

    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    MPI_Comm_size(MPI_COMM_WORLD, &nodeSize);



    int i = 0;



    //1.0 Initialize everything!

    // int threadNum = 4;

    // omp_set_num_threads(threadNum);



    //1.1 init poker hand ADT struct variables

    const int numCards = 7;

    struct PokerHand pkh;	//Poker hand object

    pkh.num = numCards;

    pkh.hand = new int[pkh.num];



    //1.2 init. probability calculators

    int totalRuns = 0;



    //1.3 init ready var and slave counters

    int readyInt = 0;
    
    int frequencyCounters[10] = {0,0,0,0,0,0,0,0,0,0};




    //1.4 init reduce counters and total runs counter 
    // (MPI Reduce adds the total of each hand frequency into its respective reduce counter)
    
    int reduceCounters[10] = {0,0,0,0,0,0,0,0,0,0};

    int totalTotalRuns = 0;



    //1.5 initialize loop variables

    int a, b, c, d, e, f, g;

    a = 1;



    //2.0 Run Loops

    //Master does outermost loop...

    //sends work to the ancillaries...



    //If(you're the master node) {

    if (myRank == 0) {

        //	Run a = 0 -> 46

        //	Send a to the slaves

        for (a = 0; a < 46; a++) {

            //MPI_Recv(void* data,int count,MPI_Datatype datatype,int source,int tag,MPI_Comm communicator,MPI_Status* status)

            MPI_Recv(&readyInt, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            printf("Received from: %d\n", readyInt);

            //MPI_Send(void* data,int count,MPI_Datatype datatype, int destination,int tag, MPI_Comm communicator)

            MPI_Send(&a, 1, MPI_INT, readyInt, 0, MPI_COMM_WORLD);

        }



        //start for loop to close down ancillary processes

        for(i = 1; i<nodeSize; i++){

            a = -1;

            MPI_Recv(&readyInt, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            printf("Going to tell node %d to finalize.\n", readyInt);

            MPI_Send(&a, 1, MPI_INT, readyInt, 0, MPI_COMM_WORLD);

        }



    } else {



        while(a >= 0){

            readyInt = myRank;

            MPI_Send(&readyInt, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

            MPI_Recv(&a, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            printf("Node %d received a value: %d\n", myRank, a);

            if (a < 0) {

                break;

            }

#pragma omp parallel

            {

            const int numCards = 7;

            struct PokerHand pkh;	//Poker hand object

            pkh.num = numCards;

            pkh.hand = new int[pkh.num];



            for (int n = 0; n < numCards; n++) {

                pkh.hand[n] = 0;

            }



            //	  Run 7 inner for loops

#pragma omp for schedule(dynamic,1) \

            reduction(+:totalRuns, frequencyCounters[0], frequencyCounters[1], frequencyCounters[2], frequencyCounters[3], frequencyCounters[4], frequencyCounters[5], frequencyCounters[6], frequencyCounters[7], frequencyCounters[8], frequencyCounters[9])



                for (b = a + 1; b < 47; b++) {



                    for (c = b + 1; c < 48; c++) {



                        for (d = c + 1; d < 49; d++) {



                            for (e = d + 1; e < 50; e++) {



                                for (f = e + 1; f < 51; f++) {



                                    for (g = f + 1; g < 52; g++) {		//card # 52 inclusive



                                        totalRuns++;



                                        pkh.hand[0] = a + 1;

                                        pkh.hand[1] = b + 1;

                                        pkh.hand[2] = c + 1;

                                        pkh.hand[3] = d + 1;

                                        pkh.hand[4] = e + 1;

                                        pkh.hand[5] = f + 1;

                                        pkh.hand[6] = g + 1;



                                        //3.0 Test all 10 functions, increment the counter for each function!



                                        if (royalFlush(pkh)) {

                                            frequencyCounters[0]++;

                                        }

                                        else if (isStraightFlush(pkh)) {

                                            frequencyCounters[1]++;

                                        }

                                        else if (isFourOfAKind(pkh)) {

                                            frequencyCounters[2]++;

                                        }

                                        else if (isFullHouse(pkh)) {

                                            frequencyCounters[3]++;

                                        }

                                        else if (isFlush(pkh)) {

                                            frequencyCounters[4]++;

                                        }

                                        else if (isStraight(pkh)) {

                                            frequencyCounters[5]++;

                                        }

                                        else if (threeOfKind(pkh)) {

                                            frequencyCounters[6]++;

                                        }

                                        else if (two_pair(pkh)) {

                                            frequencyCounters[7]++;

                                        }

                                        else if (isOnePair(pkh)) {

                                            frequencyCounters[8]++;

                                        }

                                        else if (isHighCard(pkh)) {

                                            frequencyCounters[9]++;

                                        }

                                    }

                                }

                            }

                        }

                    }

                }

            }

        }

    }



    //REDUCE

    printf("node %d: %d\n", myRank, frequencyCounters[0]);
    
    for(int i = 0; i<10; i++){
	MPI_Reduce(&frequencyCounters[i], &reduceCounters[i], nodeSize, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    }

    MPI_Reduce(&totalRuns, &totalTotalRuns, nodeSize, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);





    if (myRank == 0) {

        //4.0 Print the probability of each hand

	for(int j = 0; j < 10; j++){
		printf("Cntr %d%d:\n", j, reduceCounters[j]);
	}


        printf("Probability of getting Royal Flush:     %2.4f%%\n", 100.0 * (double) reduceCounters[0] / (double) totalTotalRuns);

        printf("Probability of getting Straight Flush:  %2.4f%%\n", 100.0 * (double) reduceCounters[1] / (double) totalTotalRuns);

        printf("Probability of getting Four Of A Kind:  %2.4f%%\n", 100.0 * (double) reduceCounters[2] / (double) totalTotalRuns);

        printf("Probability of getting Full House:      %2.4f%%\n", 100.0 * (double) reduceCounters[3] / (double) totalTotalRuns);

        printf("Probability of getting Flush:           %2.4f%%\n", 100.0 * (double) reduceCounters[4] / (double) totalTotalRuns);

        printf("Probability of getting Straight:        %2.4f%%\n", 100.0 * (double) reduceCounters[5] / (double) totalTotalRuns);

        printf("Probability of getting Three of a Kind: %2.4f%%\n", 100.0 * (double) reduceCounters[6] / (double) totalTotalRuns);

        printf("Probability of getting Two Pairs:       %2.4f%%\n", 100.0 * (double) reduceCounters[7] / (double) totalTotalRuns);

        printf("Probability of getting One Pair:        %2.4f%%\n", 100.0 * (double) reduceCounters[8] / (double) totalTotalRuns);

        printf("Probability of getting the High Card:   %2.4f%%\n", 100.0 * (double) reduceCounters[9] / (double) totalTotalRuns);

    }





    MPI_Finalize();	  

    return 0;

}