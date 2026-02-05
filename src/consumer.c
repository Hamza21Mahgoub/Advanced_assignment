/*
 * Consumer Thread Implementation
 * 
 * Each consumer thread reads items from the shared queue and displays them.
 * Implements priority-based consumption as specified in the coursework.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "consumer.h"
#include "queue.h"
#include "utils.h"
#include "config.h"

/*
 * Helper function to check if queue is empty without blocking
 */
int queue_is_empty_check(Queue *q) {
    pthread_mutex_lock(&q->mutex);
    int is_empty = queue_is_empty(q);
    pthread_mutex_unlock(&q->mutex);
    return is_empty;
}

/*
 * Consumer thread function
 * Continuously reads from queue and processes items until timeout
 */
void* consumer_thread(void *args) {
    ConsumerArgs *cargs = (ConsumerArgs *)args;
    
    if (cargs == NULL) {
        fprintf(stderr, "Error: Consumer received NULL arguments\n");
        return NULL;
    }
    
    int consumer_id = cargs->id;
    Queue *queue = cargs->queue;
    int max_wait = cargs->max_wait;
    volatile int *timeout_flag = cargs->timeout_flag;
    Analytics *analytics = cargs->analytics;
    
    printf("[C%d] Consumer thread started\n", consumer_id);
    
    // Create thread-local random seed
    unsigned int local_seed = (unsigned int)(get_timestamp() * 1000) + consumer_id + 100;
    int items_consumed = 0;
    
    // Main consumer loop - continues until timeout
    while (!(*timeout_flag)) {
        QueueItem item;
        
        // Check timeout before potentially blocking on empty queue
        if (*timeout_flag) {
            break;
        }
        
        // Track if we're about to block
        int was_empty = queue_is_empty_check(queue);
        
        // Attempt to read from queue (may block if empty)
        int result = queue_dequeue(queue, &item);
        
        if (result == 0) {
            items_consumed++;
            
            // Calculate latency (time from production to consumption)
            double current_time = get_timestamp();
            double latency = current_time - item.timestamp;
            
            // Display consumed item
            printf("[C%d] READ  <- seq=%d, value=%d, priority=%d, from P%d, latency=%.3fs, queue_size=%d\n",
                   consumer_id, item.sequence, item.value, item.priority, 
                   item.producer_id, latency, queue_get_size(queue));
            
            // Record analytics with priority information
            analytics_record_consume_priority(analytics, item.priority, latency);
            
            if (was_empty) {
                // We were blocked waiting for data
                analytics_record_consumer_block(analytics);
                printf("[C%d] STARVED (queue was empty, waited for data)\n", consumer_id);
            }
            
        } else {
            // Error reading from queue
            if (!(*timeout_flag)) {
                fprintf(stderr, "[C%d] Error: Failed to dequeue item\n", consumer_id);
            }
            break;
        }
        
        // Wait for random time before next read
        int wait_time = random_range_seed(1, max_wait, &local_seed);
        
        if (DEBUG_MODE) {
            printf("[C%d] Sleeping for %d second(s)...\n", consumer_id, wait_time);
        }
        
        // Sleep in small increments to check timeout flag
        for (int i = 0; i < wait_time && !(*timeout_flag); i++) {
            sleep(1);
        }
    }
    
    printf("[C%d] Consumer thread terminating (consumed %d items)\n", 
           consumer_id, items_consumed);
    
    return NULL;
}