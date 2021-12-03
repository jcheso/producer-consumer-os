#include <array>
#include <memory>

#include "helper.h"

// Declare functions for producer and consumer
void *producer(void *args);
void *consumer(void *args);

// Create an array of 3 semaphores
int semArray = sem_create(SEM_KEY, 3);
int const TIME_OUT = 20;

class Queue {
   public:
    int front, rear, queue_size;
    std::unique_ptr<int[]> items;
    Queue(int queue_size) : queue_size(queue_size), items{new int[queue_size]} {
        front = -1;
        rear = -1;
    };
    int getValue(int index) {
        return items[index];
    }
    // Check if the queue is full
    bool isFull() {
        if (front == 0 && rear == queue_size - 1) {
            return true;
        }
        if (front == rear + 1) {
            return true;
        }
        return false;
    }
    // Check if the queue is empty
    bool isEmpty() {
        if (front == -1)
            return true;
        else
            return false;
    }
    // Adding an element
    void enQueue(int element) {
        if (isFull()) {
            cout << "Queue is full";
        } else {
            if (front == -1) front = 0;
            rear = (rear + 1) % queue_size;
            items[rear] = element;
        }
    }
    // Removing an element
    int deQueue() {
        int element;
        if (isEmpty()) {
            cout << "Queue is empty" << endl;
            return (-1);
        } else {
            element = items[front];
            if (front == rear) {
                front = -1;
                rear = -1;
            }
            // Q has only one element,
            // so we reset the queue after deleting it.
            else {
                front = (front + 1) % queue_size;
            }
            return (element);
        }
    }

    void display() {
        // Function to display status of Circular Queue
        int i;
        if (isEmpty()) {
            cout << endl
                 << "Empty Queue" << endl;
        } else {
            cout << "Front -> " << front;
            cout << endl
                 << "Items -> ";
            for (i = front; i != rear; i = (i + 1) % queue_size)
                cout << items[i];
            cout << items[i];
            cout << endl
                 << "Rear -> " << rear;
        }
    }
};

class ThreadArguments {
   public:
    int num_of_threads = 0;
    int threadId = 0;
    Queue *q;
    // Constructor
    ThreadArguments(int num_of_threads, int threadId, Queue *q) : num_of_threads(num_of_threads), threadId(threadId), q(q) {}
};

int main(int argc, char **argv) {
    // ** READ IN ARGUMENTS **
    // Read in four command line arguments - size of the queue, number of jobs to generate for
    // each producer, number of producers, and number of consumers.
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
    // Mutual Exclusion Binary Sempahore
    sem_init(semArray, 0, 1);
    // Semphore to check if queue has space
    sem_init(semArray, 1, queue_size);
    // Semaphore to check if buffer is not empty
    sem_init(semArray, 2, 0);

    // ** INITIALISE DATA STRUCTURES **
    int i;
    Queue *q = new Queue(queue_size);
    // Randomise the random number seed for each run of the program
    srand(time(NULL));

    // Create Producer Threads
    for (i = 0; i < num_producers; i++) {
        ThreadArguments *argument = new ThreadArguments(num_jobs, i, q);
        if (pthread_create(&producerid[i], NULL, producer, (void *)argument) != 0)
            perror("Failed to create producer thread");
    }
    // Create Consumer Threads
    for (i = 0; i < num_consumers; i++) {
        ThreadArguments *argument = new ThreadArguments(num_jobs, i, q);
        if (pthread_create(&consumerid[i], NULL, consumer, (void *)argument) != 0)
            perror("Failed to create consumer thread");
    }

    // Join Producer Threads
    for (i = 0; i < num_producers; i++) {
        if (pthread_join(producerid[i], NULL) != 0)
            perror("Failed to join producer thread");
    }

    // Join Consumer Threads
    for (i = 0; i < num_consumers; i++) {
        if (pthread_join(consumerid[i], NULL) != 0)
            perror("Failed to join consumer thread");
    }

    // Destroy semaphores
    sem_close(semArray);

    return 0;
}

void *producer(void *args) {
    ThreadArguments *prod_args = (ThreadArguments *)args;
    Queue *q = prod_args->q;
    // Iterate through and create the number of jobs required per a producer
    for (int i = 0; i < prod_args->num_of_threads; i++) {
        // Generate random duration for each job between 1 – 10 seconds.
        int job_duration = rand() % 10;
        // Sleep a 1-5 seconds before adding each job to the queue.
        int delay = rand() % 5;
        sleep(delay);

        // If the circular queue is full, block while waiting for an empty slot and if a slot doesn’t become
        // available after 20 seconds, quit, even though you have not produced all the jobs.
        if (sem_wait(semArray, 1, TIME_OUT) < 0) {
            cout << "Producer("
                 << prod_args->threadId
                 << "): No more jobs to generate." << endl;
            break;
        };

        sem_wait(semArray, 0);
        q->enQueue(job_duration);
        int job_id = q->front;
        // If a job is taken (and deleted) by the consumer, then another job can be produced which has the same id.
        sem_signal(semArray, 0);
        sem_signal(semArray, 2);

        // Print the status
        cout << "Producer("
             << prod_args->threadId
             << "): Job id " << job_id << " duration " << job_duration << endl;
    }
    // delete prod_args;
    // Quit when there are no more jobs left to produce.
    cout << "Producer("
         << prod_args->threadId
         << "): No more jobs to generate." << endl;
    pthread_exit(0);
}

void *consumer(void *args) {
    ThreadArguments *cons_args = (ThreadArguments *)args;
    Queue *q = cons_args->q;
    int threadId = cons_args->threadId;
    while (1) {
        // Wait for a job to be added to the queue
        // If there are no jobs left to consume, wait for 20 seconds to check if any new jobs are added, and if not, quit.
        if (sem_wait(semArray, 2, TIME_OUT) < 0) {
            cout << "Consumer(" << threadId << "): No more jobs left." << endl;
            break;
        };

        // Mutual Exclusion to Queue
        sem_wait(semArray, 0);
        // Retrieve job duration and job id
        int job_id = q->front;
        int job_duration = q->getValue(job_id);
        // Remove Mutual Exclusion
        sem_signal(semArray, 0);
        // Consume job
        sleep(job_duration);
        // Gain mutual exclusion to queue
        sem_wait(semArray, 0);
        // Remove job from queue
        q->deQueue();
        sem_signal(semArray, 0);

        // Print the status
        cout << "Consumer(" << job_id << "): Job id " << job_id << " executing sleep duration " << job_duration << endl;
        cout << "Consumer(" << job_id << "): Job id " << job_id << " completed" << endl;
        sem_signal(semArray, 1);
    }
    pthread_exit(0);
}
