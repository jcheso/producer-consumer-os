/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

void *producer (void *id);
void *consumer (void *id);

int main (int argc, char **argv)
{

  // TODO
  // (a) Read in four command line arguments - size of the queue, number of jobs to generate for
  // each producer (each producer will generate the same number of jobs), number of producers,
  // and number of consumers.
  // (b) Set-up and initialise the required data structures and variables, as necessary.
  // (c) Set-up and initialise semaphores, as necessary.
  // (d) Create the required producers and consumers.
  // (e) Quit, but ensure that there is process clean-up.

  pthread_t producerid;
  
  int parameter = 5;

  pthread_create (&producerid, NULL, producer, (void *) &parameter);

  pthread_join (producerid, NULL);

  cout << "Doing some work after the join" << endl;

  return 0;
}

void *producer (void *parameter) 
{

  // TODO
  // (a) Initialise parameters, as required.
  // (b) Add the required number of jobs to the circular queue, with each job being added once every
  // 1 – 5 seconds. If a job is taken (and deleted) by the consumer, then another job can be
  // produced which has the same id. Duration for each job should be between 1 – 10 seconds. If
  // the circular queue is full, block while waiting for an empty slot and if a slot doesn’t become
  // available after 20 seconds, quit, even though you have not produced all the jobs.
  // (c) Print the status (example format given in example output.txt).
  // (d) Quit when there are no more jobs left to produce.
  int *param = (int *) parameter;

  cout << "Parameter = " << *param << endl;

  sleep (5);

  cout << "\nThat was a good sleep - thank you \n" << endl;

  pthread_exit(0);
}

void *consumer (void *id) 
{
    // TODO 
    // (a) Initialise parameters, as required.
    // (b) Take a job from the circular queue and ‘sleep’ for the duration specified. If the circular queue
    // is empty, block while waiting for jobs and quit if no jobs arrive within 20 seconds.
    //     (c) Print the status (example format given in example output.txt).
    // (d) If there are no jobs left to consume, wait for 20 seconds to check if any new jobs are added,
    // and if not, quit.
  pthread_exit (0);

}
