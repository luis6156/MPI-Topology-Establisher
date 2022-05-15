#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

#define NUM_CLUSTERS 3

void create_topology(int rank, int **adj, int sizes[], int *leader, int isError) {
    if (rank <= 2) {
        // Leaders
        FILE *fp;
        char file_source[128];

        // Leader is current rank
        *leader = rank;
    
        // Opens file
        sprintf(file_source, "cluster%d.txt", rank);
        fp = fopen(file_source, "r");
        if (fp == NULL) {
            perror("Error: opening cluster file\n");
        }

        // Read number of workers
        int num_workers, worker;
        fscanf(fp, "%d", &num_workers);
        if (num_workers < 0) {
            perror("Error: cluster file has negative number\n");
        }

        // Read workers
        sizes[rank] = num_workers;
        adj[rank] = (int*)calloc(num_workers, sizeof(int));

        for (int i = 0; i < num_workers; ++i) {
            fscanf(fp, "%d", &worker);
            if (worker < 0) {
                perror("Error: cluster file has negative number\n");
            }
            adj[rank][i] = worker;
        }

        // Send leader to workers
        for (int i = 0; i < num_workers; ++i) {
            MPI_Send(&rank, 1, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, adj[rank][i]);
        }

        // Send current cluster to other coordinators
        if (isError) {
            if (rank == 0 || rank == 1) {
                MPI_Send(&sizes[rank], 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 2);
                MPI_Send(adj[rank], sizes[rank], MPI_INT, 2, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 2);
            } else {
                MPI_Send(&sizes[rank], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 0);
                MPI_Send(adj[rank], sizes[rank], MPI_INT, 0, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 0);
                MPI_Send(&sizes[rank], 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 1);
                MPI_Send(adj[rank], sizes[rank], MPI_INT, 1, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 1);
            }
        } else {
            // Send directly to other coordinators
            for (int i = 0; i < NUM_CLUSTERS; ++i) {
                if (rank != i) {
                    MPI_Send(&sizes[rank], 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                    MPI_Send(adj[rank], sizes[rank], MPI_INT, i, 0, MPI_COMM_WORLD);
                    printf("M(%d,%d)\n", rank, i);
                }
            }
        }

        // Receive clusters from other coordinators
        MPI_Status status;
        if (isError) {
            if (rank == 0) {
                // Rank 0 receives data from rank 2, two times (corresponding from rank 2 & 1)
                MPI_Recv(&sizes[2], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
                adj[2] = (int*)calloc(sizes[2], sizeof(int));
                MPI_Recv(adj[2], sizes[2], MPI_INT, 2, 0, MPI_COMM_WORLD, &status);

                MPI_Recv(&sizes[1], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
                adj[1] = (int*)calloc(sizes[1], sizeof(int));
                MPI_Recv(adj[1], sizes[1], MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            } else if (rank == 2) {
                // Rank 2 receives data from the other coordinators
                MPI_Recv(&sizes[0], 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
                adj[0] = (int*)calloc(sizes[0], sizeof(int));
                MPI_Recv(adj[0], sizes[0], MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

                MPI_Recv(&sizes[1], 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
                adj[1] = (int*)calloc(sizes[1], sizeof(int));
                MPI_Recv(adj[1], sizes[1], MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

                // Sends the other coordinators data to each other
                MPI_Send(&sizes[0], 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 1);
                MPI_Send(adj[0], sizes[0], MPI_INT, 1, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 1);

                MPI_Send(&sizes[1], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 0);
                MPI_Send(adj[1], sizes[1], MPI_INT, 0, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, 0);
            } else {
                // Rank 1 receives data from rank 2, two times (corresponding from rank 2 & 0)
                MPI_Recv(&sizes[2], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
                adj[2] = (int*)calloc(sizes[2], sizeof(int));
                MPI_Recv(adj[2], sizes[2], MPI_INT, 2, 0, MPI_COMM_WORLD, &status);

                MPI_Recv(&sizes[0], 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
                adj[0] = (int*)calloc(sizes[0], sizeof(int));
                MPI_Recv(adj[0], sizes[0], MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            }
        } else {
            // Recv directly from other coordinators
            for (int i = 0; i < NUM_CLUSTERS; ++i) {
                if (rank != i) {
                    MPI_Recv(&sizes[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
                    adj[i] = (int*)calloc(sizes[i], sizeof(int));
                    MPI_Recv(adj[i], sizes[i], MPI_INT, i, 0, MPI_COMM_WORLD, &status);
                }
            }
        }
        
        // Coordinator prints the topology
        printf("%d ->", rank);
        for (int i = 0; i < NUM_CLUSTERS; ++i) {
            printf(" %d:", i);
            for (int j = 0; j < sizes[i]; ++j) {
                if (j == sizes[i] - 1) {
                    printf("%d", adj[i][j]);
                } else {
                    printf("%d,", adj[i][j]);
                }
            }
        }
        printf("\n");

        // Send topology to workers
        for (int i = 0; i < NUM_CLUSTERS; ++i) {
            for (int j = 0; j < sizes[rank]; ++j) {
                MPI_Send(&sizes[i], 1, MPI_INT, adj[rank][j], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, adj[rank][j]);
                MPI_Send(adj[i], sizes[i], MPI_INT, adj[rank][j], 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, adj[rank][j]);
            }
        }
        
        fclose(fp);
    } else {
        // Workers
        MPI_Status status;

        // Receive leader
        MPI_Recv(leader, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        // Receive topology
        for (int i = 0; i < NUM_CLUSTERS; ++i) {
            MPI_Recv(&sizes[i], 1, MPI_INT, *leader, 0, MPI_COMM_WORLD, &status);
            adj[i] = (int*)calloc(sizes[i], sizeof(int));
            MPI_Recv(adj[i], sizes[i], MPI_INT, *leader, 0, MPI_COMM_WORLD, &status);
        }

        // Worker prints topology
        printf("%d ->", rank);
        for (int i = 0; i < NUM_CLUSTERS; ++i) {
            printf(" %d:", i);
            for (int j = 0; j < sizes[i]; ++j) {
                if (j == sizes[i] - 1) {
                    printf("%d", adj[i][j]);
                } else {
                    printf("%d,", adj[i][j]);
                }
            }
        }
        printf("\n");
    }
}

void compute_array(int rank, int num_tasks, int **adj, int sizes[], int leader, int N, int isError) {
    int V[N];

    if (rank == 0) {
        MPI_Status status;

        // Compute initial vector
        for (int i = 0; i < N; ++i) {
            V[i] = i;
        }

        int num_workers = num_tasks - NUM_CLUSTERS;
        int elements_per_worker = N / num_workers;
        int start_local = 0;
        int isEquallyDivied = 1;

        // Check if tasks can be equally divided to all workers
        if (N % (num_tasks - NUM_CLUSTERS) != 0) {
            isEquallyDivied = 0;
        }

        // Send initial vector to other coordinators
        if (isError) {
            // Send only to rank 2 so that he can forward the vector
            MPI_Send(V, N, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
        } else {    
            // Send to other coordinators directly    
            for (int i = 1; i < NUM_CLUSTERS; ++i) {
                MPI_Send(V, N, MPI_INT, i, 0, MPI_COMM_WORLD);
                printf("M(%d,%d)\n", rank, i);
            }
        }

        // Send to 0's workers the initial vector as well as the start index
        // and the number of elements to compute
        for (int i = 0; i < sizes[rank]; ++i) {
            MPI_Send(V, N, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, adj[rank][i]);
            MPI_Send(&start_local, 1, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, adj[rank][i]);
            MPI_Send(&elements_per_worker, 1, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, adj[rank][i]);
            start_local += elements_per_worker;
        }

        // Receive updates from current workers & update vector
        int V_local[N];
        start_local = 0;
        for (int i = 0; i < sizes[rank]; ++i) {
            MPI_Recv(V_local, N, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD, &status);
            for (int j = start_local; j < start_local + elements_per_worker; ++j) {
                V[j] = V_local[j];
            }
            start_local += elements_per_worker;
        }

        // Receive updates from other coordinators & update vector
        int end;
        start_local = sizes[rank] * elements_per_worker;
        for (int i = 1; i < NUM_CLUSTERS; ++i) {
            if (isError) {
                MPI_Recv(V_local, N, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
            } else {
                MPI_Recv(V_local, N, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            }
            end = start_local + (sizes[i] * elements_per_worker);
            // Send more data to last worker if tasks cannot be equally divided
            if (!isEquallyDivied && i == 2) {
                end += (N % (num_tasks - NUM_CLUSTERS));
            }
            for (int j = start_local; j < end; ++j) {
                V[j] = V_local[j];
            }
            start_local += (sizes[i] * elements_per_worker);
        }
    } else if (rank <= 2) {
        MPI_Status status;
        int start_cluster = 0, start_local;
        int elements_per_worker = N / (num_tasks - NUM_CLUSTERS);
        int isEquallyDivied = 1;

        // Check if tasks can be equally divided to all workers
        if (rank == 2 && N % (num_tasks - NUM_CLUSTERS) != 0) {
            isEquallyDivied = 0;
        }

        // Receive vector from coordinator
        if (isError && rank == 1) {
            // Receive from rank 2 if error is ON and rank is 1
            MPI_Recv(V, N, MPI_INT, 2, 0, MPI_COMM_WORLD, &status);
        } else {
            // Receive directly from 0
            MPI_Recv(V, N, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        }

        // Send vector to coordinator 1 if connection is broken
        if (isError && rank == 2) {
            MPI_Send(V, N, MPI_INT, 1, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 1);
        }

        // Compute start index for current cluster
        for (int i = 0; i < rank; ++i) {
            start_cluster += (sizes[i] * elements_per_worker);
        }
        start_local = start_cluster;

        // Send to coordinator's workers
        for (int i = 0; i < sizes[rank]; ++i) {
            MPI_Send(V, N, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, adj[rank][i]);

            MPI_Send(&start_local, 1, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, adj[rank][i]);

            // If tasks cannot be equally divided, send to last worker more elements
            if (!isEquallyDivied && i == sizes[rank] - 1) {
                elements_per_worker += (N % (num_tasks - NUM_CLUSTERS));
                MPI_Send(&elements_per_worker, 1, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD);
                elements_per_worker -= (N % (num_tasks - NUM_CLUSTERS));
            } else {
                MPI_Send(&elements_per_worker, 1, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD);
            }
            printf("M(%d,%d)\n", rank, adj[rank][i]);

            start_local += elements_per_worker;
        }

        // Receive updates from current workers & update vector
        start_local = start_cluster;
        int V_local[N], end;
        for (int i = 0; i < sizes[rank]; ++i) {
            MPI_Recv(V_local, N, MPI_INT, adj[rank][i], 0, MPI_COMM_WORLD, &status);
            end = start_local + elements_per_worker;
            // If tasks cannot be equally divided, recv more elements from last workers
            if (!isEquallyDivied && i == sizes[rank] - 1) {
                end += (N % (num_tasks - NUM_CLUSTERS));
            }
            for (int j = start_local; j < end; ++j) {
                V[j] = V_local[j];
            }
            start_local += elements_per_worker;
        }

        // If connection is broken, receive partially solved vector from 1 and forward it to 0
        if (isError && rank == 2) {
            MPI_Recv(V_local, N, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
            MPI_Send(V_local, N, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        }

        // Sent updates to rank 0
        if (isError && rank == 1) {
            // Send to rank 2 to be forwarded
            MPI_Send(V, N, MPI_INT, 2, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 2);
        } else {
            // Send directly to rank 0
            MPI_Send(V, N, MPI_INT, 0, 0, MPI_COMM_WORLD);
            printf("M(%d,%d)\n", rank, 0);
        }
    } else {
        MPI_Status status;
        int start, num_of_elements;

        // Receive vector from coordinator
        MPI_Recv(V, N, MPI_INT, leader, 0, MPI_COMM_WORLD, &status);

        // Receive start/end index from coordinator
        MPI_Recv(&start, 1, MPI_INT, leader, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&num_of_elements, 1, MPI_INT, leader, 0, MPI_COMM_WORLD, &status);

        // Update vector
        for (int i = start; i < start + num_of_elements; ++i) {
            V[i] *= 2;
        }

        // Send to coordinator
        MPI_Send(V, N, MPI_INT, leader, 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", rank, leader);
    }

    // Used to stop STDOUT buffer overflow
    MPI_Barrier(MPI_COMM_WORLD);

    // Let rank 0 print the computed vector
    if (rank == 0) {
        printf("Rezultat: ");
        for (int i = 0; i < N; ++i) {
            printf("%d ", V[i]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    
    int N;
    int isError;
    int num_tasks, rank;

    // Sanity check
    if (argc < 2) {
        printf("Error: usage is ./tema3 <dimensiune_vector> <eroare_comunicatie>\n");
        return -1;
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    N = atoi(argv[1]);
    isError = atoi(argv[2]);

    int **adj = malloc(NUM_CLUSTERS * sizeof(int *)); 
    int sizes[NUM_CLUSTERS];
    int leader;
    
    // Create topology & compute array result
    create_topology(rank, adj, sizes, &leader, isError);
    compute_array(rank, num_tasks, adj, sizes, leader, N, isError);

    // Free topology
    for (int i = 0; i < NUM_CLUSTERS; ++i) {
        free(adj[i]);
    }
    free(adj);

    MPI_Finalize();
}