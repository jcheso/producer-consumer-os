#include <array>
#include <memory>

#include "helper.h"

// ** Declare functions for producer and consumer **
void *producer(void *args);
void *consumer(void *args);

// ** Initialise global variables **
int const TIME_OUT = 20;
int semArray = sem_create(SEM_KEY, 3);
int front = 0;
int rear = 0;

// ** Struct for passing variables into producers/consumers **
struct ThreadArguments {
    int num_jobs = 0;
    int thread_id = 0;
    int *queue;
    int queue_size = 0;
    ThreadArguments(int num_jobs, int thread_id, int *queue, int queue_size) : num_jobs(num_jobs), thread_id(thread_id), queue(queue), queue_size(queue_size) {}
};

int main(int argc, char **argv) {
    // ** READ IN ARGUMENTS **
    if (argc < 5) {
        cerr << "Not enough arguments" << endl;
        return EXIT_FAILURE;
    }

    int queue_size = check_arg(argv[1]);
    int num_jobs = check_arg(argv[2]);
    int num_producers = check_arg(argv[3]);
    int num_consumers = check_arg(argv[4]);

    cout << "-------------------------------" << endl;
    cout << "Queue Size: " << queue_size << endl;
    cout << "Number of Jobs per Producer: " << num_jobs << endl;
    cout << "Number of Producers: " << num_producers << endl;
    cout << "Number of Consumers: " << num_consumers << endl;
    cout << "-------------------------------" << endl;

    // ** INITIALISE ARRAYS TO STORE THREADS **
    pthread_t producerid[num_producers];
    pthread_t consumerid[num_consumers];

    // **INITIALISE SEMAPHORES**
    // Mutual Exclusion Binary Semaphore
    if (sem_init(semArray, 0, 1) < 0)
        return EXIT_FAILURE;
    // Semphore to check if queue has space
    if (sem_init(semArray, 1, queue_size) < 0)
        return EXIT_FAILURE;
    // Semaphore to check if buffer is not empty
    if (sem_init(semArray, 2, 0) < 0)
        return EXIT_FAILURE;

    // ** INITIALISE DATA STRUCTURES **
    // Randomise the random number seed for each run of the program
    srand(time(NULL));
    int queue[queue_size];

    // Create Producer Threads
    for (int i = 0; i < num_producers; i++) {
        ThreadArguments *argument = new ThreadArguments(num_jobs, i, queue, queue_size);
        if (pthread_create(&producerid[i], NULL, producer, (void *)argument) != 0)
            perror("Failed to create producer thread");
    }

    // Create Consumer Threads
    for (int i = 0; i < num_consumers; i++) {
        ThreadArguments *argument = new ThreadArguments(num_jobs, i, queue, queue_size);
        if (pthread_create(&consumerid[i], NULL, consumer, (void *)argument) != 0)
            perror("Failed to create consumer thread");
    }

    // Join Producer Threads
    for (int i = 0; i < num_producers; i++) {
        if (pthread_join(producerid[i], NULL) != 0)
            perror("Failed to join producer thread");
    }

    // Join Consumer Threads
    for (int i = 0; i < num_consumers; i++) {
        if (pthread_join(consumerid[i], NULL) != 0)
            perror("Failed to join consumer thread");
    }

    // Destroy semaphores
    sem_close(semArray);

    return 0;
}

void *producer(void *args) {
    // Assign variables to the argument passed in
    ThreadArguments *prod_args = (ThreadArguments *)args;
    int *queue = prod_args->queue;
    int queue_size = prod_args->queue_size;
    int thread_id = prod_args->thread_id;

    // Iterate through and create the number of jobs required per a producer
    for (int i = 0; i < prod_args->num_jobs; i++) {
        // Generate random duration for each job between 1 – 10 seconds.
        int job_duration = rand() % 10;

        // Sleep a 1-5 seconds before adding each job to the queue.
        int delay = rand() % 5;
        sleep(delay);

        // If the circular queue is full, block while waiting for an empty slot and if a slot doesn’t become available after 20 seconds, quit.
        if (sem_wait(semArray, 1, TIME_OUT) < 0) {
            cout << "Producer("
                 << thread_id
                 << "): No more jobs to generate." << endl;
            break;
        };

        // Mutual Exclusion to Queue
        sem_wait(semArray, 0);
        // Add job to end of queue
        queue[rear] = job_duration;
        int job_id = rear;
        // Move up rear of queue and check for overflow
        rear++;
        if (rear > queue_size) {
            rear = 0;
        }
        // Remove mutual exclusion
        sem_signal(semArray, 0);

        // Print the status
        cout << "Producer(" << thread_id << "): Job id " << job_id << " duration " << job_duration << endl;
        sem_signal(semArray, 2);
    }

    cout << "Producer(" << thread_id << "): No more jobs to generate." << endl;

    // Clear producer arguments struct from memory
    delete prod_args;
    // Quit when there are no more jobs left to produce
    pthread_exit(0);
}

void *consumer(void *args) {
    // Assign variables to the argument passed in
    ThreadArguments *cons_args = (ThreadArguments *)args;
    int *queue = cons_args->queue;
    int thread_id = cons_args->thread_id;
    int queue_size = cons_args->queue_size;

    while (1) {
        // Wait for a job to be added to the queue
        // If there are no jobs left to consume, wait for 20 seconds to check if any new jobs are added, and if not, quit.
        if (sem_wait(semArray, 2, TIME_OUT) < 0) {
            cout << "Consumer(" << thread_id << "): No more jobs left." << endl;
            break;
        };

        // Mutual Exclusion to Queue
        sem_wait(semArray, 0);

        // Retrieve job duration and job id
        int job_duration = queue[front];
        int job_id = front;

        /// Move front of queue up and check for overflow
        front++;
        if (front > queue_size) {
            front = 0;
        }

        // Remove Mutual Exclusion
        sem_signal(semArray, 0);
        // Signal producer that job is being consumed and slot has been freed
        sem_signal(semArray, 1);
        cout << "Consumer(" << thread_id << "): Job id " << job_id << " executing sleep duration " << job_duration << endl;
        // Consume job
        sleep(job_duration);
        cout << "Consumer(" << thread_id << "): Job id " << job_id << " completed" << endl;
    }
    // Clear consumer arguments struct from memory
    delete cons_args;
    pthread_exit(0);
}
