/*
 * ELE430 Producer-Consumer Coursework
 * Main entry point and program orchestration
 * 
 * This program models a producer-consumer system with multiple threads
 * interacting through a shared priority FIFO queue, demonstrating
 * multiprogramming and synchronization concepts covered in lectures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "config.h"
#include "queue.h"
#include "producer.h"
#include "consumer.h"
#include "utils.h"
#include "analytics.h"

// Global timeout flag - volatile as it's accessed by multiple threads
volatile int timeout_flag = 0;

// Global analytics structure
Analytics *global_analytics = NULL;

// Global queue pointer for timeout handler
Queue *global_queue = NULL;

// Signal handler for timeout
void timeout_handler(int signum) {
    (void)signum; // Suppress unused parameter warning
    timeout_flag = 1;
    printf("\n[TIMEOUT] Timeout reached, signaling all threads to terminate...\n");
    
    // Wake up all threads waiting on condition variables
    if (global_queue != NULL) {
        pthread_mutex_lock(&global_queue->mutex);
        pthread_cond_broadcast(&global_queue->not_full);
        pthread_cond_broadcast(&global_queue->not_empty);
        pthread_mutex_unlock(&global_queue->mutex);
    }
}

/*
 * Display usage information
 */
void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <n_producers> <n_consumers> <queue_size> <timeout_seconds>\n", program_name);
    fprintf(stderr, "  n_producers: Number of producer threads (1-%d)\n", MAX_PRODUCERS);
    fprintf(stderr, "  n_consumers: Number of consumer threads (1-%d)\n", MAX_CONSUMERS);
    fprintf(stderr, "  queue_size:  Maximum queue entries (1-20)\n");
    fprintf(stderr, "  timeout_seconds: Runtime duration in seconds\n");
    fprintf(stderr, "\nExample: %s 5 3 10 30\n", program_name);
}

/*
 * Validate command line arguments
 */
int validate_arguments(int n_producers, int n_consumers, int queue_size, int timeout) {
    int valid = 1;
    
    if (n_producers < 1 || n_producers > MAX_PRODUCERS) {
        fprintf(stderr, "Error: n_producers must be between 1 and %d\n", MAX_PRODUCERS);
        valid = 0;
    }
    
    if (n_consumers < 1 || n_consumers > MAX_CONSUMERS) {
        fprintf(stderr, "Error: n_consumers must be between 1 and %d\n", MAX_CONSUMERS);
        valid = 0;
    }
    
    if (queue_size < 1 || queue_size > 20) {
        fprintf(stderr, "Error: queue_size must be between 1 and 20\n");
        valid = 0;
    }
    
    if (timeout <= 0) {
        fprintf(stderr, "Error: timeout must be positive\n");
        valid = 0;
    }
    
    return valid;
}

int main(int argc, char *argv[]) {
    // Parse command line arguments
    if (argc != 5) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    int n_producers = atoi(argv[1]);
    int n_consumers = atoi(argv[2]);
    int queue_size = atoi(argv[3]);
    int timeout = atoi(argv[4]);
    
    // Validate arguments
    if (!validate_arguments(n_producers, n_consumers, queue_size, timeout)) {
        return EXIT_FAILURE;
    }
    
    // Print program header
    printf("================================================================================\n");
    printf("               ELE430 Producer-Consumer System Model\n");
    printf("================================================================================\n\n");
    
    // Print system information (username, hostname, timestamp)
    print_system_info();
    
    // Print runtime parameters
    printf("\n--- Runtime Configuration ---\n");
    print_run_parameters(n_producers, n_consumers, queue_size, timeout);
    
    // Print compiled defaults
    printf("\n--- Compiled Model Parameters ---\n");
    printf("Max Producer Wait:     %d seconds\n", DEFAULT_MAX_PRODUCER_WAIT);
    printf("Max Consumer Wait:     %d seconds\n", DEFAULT_MAX_CONSUMER_WAIT);
    printf("Random Value Range:    %d to %d\n", RANDOM_VALUE_MIN, RANDOM_VALUE_MAX);
    printf("Debug Mode:            %s\n", DEBUG_MODE ? "ENABLED" : "DISABLED");
    
    printf("\n================================================================================\n");
    printf("Starting simulation at %s", get_current_time_string());
    printf("================================================================================\n\n");
    
    // Initialize queue
    Queue *queue = queue_init(queue_size);
    if (queue == NULL) {
        fprintf(stderr, "Error: Failed to initialize queue\n");
        return EXIT_FAILURE;
    }
    
    // Set global queue pointer for timeout handler
    global_queue = queue;
    
    // Initialize analytics
    global_analytics = analytics_init();
    if (global_analytics == NULL) {
        fprintf(stderr, "Error: Failed to initialize analytics\n");
        queue_destroy(queue);
        return EXIT_FAILURE;
    }
    
    // Setup timeout signal handler
    signal(SIGALRM, timeout_handler);
    alarm(timeout);
    
    // Create producer threads
    pthread_t producer_threads[MAX_PRODUCERS];
    ProducerArgs producer_args[MAX_PRODUCERS];
    
    printf("[INIT] Creating %d producer thread(s)...\n", n_producers);
    for (int i = 0; i < n_producers; i++) {
        producer_args[i].id = i + 1;
        producer_args[i].queue = queue;
        producer_args[i].max_wait = DEFAULT_MAX_PRODUCER_WAIT;
        producer_args[i].timeout_flag = &timeout_flag;
        producer_args[i].analytics = global_analytics;
        
        if (pthread_create(&producer_threads[i], NULL, producer_thread, &producer_args[i]) != 0) {
            fprintf(stderr, "Error: Failed to create producer thread %d\n", i + 1);
            timeout_flag = 1; // Signal other threads to stop
            // Clean up already created threads
            for (int j = 0; j < i; j++) {
                pthread_join(producer_threads[j], NULL);
            }
            analytics_destroy(global_analytics);
            queue_destroy(queue);
            return EXIT_FAILURE;
        }
        printf("[INIT] Producer P%d created (PID: %d, TID: %lu)\n", 
               i + 1, getpid(), (unsigned long)producer_threads[i]);
    }
    
    // Create consumer threads
    pthread_t consumer_threads[MAX_CONSUMERS];
    ConsumerArgs consumer_args[MAX_CONSUMERS];
    
    printf("[INIT] Creating %d consumer thread(s)...\n", n_consumers);
    for (int i = 0; i < n_consumers; i++) {
        consumer_args[i].id = i + 1;
        consumer_args[i].queue = queue;
        consumer_args[i].max_wait = DEFAULT_MAX_CONSUMER_WAIT;
        consumer_args[i].timeout_flag = &timeout_flag;
        consumer_args[i].analytics = global_analytics;
        
        if (pthread_create(&consumer_threads[i], NULL, consumer_thread, &consumer_args[i]) != 0) {
            fprintf(stderr, "Error: Failed to create consumer thread %d\n", i + 1);
            timeout_flag = 1; // Signal other threads to stop
            // Clean up
            for (int j = 0; j < n_producers; j++) {
                pthread_join(producer_threads[j], NULL);
            }
            for (int j = 0; j < i; j++) {
                pthread_join(consumer_threads[j], NULL);
            }
            analytics_destroy(global_analytics);
            queue_destroy(queue);
            return EXIT_FAILURE;
        }
        printf("[INIT] Consumer C%d created (PID: %d, TID: %lu)\n", 
               i + 1, getpid(), (unsigned long)consumer_threads[i]);
    }
    
    printf("\n[RUNNING] All threads created. Model is now executing...\n");
    printf("[RUNNING] Press Ctrl+C to stop early, or wait for timeout.\n\n");
    
    // Record start time
    double start_time = get_timestamp();
    
    // Wait for all producer threads to complete
    for (int i = 0; i < n_producers; i++) {
        pthread_join(producer_threads[i], NULL);
        if (DEBUG_MODE) {
            printf("[CLEANUP] Producer P%d joined\n", i + 1);
        }
    }
    
    // Wait for all consumer threads to complete
    for (int i = 0; i < n_consumers; i++) {
        pthread_join(consumer_threads[i], NULL);
        if (DEBUG_MODE) {
            printf("[CLEANUP] Consumer C%d joined\n", i + 1);
        }
    }
    
    // Calculate runtime
    double end_time = get_timestamp();
    double runtime = end_time - start_time;
    
    // Print final summary
    printf("\n================================================================================\n");
    printf("                          Simulation Complete\n");
    printf("================================================================================\n");
    printf("Ended at: %s", get_current_time_string());
    printf("Total Runtime: %.2f seconds\n\n", runtime);
    
    // Print analytics summary
    analytics_print_summary(global_analytics, runtime, n_producers, n_consumers);
    
    printf("\n================================================================================\n");
    printf("                    All threads terminated cleanly\n");
    printf("================================================================================\n");
    
    // Cleanup
    analytics_destroy(global_analytics);
    queue_destroy(queue);
    
    return EXIT_SUCCESS;
}