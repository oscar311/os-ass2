/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include "synch.h"
#include "producerconsumer_driver.h"

/* Declare any variables you need here to keep track of and
   synchronise your bounded. A sample declaration of a buffer is shown
   below. You can change this if you choose another implementation. */

static struct pc_data buffer[BUFFER_SIZE];

// the start and end indices of the array
int start;
int end;

struct semaphore *consumer, *producer;
struct lock *lock; //, *lock_producer;


/* consumer_receive() is called by a consumer to request more data. It
   should block on a sync primitive if no data is available in your
   buffer. */

struct pc_data consumer_receive(void)
{
    P(consumer); // decrement - used to represent waiting for producers to produce something 


    struct pc_data thedata;

    lock_acquire(lock);
    // critical section - start
	thedata = buffer[start];
    start = (start + 1 ) % BUFFER_SIZE; // the creates circular 	
    // critical section - end 
    lock_release(lock);
    
    V(producer); // - increment - used to represent a consumer consuming something

    return thedata;
}

/* procucer_send() is called by a producer to store data in your
   bounded buffer. */

void producer_send(struct pc_data item)
{


    P(producer); // decrement - used to represent waiting for a consumer to consume something

    lock_acquire(lock);
    // critical section - start
    buffer[end] = item;
    end = (end + 1) % BUFFER_SIZE;    
    // critical section - end 
    lock_release(lock);



    V(consumer); // increment - used to represent a producers 
                 // produced something and therefore the consumer can continue 

}




/* Perform any initialisation (e.g. of global data) you need
   here. Note: You can panic if any allocation fails during setup */

void producerconsumer_startup(void)
{
    consumer = sem_create("consumer", 0);
    producer = sem_create("producer", BUFFER_SIZE-1);

    start = 0;
    end = 0;

    lock = lock_create("lock_consumer");
    //lock_producer = lock_create("lock_producer");


}

/* Perform any clean-up you need here */
void producerconsumer_shutdown(void)
{

    sem_destroy(consumer);
    sem_destroy(producer);

    lock_destroy(lock);
    //lock_destroy(lock_producer);


}

