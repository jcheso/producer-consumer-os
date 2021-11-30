#include "helper.h"
#include <queue>

// Declare functions for producer and consumer
void *producer(void *args);
void *consumer(void *args);

// Create an array of 3 semaphores
int semArray = sem_create(SEM_KEY, 3);
queue<vector<int>> circular_queue;
int const TIME_OUT = 20;

class ProducerArguments{
  public:
  int num_of_producers = 0;
  int producerId = 0;
  // Constructor
  ProducerArguments(int num_of_producers, int producerId) : num_of_producers(num_of_producers), producerId(producerId) {}
};

int main(int argc, char **argv)
{

  // ** READ IN ARGUMENTS **
  // Read in four command line arguments - size of the queue, number of jobs to generate for
  // each producer, number of producers, and number of consumers.
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

  int i;
  int ids[num_consumers];

// Randomise the random number seed for each run of the program
srand(time(NULL));

  // Create Producer Threads
  for (i = 0; i < num_producers; i++)
  {
    ProducerArguments* argument = new ProducerArguments(num_jobs, i);
    if (pthread_create(&producerid[i], NULL, producer, (void *)argument) != 0)
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
  sem_close(semArray);

  return 0;
}

void *producer(void *args)
{
  ProducerArguments* prod_args = (ProducerArguments *) args;

  // Iterate through and create the number of jobs required per a producer
  for (int i = 0; i < prod_args->num_of_producers; i++)
  {
    // Generate random duration for each job between 1 – 10 seconds.
    int job_duration = rand() % 10;
    // Sleep a 1-5 seconds before adding each job to the queue.
    int delay = rand() % 5;
    sleep(delay);

    
    // If the circular queue is full, block while waiting for an empty slot and if a slot doesn’t become
    // available after 20 seconds, quit, even though you have not produced all the jobs.
    if (sem_wait(semArray, 1, TIME_OUT) < 0){
      cout << "Producer("
         << prod_args->producerId
         << "): No more jobs to generate." << endl;
      break;
    };

    sem_wait(semArray, 0);
    // for ( job_id = 0; job_id < circular_queue.size(); job_id++){
    //   if job_id != 
    // }
    int job_id = circular_queue.size();
    vector<int> job{job_duration, job_id};
    circular_queue.push(job);
    // If a job is taken (and deleted) by the consumer, then another job can be produced which has the same id.  
    sem_signal(semArray, 0);    
    sem_signal(semArray, 2);

    // Print the status
    cout << "Producer("
         << prod_args->producerId
         << "): Job id " << job_id << " duration " << job_duration << endl;

  }
  // delete prod_args;
  // Quit when there are no more jobs left to produce.
  cout << "Producer("
    << prod_args->producerId
    << "): No more jobs to generate." << endl;
  pthread_exit(0);
}

void *consumer(void *args)
{
  while (1)
  {

    // Initialise parameters
    int id = *((int *)args);
    int job_duration;
    int job_id;

    // Wait for a job to be added to the queue
    // If there are no jobs left to consume, wait for 20 seconds to check if any new jobs are added, and if not, quit.
    if (sem_wait(semArray, 2, TIME_OUT) < 0){
      cout << "Consumer(" << id << "): No more jobs left." << endl;
      break;
    };

    // Mutual Exclusion to Queue
    sem_wait(semArray, 0);
    // Retrieve job duration and job id
    job_duration = circular_queue.front()[0];
    job_id = circular_queue.front()[1];
    // Remove Mutual Exclusion
    sem_signal(semArray,0);
    // Consume job
    sleep(job_duration);
    // Gain mutual exclusion to queue
    sem_wait(semArray, 0);
    // Remove job from queue
    circular_queue.pop();
    sem_signal(semArray,0);

    // Print the status
    cout << "Consumer(" << id << "): Job id " << job_id << " executing sleep duration " << job_duration << endl;
    cout << "Consumer(" << id << "): Job id " << job_id << " completed" << endl;
    sem_signal(semArray, 1);


  }
  pthread_exit(0);
}