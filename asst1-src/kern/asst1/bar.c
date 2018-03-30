#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "bar.h"
#include "bar_driver.h"



/*
 * **********************************************************************
 * YOU ARE FREE TO CHANGE THIS FILE BELOW THIS POINT AS YOU SEE FIT
 *
 */

/* Declare any globals you need here (e.g. locks, etc...) */
struct semaphore *customer_sem, *bartender_sem;
struct semaphore *customer_waiting[NCUSTOMERS];
struct lock *bar_lock;
struct barorder requested_orders[NCUSTOMERS]; // can only queue orders to the number of customers

int start;
int end;

/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */

/*
 * order_drink()
 *
 * Takes one argument referring to the order to be filled. The
 * function makes the order available to staff threads and then blocks
 * until a bartender has filled the glass with the appropriate drinks.
 */

void order_drink(struct barorder *order)
{


    // place order in the queue 
    P(customer_sem); // decrement - used to represent a producer producing something, as it start at,
                     //             customer size, therefore it only waits if every customer has ordered


    lock_acquire(bar_lock);
    int index = end;
    order->spot = index;
    requested_orders[end] = *order;
    end = (end + 1) % NCUSTOMERS;

    lock_release(bar_lock);



    V(bartender_sem); // increment - used to represent that there is now a drink available to be served 

    P(customer_waiting[index]); // decrement - used to represent the customer waiting for their order to be filled


}



/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY BARTENDER THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it returns a pointer to the order.
 *
 */

struct barorder *take_order(void)
{

        P(bartender_sem); // decrement - used to represent a bartender waiting for a drink to be ordered
                         //             as it waits at 0



        lock_acquire(bar_lock);

        struct barorder *ret = &requested_orders[start];
        
        start = (start+1) % NCUSTOMERS;

        lock_release(bar_lock);


        V(customer_sem); // increment - used to represent that a space for an order has opened 



        return ret;
}


/*
 * fill_order()
 *
 * This function takes an order provided by take_order and fills the
 * order using the mix() function to mix the drink.
 *
 * NOTE: IT NEEDS TO ENSURE THAT MIX HAS EXCLUSIVE ACCESS TO THE
 * REQUIRED BOTTLES (AND, IDEALLY, ONLY THE BOTTLES) IT NEEDS TO USE TO
 * FILL THE ORDER.
 */

void fill_order(struct barorder *order)
{

        /* add any sync primitives you need to ensure mutual exclusion
           holds as described */

        /* the call to mix must remain */
        



        lock_acquire(bar_lock);

        mix(order);

        lock_release(bar_lock);

}


/*
 * serve_order()
 *
 * Takes a filled order and makes it available to (unblocks) the
 * waiting customer.
 */

void serve_order(struct barorder *order)
{
    (void) order; /* avoid a compiler warning, remove when you
                         start */


    V(customer_waiting[order->spot]);
}



/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */


/*
 * bar_open()
 *
 * Perform any initialisation you need prior to opening the bar to
 * bartenders and customers. Typically, allocation and initialisation of
 * synch primitive and variable.
 */

void bar_open(void)
{
    customer_sem = sem_create("customer_sem", NCUSTOMERS - 1);
    bartender_sem = sem_create("bartender_sem", 0 );

    bar_lock = lock_create("bar_lock");

    start = 0;
    end = 0;

    for(int i = 0; i < NCUSTOMERS; i++) 
        customer_waiting[i] = sem_create("hello"+i, 0);
}

/*
 * bar_close()
 *
 * Perform any cleanup after the bar has closed and everybody
 * has gone home.
 */

void bar_close(void)
{

    sem_destroy(customer_sem);
    sem_destroy(bartender_sem);

    lock_destroy(bar_lock);


    for(int i = 0; i < NCUSTOMERS; i++) 
        sem_destroy( customer_waiting[i] );

}

