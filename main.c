#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define ARRAY_SIZE ((long int)(12010060 + 12028670) / 2)
#define m 5
#define MAX_PROCESSES 10

double packet[ARRAY_SIZE];
double result_serial = 0.0;
double result_parallel = 0.0;

int main() {

    clock_t timer0Start, timer0End;
    double time0;

    timer0Start = clock();
    // Initialize packet array with random values between 0 and 1
    for (long int i = 0; i < ARRAY_SIZE; i++) {
        packet[i] = (double)rand() / (double)(RAND_MAX);
        result_serial += packet[i];
    }
    
    
    timer0End = clock();
    time0 = ((double)(timer0End - timer0Start)) / ARRAY_SIZE;

    // Create named pipes for IPC
    for (int i = 0; i < MAX_PROCESSES; i++) {
        char pipe_name[32];
        snprintf(pipe_name, sizeof(pipe_name), "fifo%d", i);
        mkfifo(pipe_name, 0666);
    }

    // Number of processes
    int M = 5;

    if (m == 0) {
        fprintf(stderr, "Error: packet length is zero.\n");
        exit(EXIT_FAILURE);
    }


    

    // Create child processes
    for (int i = 0; i < M; i++) {
        pid_t child_pid = fork();

        if (child_pid == 0) { // Child process
            double partial_sum = 0.0;
            int start = i * (ARRAY_SIZE / M); // Use ARRAY_SIZE here
            int end = (i + 1) * (ARRAY_SIZE / M); // Use ARRAY_SIZE here

            for (long int j = start; j < end; j++) {
                partial_sum += packet[j];
            }

            char pipe_name[32];
            snprintf(pipe_name, sizeof(pipe_name), "fifo%d", i);
            int fd = open(pipe_name, O_WRONLY);

            // Write the partial sum to the pipe
            write(fd, &partial_sum, sizeof(double));

            close(fd);
            exit(0);
        } else if (child_pid < 0) {
            perror("Fork failed");
            exit(1);
        }
    }

    

    clock_t timer1Start, timer1End;
    timer1Start = clock();

    // Parent process collects partial sums
    for (int i = 0; i < M; i++) {

        char pipe_name[32];
        snprintf(pipe_name, sizeof(pipe_name), "fifo%d", i);
        int fd = open(pipe_name, O_RDONLY);

        double partial_sum;
        read(fd, &partial_sum, sizeof(double));

        result_parallel += partial_sum;

        close(fd);
    }

    timer1End = clock();
    double time1 = ((double)(timer1End - timer1Start)) / CLOCKS_PER_SEC; // Convert to seconds

    double tolerance = 1e-6; // Define an appropriate tolerance
    if ((result_serial - result_parallel) < tolerance) {
        printf("Results match: Serial = %.6f, Parallel = %.6f\n", result_serial, result_parallel);
        fflush(stdout);
    } else {
        printf("Results do not match!\n Serial = %.6f, Parallel = %.6f\n", result_serial, result_parallel);
        fflush(stdout);
    }

    printf("\nSerial time : %f\n", time0);
    fflush(stdout);

    printf("Parallel time : %f\n", time1);
    fflush(stdout);

    return 0;
}
