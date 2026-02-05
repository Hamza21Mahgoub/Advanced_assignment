/*
 * Producer Thread Implementation
 * 
 * Each producer thread generates random integer values with assigned priorities
 * and writes them to the shared queue, implementing the producer role in the
 * producer-consumer pattern discussed in lectures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "producer.h"
#include "queue.h"
#include "utils.h"
#include "config.h"

/*
 * Producer thread function
 * Continuously generates random data and writes to queue until timeout
 */
void* producer_thread(void *args) {
    ProducerArgs *pargs = (ProducerArgs *)args;
    
    if (pargs == NULL) {
        fprintf(stderr, "Error: Producer received NULL arguments\n");
        return NULL;
    }
    
    int producer_id = pargs->id;
    Queue *queue = pargs->queue;
    int max_wait = pargs->max_wait;
    volatile int *timeout_flag = pargs->timeout_flag;
    Analytics *analytics = pargs->analytics;
    
    printf("[P%d] Producer thread started\n", producer_id);
    
    unsigned int local_seed = (unsigned int)(get_timestamp() * 1000) + producer_id;
    int sequence_number = 0;
    
    // Main producer loop - continues until timeout
    while (!(*timeout_flag)) {
        sequence_number++;
        
        // Generate random data value
        int value = random_range(RANDOM_VALUE_MIN, RANDOM_VALUE_MAX);
        
        // Assign priority based on value
        // High values (7-9) get high priority
        // Medium values (4-6) get normal priority  
        // Low values (0-3) get low priority
        int priority;
        if (value >= 7) {
            priority = PRIORITY_HIGH;
        } else if (value >= 4) {
            priority = PRIORITY_NORMAL;
        } else {
            priority = PRIORITY_LOW;
        }
        
        // Create queue item
        QueueItem item;
        item.value = value;
        item.priority = priority;
        item.producer_id = producer_id;
        item.timestamp = get_timestamp();
        item.sequence = sequence_number;
        
        if (DEBUG_MODE) {
            printf("[P%d] Generated: seq=%d, value=%d, priority=%d\n",
                   producer_id, sequence_number, value, priority);
        }
        
        // Check timeout before blocking on queue
        if (*timeout_flag) {
            break;
        }
        
        // Attempt to write to queue (may block if full)
        printf("[P%d] WRITE -> seq=%d, value=%d, priority=%d, queue_size=%d\n",
               producer_id, sequence_number, value, priority, queue_get_size(queue));
        
        // Track if we're about to block
        int was_full = queue_is_full_check(queue);
        
        int result = queue_enqueue(queue, item);
        
        if (result == 0) {
            // Successfully enqueued
            analytics_record_produce(analytics);
            
            if (was_full) {
                // We were blocked waiting for space
                analytics_record_producer_block(analytics);
                printf("[P%d] BLOCKED (queue was full, waited for space)\n", producer_id);
            }
        } else {
            fprintf(stderr, "[P%d] Error: Failed to enqueue item\n", producer_id);
        }
        
        // Wait for random time before next write
        int wait_time = random_range_seed(1, max_wait, &local_seed);
        
        if (DEBUG_MODE) {
            printf("[P%d] Sleeping for %d second(s)...\n", producer_id, wait_time);
        }
        
        // Sleep in small increments to check timeout flag
        for (int i = 0; i < wait_time && !(*timeout_flag); i++) {
            sleep(1);
        }
    }
    
    printf("[P%d] Producer thread terminating (produced %d items)\n", 
           producer_id, sequence_number);
    
    return NULL;
}

/*
 * Helper function to check if queue is full without blocking
 */
int queue_is_full_check(Queue *q) {
    pthread_mutex_lock(&q->mutex);
    int is_full = queue_is_full(q);
    pthread_mutex_unlock(&q->mutex);
    return is_full;
}
