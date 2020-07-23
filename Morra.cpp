// Game of Morra implemented using MSMPI in C++
//
// Author:	Logan D'Auria
// Contact:	lxd1644@rit.edu
//

#include "mpi.h"
#include <iostream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <vector>
#include <cmath>
#include <chrono>

using namespace std;

// Executes MPI Threads to play the game of Morra. Games will repeat based on
// program argument for amount of rounds played. Process actions are separated
// based on rank into one coordinator process and other player processes
int main(int argc, char * argv[]) {

	//auto start = chrono::high_resolution_clock::now();

	const int tag = 5;
	int run,				//number of rounds to play
		numtasks,			//number of processes running
		rank,				//rank of a process
		total = 0,			//tracks the total
		wins = 0,			//tracks the wins for each process/player 
		betterthan = 0;		//tracks how many other players a player beats
	int msg[3];
	MPI_Status status;

	int rc = MPI_Init(&argc, &argv);
	if (rc != MPI_SUCCESS) {
		printf("Error starting MPI Program, Abort mission D:\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
	}

	run = stoi(argv[1]);

	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// Checks for more than 1 participant
	if (numtasks < 2) {
		printf("You need at least 2 participants to run this program\n");
		MPI_Finalize();
		return 0;
	}

	// Completes rounds based on desired amount
	for (int i = 0; i < run; i++) {

		if (rank == 0) { // reciever process
			int* guess = new int[numtasks];

			// RECIEVES EXTENDED FINGER AMOUNTS AND GUESSES
			for (int x = 1; x < numtasks; x++) {
				MPI_Recv(msg, 3, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
				//source_id = status.MPI_SOURCE;
				total += msg[1];
				guess[msg[0]] = msg[2];
			}

			int diff = 0, winnerDiff = 9999;
			vector<int> winners;
			// CREATES A LIST OF THE WINNER(S)
			for (int x = 1; x < numtasks; x++) { // x for rank
				diff = abs(guess[x] - total);
				if (diff < winnerDiff) {
					winners.clear();
					winners.push_back(x);
					winnerDiff = diff;
				}
				else if (diff == winnerDiff) {
					winners.push_back(x);
				}
			}
			delete[] guess;

			// PRINTS WINNERS
			if (winners.size() > 1) {
				for (int x = 0; x < winners.size(); x++) {
					cout << "I am " << winners.at(x) << " and I almost won the run " << i << "." << endl;
				}
			}
			else {
				cout << "I am " << winners.back() << " and I won the run " << i << "." << endl;
			}


			// TELLS WHICH PROCESSES WON
			for (int x = 1; x < numtasks; x++) {
				int winmsg[] = { 1 };
				int losemsg[] = { 0 };
				if (x == winners.back() && winners.size() == 1)
					MPI_Send(winmsg, 1, MPI_INT, x, tag, MPI_COMM_WORLD);
				else
					MPI_Send(losemsg, 1, MPI_INT, x, tag, MPI_COMM_WORLD);
			}
		}
		// Player process
		else {
			srand(time(NULL) + rank + i);
			int fingers = 1 + rand() % 5; //number of fingers player extends
			int guess = (numtasks - 2) * (1 + rand() % 5) + fingers; //optimized guess of the total
			cout << "I am " << rank << ". For run " << i << ", I extend " << fingers
				<< " fingers. My guess is " << guess << "." << endl;
			msg[0] = rank;
			msg[1] = fingers;
			msg[2] = guess;
			MPI_Send(msg, 3, MPI_INT, 0 /*destination id*/, tag, MPI_COMM_WORLD); // sends info to recieving process
			MPI_Recv(msg, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
			wins += msg[0];
		}
	}

	// Each player sends and recieves win amounts to/from the other players to determine
	// how many players they outperformed.
	if (rank != 0) {
		MPI_Request request;
		int tag2 = 10;
		for (int x = 1; x < numtasks; x++) {
			if (x != rank) {
				int msg2[] = { wins };
				MPI_Send(msg2, 1, MPI_INT, x, tag2, MPI_COMM_WORLD);
			}
		}
		for (int x = 2; x < numtasks; x++) {
			int msg3[1];
			MPI_Recv(msg3, 1, MPI_INT, MPI_ANY_SOURCE, tag2, MPI_COMM_WORLD, &status);
			if (wins > msg3[0])
				betterthan++;
		}

		cout << "Hey! I am " << rank << " and I won " << wins
			<< " time(s). Overall, I played better than " << betterthan << " player(s)." << endl;
	}
	MPI_Finalize();

	//auto finish = std::chrono::high_resolution_clock::now();
	//chrono::duration<double> elapsed = finish - start;
	//cout << "Elapsed time: " << elapsed.count() << " s\n";

	return 0;
}