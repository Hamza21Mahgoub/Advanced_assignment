/*
 * Queue Header File
 * Priority FIFO queue with thread-safe operations
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

// Queue item structure - represents one message in the queue
typedef struct {
    int value;           // Random data value (0-9)
    int priority;        // Priority level (0, 5, or 9)
    int producer_id;     // Which producer created this item
    double timestamp;    // When it was produced (for latency calculation)
    int sequence;        // Sequence number from this producer
} QueueItem;

// Queue structure - circular buffer with synchronization
typedef struct {
    QueueItem *items;           // Array of queue items
    int capacity;               // Maximum size of queue
    int size;                   // Current number of items
    int head;                   // Index of front item (for dequeue)
    int tail;                   // Index where next item will be added
    pthread_mutex_t mutex;      // Protects queue data structure
    pthread_cond_t not_full;    // Condition variable: signals when space available
    pthread_cond_t not_empty;   // Condition variable: signals when data available
} Queue;

// Function declarations
Queue* queue_init(int capacity);
int queue_enqueue(Queue *q, QueueItem item);
int queue_dequeue(Queue *q, QueueItem *item);
void queue_destroy(Queue *q);
int queue_is_full(Queue *q);
int queue_is_empty(Queue *q);
int queue_get_size(Queue *q);

#endif
