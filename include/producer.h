/*
 * Producer Header File
 * Producer thread interface and structures
 */

#ifndef PRODUCER_H
#define PRODUCER_H

#include "queue.h"
#include "analytics.h"

// Producer thread arguments structure
typedef struct {
    int id;                         // Producer ID (1, 2, 3, ...)
    Queue *queue;                   // Pointer to shared queue
    int max_wait;                   // Maximum wait time between writes
    volatile int *timeout_flag;     // Pointer to global timeout flag
    Analytics *analytics;           // Pointer to analytics structure
} ProducerArgs;

// Function declarations
void* producer_thread(void *args);
int queue_is_full_check(Queue *q);

#endif
