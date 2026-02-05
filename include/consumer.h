/*
 * Consumer Header File
 * Consumer thread interface and structures
 */

#ifndef CONSUMER_H
#define CONSUMER_H

#include "queue.h"
#include "analytics.h"

// Consumer thread arguments structure
typedef struct {
    int id;                         // Consumer ID (1, 2, 3, ...)
    Queue *queue;                   // Pointer to shared queue
    int max_wait;                   // Maximum wait time between reads
    volatile int *timeout_flag;     // Pointer to global timeout flag
    Analytics *analytics;           // Pointer to analytics structure
} ConsumerArgs;

// Function declarations
void* consumer_thread(void *args);
int queue_is_empty_check(Queue *q);

#endif
