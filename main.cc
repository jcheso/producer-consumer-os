/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

void *producer(void *args);
void *consumer(void *args);

int semEmpty = sem_create(SEM_KEY, 1);
int semFull = sem_create(SEM_KEY, 1);
pthread_mutex_t mutexBuffer;
queue<int> circular_queue;
int count = 0;

int main(int argc, char **argv)
{

  /* ------ READ IN ARGUMENTS ----- */
  // Read in four command line arguments - size of the queue, number of jobs to generate for
  // each producer (each producer will generate the same number of jobs), number of producers,
  // and number of consumers.
  if (argc < 5)
  {
    cerr << "Not enough arguments" << endl;
    return EXIT_FAILURE;
  }

  int queue_size = check_arg(argv[1]);
  int num_jobs = check_arg(argv[2]);
  int num_producers = check_arg(argv[3]);
  int num_consumers = check_arg(argv[4]);

  cout << "-------------------------------" << endl;
  cout << "Queue Size: " << queue_size << endl;
  cout << "Number of Jobs: " << num_jobs << endl;
  cout << "Number of Producers: " << num_producers << endl;
  cout << "Number of Consumers: " << num_consumers << endl;
  cout << "-------------------------------" << endl;

  // Set-up and initialise the required data structures and variables, as necessary.
  srand(time(NULL));
  pthread_t producerid[num_producers];
  pthread_t consumerid[num_consumers];
  pthread_mutex_init(&mutexBuffer, NULL);

  // Set-up and initialise semaphores, as necessary.
  sem_init(semEmpty, 0, queue_size);
  sem_init(semFull, 0, 0);
  int i;
  int ids[num_producers + num_consumers];

  // Create Producer Threads
  for (i = 0; i < num_producers; i++)
  {
    ids[i] = i + 1;
    if (pthread_create(&producerid[i], NULL, producer, &(ids[i])) != 0)
      perror("Failed to create producer thread");
  }
  // Create Consumer Threads
  for (i = 0; i < num_consumers; i++)
  {
    ids[i] = i + 1;
    if (pthread_create(&consumerid[i], NULL, consumer, &(ids[i])) != 0)
      perror("Failed to create consumer thread");
  }
  // Join Producer Threads
  for (i = 0; i < num_producers; i++)
  {
    if (pthread_join(producerid[i], NULL) != 0)
      perror("Failed to join producer thread");
  }
  // Join Consumer Threads
  for (i = 0; i < num_consumers; i++)
  {
    if (pthread_join(consumerid[i], NULL) != 0)
      perror("Failed to join consumer thread");
  }

  // Destroy semaphores
  sem_close(semEmpty);
  sem_close(semFull);

  // Destroy mutex buffer
  pthread_mutex_destroy(&mutexBuffer);
  return 0;
}

void *producer(void *args)
{

  while (1)
  {
    // (a) Initialise parameters, as required.
    int id = *((int *)args);
    int x = rand() % 10;
    int delay = rand() % 5;

    // (b) Add the required number of jobs to the circular queue, with each job being added once every
    // 1 – 5 seconds. If a job is taken (and deleted) by the consumer, then another job can be
    // produced which has the same id. Duration for each job should be between 1 – 10 seconds. If
    // the circular queue is full, block while waiting for an empty slot and if a slot doesn’t become
    // available after 20 seconds, quit, even though you have not produced all the jobs.
    sleep(delay);
    sem_wait(semEmpty, 0);
    pthread_mutex_lock(&mutexBuffer);
    circular_queue.push(x);
    pthread_mutex_unlock(&mutexBuffer);
    sem_signal(semFull, 0);

    // (c) Print the status (example format given in example output.txt).
    cout << "Producer(" << id << "): Job id " << circular_queue.size() - 1 << " duration " << x << endl;

    // (d) Quit when there are no more jobs left to produce.
  }
  pthread_exit(0);
}

void *consumer(void *args)
{
  while (1)
  {
    // (a) Initialise parameters, as required.
    int id = *((int *)args);
    int y;

    // (b) Take a job from the circular queue and ‘sleep’ for the duration specified. If the circular queue
    // is empty, block while waiting for jobs and quit if no jobs arrive within 20 seconds.
    sem_wait(semFull, 0);
    pthread_mutex_lock(&mutexBuffer);
    y = circular_queue.front();
    circular_queue.pop();
    pthread_mutex_unlock(&mutexBuffer);
    sem_signal(semEmpty, 0);

    // (c) Print the status (example format given in example output.txt).
    cout << "Consumer(" << id << "): Job id " << circular_queue.size() - 1 << " executing sleep duration " << y << endl;
    sleep(y);
    cout << "Consumer(" << id << "): Job id " << circular_queue.size() - 1 << " completed" << endl;

    // (d) If there are no jobs left to consume, wait for 20 seconds to check if any new jobs are added,
    // and if not, quit.
  }
  pthread_exit(0);
}