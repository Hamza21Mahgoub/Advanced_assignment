/*
 * Queue Implementation
 * 
 * Implements a priority FIFO queue with thread-safe operations
 * using mutex locks and condition variables for synchronization.
 * 
 * Based on concepts from Week 3 lecture notes on process synchronization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue.h"
#include "config.h"
extern volatile int timeout_flag;

/*
 * Initialize a new queue with the specified capacity
 * Returns pointer to queue on success, NULL on failure
 */
Queue* queue_init(int capacity) {
    if (capacity <= 0 || capacity > 20) {
        fprintf(stderr, "Error: Invalid queue capacity %d\n", capacity);
        return NULL;
    }
    
    Queue *q = (Queue *)malloc(sizeof(Queue));
    if (q == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for queue structure\n");
        return NULL;
    }
    
    q->items = (QueueItem *)malloc(sizeof(QueueItem) * capacity);
    if (q->items == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for queue items\n");
        free(q);
        return NULL;
    }
    
    q->capacity = capacity;
    q->size = 0;
    q->head = 0;
    q->tail = 0;
    
    // Initialize mutex
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize queue mutex\n");
        free(q->items);
        free(q);
        return NULL;
    }
    
    // Initialize condition variables
    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize not_full condition variable\n");
        pthread_mutex_destroy(&q->mutex);
        free(q->items);
        free(q);
        return NULL;
    }
    
    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize not_empty condition variable\n");
        pthread_cond_destroy(&q->not_full);
        pthread_mutex_destroy(&q->mutex);
        free(q->items);
        free(q);
        return NULL;
    }
    
    if (DEBUG_MODE) {
        printf("[QUEUE] Initialized queue with capacity %d\n", capacity);
    }
    
    return q;
}

/*
 * Check if queue is full (must be called with mutex locked)
 */
int queue_is_full(Queue *q) {
    return q->size == q->capacity;
}

/*
 * Check if queue is empty (must be called with mutex locked)
 */
int queue_is_empty(Queue *q) {
    return q->size == 0;
}

/*
 * Get the current size of the queue (thread-safe)
 */
int queue_get_size(Queue *q) {
    pthread_mutex_lock(&q->mutex);
    int size = q->size;
    pthread_mutex_unlock(&q->mutex);
    return size;
}

/*
 * Enqueue an item into the queue
 * Blocks if queue is full (using condition variable)
 * Returns 0 on success, -1 on error
 */
int queue_enqueue(Queue *q, QueueItem item) {
    if (q == NULL) {
        fprintf(stderr, "Error: Cannot enqueue to NULL queue\n");
        return -1;
    }
    
    // Acquire mutex lock - entering critical section
    pthread_mutex_lock(&q->mutex);
    
    // Wait while queue is full (condition variable)
    // This implements the "Producer must not write to full queue" requirement
while (queue_is_full(q) && !timeout_flag) {
    pthread_cond_wait(&q->not_full, &q->mutex);
}

if (timeout_flag) {
    pthread_mutex_unlock(&q->mutex);
    return -1;
}

    
    // Add item to queue (circular buffer)
    q->items[q->tail] = item;
    q->tail = (q->tail + 1) % q->capacity;
    q->size++;
    
    if (DEBUG_MODE) {
        printf("[QUEUE] Enqueued: value=%d, priority=%d, from P%d | Queue size: %d/%d\n",
               item.value, item.priority, item.producer_id, q->size, q->capacity);
    }
    
    // Signal that queue is not empty (wake up waiting consumers)
    pthread_cond_signal(&q->not_empty);
    
    // Release mutex lock - leaving critical section
    pthread_mutex_unlock(&q->mutex);
    
    return 0;
}

/*
 * Find the index of the highest priority item in the queue
 * Must be called with mutex locked
 * Returns index of highest priority item, or -1 if queue is empty
 */
static int find_highest_priority_index(Queue *q) {
    if (queue_is_empty(q)) {
        return -1;
    }
    
    int highest_idx = q->head;
    int highest_priority = q->items[q->head].priority;
    
    // Scan through the queue to find highest priority
    int idx = q->head;
    for (int i = 0; i < q->size; i++) {
        if (q->items[idx].priority > highest_priority) {
            highest_priority = q->items[idx].priority;
            highest_idx = idx;
        }
        idx = (idx + 1) % q->capacity;
    }
    
    return highest_idx;
}

/*
 * Remove an item from a specific index and compact the queue
 * Must be called with mutex locked
 */
static void remove_at_index(Queue *q, int remove_idx) {
    // Shift elements to fill the gap (maintain FIFO order for same priority)
    int current = remove_idx;
    int shifts = 0;
    
    // Count how many items to shift
    if (remove_idx >= q->head) {
        shifts = q->tail > remove_idx ? q->tail - remove_idx - 1 : 
                                        (q->capacity - remove_idx) + q->tail - 1;
    } else {
        shifts = q->tail - remove_idx - 1;
    }
    
    // Shift items
    for (int i = 0; i < shifts; i++) {
        int next = (current + 1) % q->capacity;
        q->items[current] = q->items[next];
        current = next;
    }
    
    // Update tail and size
    q->tail = (q->tail - 1 + q->capacity) % q->capacity;
    q->size--;
}

/*
 * Dequeue an item from the queue
 * Prioritizes high-priority items over FIFO order
 * Blocks if queue is empty (using condition variable)
 * Returns 0 on success, -1 on error
 */
int queue_dequeue(Queue *q, QueueItem *item) {
    if (q == NULL || item == NULL) {
        fprintf(stderr, "Error: Cannot dequeue from NULL queue or to NULL item\n");
        return -1;
    }
    
    // Acquire mutex lock - entering critical section
    pthread_mutex_lock(&q->mutex);
    
    // Wait while queue is empty (condition variable)
    // This implements the "Consumer must not read from empty queue" requirement
while (queue_is_empty(q) && !timeout_flag) {
    pthread_cond_wait(&q->not_empty, &q->mutex);
}

if (timeout_flag) {
    pthread_mutex_unlock(&q->mutex);
    return -1;
}

    
    // Find highest priority item
    int remove_idx = find_highest_priority_index(q);
    
    if (remove_idx == -1) {
        // Should never happen due to the wait above
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    
    // Copy item out
    *item = q->items[remove_idx];
    
    // If it's the head, simple FIFO removal
    if (remove_idx == q->head) {
        q->head = (q->head + 1) % q->capacity;
        q->size--;
    } else {
        // Priority override - remove from middle and compact
        remove_at_index(q, remove_idx);
    }
    
    if (DEBUG_MODE) {
        printf("[QUEUE] Dequeued: value=%d, priority=%d, from P%d | Queue size: %d/%d\n",
               item->value, item->priority, item->producer_id, q->size, q->capacity);
    }
    
    // Signal that queue is not full (wake up waiting producers)
    pthread_cond_signal(&q->not_full);
    
    // Release mutex lock - leaving critical section
    pthread_mutex_unlock(&q->mutex);
    
    return 0;
}

/*
 * Destroy queue and free all resources
 * This implements proper cleanup as discussed in lectures
 */
void queue_destroy(Queue *q) {
    if (q == NULL) {
        return;
    }
    
    if (DEBUG_MODE) {
        printf("[QUEUE] Destroying queue and releasing resources...\n");
    }
    
    // Destroy synchronization primitives
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    pthread_mutex_destroy(&q->mutex);
    
    // Free memory
    free(q->items);
    free(q);
    
    if (DEBUG_MODE) {
        printf("[QUEUE] Queue resources released\n");
    }
}