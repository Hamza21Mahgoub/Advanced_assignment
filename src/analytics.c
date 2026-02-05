/*
 * Analytics Implementation
 * 
 * Collects and tracks metrics about system performance including
 * throughput, blocking events, queue utilization, and latency.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "analytics.h"
#include "config.h"

/*
 * Initialize analytics structure
 */
Analytics* analytics_init(void) {
    Analytics *a = (Analytics *)malloc(sizeof(Analytics));
    if (a == NULL) {
        fprintf(stderr, "Error: Failed to allocate analytics structure\n");
        return NULL;
    }
    
    // Initialize counters
    a->total_produced = 0;
    a->total_consumed = 0;
    a->producer_blocks = 0;
    a->consumer_blocks = 0;
    a->total_latency = 0.0;
    a->min_latency = -1.0;  // Sentinel value for uninitialized
    a->max_latency = 0.0;
    a->high_priority_consumed = 0;
    a->normal_priority_consumed = 0;
    a->low_priority_consumed = 0;
    
    // Initialize mutex for thread-safe access
    if (pthread_mutex_init(&a->mutex, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize analytics mutex\n");
        free(a);
        return NULL;
    }
    
    if (DEBUG_MODE) {
        printf("[ANALYTICS] Analytics initialized\n");
    }
    
    return a;
}

/*
 * Record a produce event
 */
void analytics_record_produce(Analytics *a) {
    if (a == NULL) return;
    
    pthread_mutex_lock(&a->mutex);
    a->total_produced++;
    pthread_mutex_unlock(&a->mutex);
}

/*
 * Record a consume event with latency measurement
 */
void analytics_record_consume(Analytics *a, double latency) {
    if (a == NULL) return;
    
    pthread_mutex_lock(&a->mutex);
    a->total_consumed++;
    a->total_latency += latency;
    
    // Update min/max latency
    if (a->min_latency < 0 || latency < a->min_latency) {
        a->min_latency = latency;
    }
    if (latency > a->max_latency) {
        a->max_latency = latency;
    }
    
    pthread_mutex_unlock(&a->mutex);
}

/*
 * Record a consume event with priority information
 */
void analytics_record_consume_priority(Analytics *a, int priority, double latency) {
    if (a == NULL) return;
    
    pthread_mutex_lock(&a->mutex);
    a->total_consumed++;
    a->total_latency += latency;
    
    // Track by priority
    if (priority == PRIORITY_HIGH) {
        a->high_priority_consumed++;
    } else if (priority == PRIORITY_NORMAL) {
        a->normal_priority_consumed++;
    } else {
        a->low_priority_consumed++;
    }
    
    // Update min/max latency
    if (a->min_latency < 0 || latency < a->min_latency) {
        a->min_latency = latency;
    }
    if (latency > a->max_latency) {
        a->max_latency = latency;
    }
    
    pthread_mutex_unlock(&a->mutex);
}

/*
 * Record a producer block event (queue was full)
 */
void analytics_record_producer_block(Analytics *a) {
    if (a == NULL) return;
    
    pthread_mutex_lock(&a->mutex);
    a->producer_blocks++;
    pthread_mutex_unlock(&a->mutex);
}

/*
 * Record a consumer block event (queue was empty - starvation)
 */
void analytics_record_consumer_block(Analytics *a) {
    if (a == NULL) return;
    
    pthread_mutex_lock(&a->mutex);
    a->consumer_blocks++;
    pthread_mutex_unlock(&a->mutex);
}

/*
 * Print comprehensive analytics summary
 */
void analytics_print_summary(Analytics *a, double runtime, int n_producers, int n_consumers) {
    if (a == NULL) {
        fprintf(stderr, "Error: Cannot print summary for NULL analytics\n");
        return;
    }
    
    pthread_mutex_lock(&a->mutex);
    
    printf("--- Performance Metrics ---\n\n");
    
    // Production/Consumption metrics
    printf("Total Items Produced:     %ld\n", a->total_produced);
    printf("Total Items Consumed:     %ld\n", a->total_consumed);
    printf("Items Lost/In-Flight:     %ld\n", a->total_produced - a->total_consumed);
    
    // Throughput calculations
    if (runtime > 0) {
        double produce_rate = (double)a->total_produced / runtime;
        double consume_rate = (double)a->total_consumed / runtime;
        printf("\nProduction Rate:          %.2f items/second\n", produce_rate);
        printf("Consumption Rate:         %.2f items/second\n", consume_rate);
        printf("Per-Producer Rate:        %.2f items/sec/producer\n", produce_rate / n_producers);
        printf("Per-Consumer Rate:        %.2f items/sec/consumer\n", consume_rate / n_consumers);
    }
    
    // Latency metrics
    if (a->total_consumed > 0) {
        double avg_latency = a->total_latency / a->total_consumed;
        printf("\n--- Latency Statistics ---\n");
        printf("Average Latency:          %.3f seconds\n", avg_latency);
        printf("Minimum Latency:          %.3f seconds\n", a->min_latency);
        printf("Maximum Latency:          %.3f seconds\n", a->max_latency);
    }
    
    // Priority distribution
    if (a->total_consumed > 0) {
        printf("\n--- Priority Distribution ---\n");
        printf("High Priority (9):        %ld (%.1f%%)\n", 
               a->high_priority_consumed,
               100.0 * a->high_priority_consumed / a->total_consumed);
        printf("Normal Priority (5):      %ld (%.1f%%)\n", 
               a->normal_priority_consumed,
               100.0 * a->normal_priority_consumed / a->total_consumed);
        printf("Low Priority (0):         %ld (%.1f%%)\n", 
               a->low_priority_consumed,
               100.0 * a->low_priority_consumed / a->total_consumed);
    }
    
    // Blocking events (key metric for analysis)
    printf("\n--- Blocking Events (Critical Metric) ---\n");
    printf("Producer Blocks:          %ld times (queue full)\n", a->producer_blocks);
    printf("Consumer Blocks:          %ld times (queue empty/starved)\n", a->consumer_blocks);
    
    if (a->total_produced > 0) {
        double producer_block_rate = 100.0 * a->producer_blocks / a->total_produced;
        printf("Producer Block Rate:      %.2f%% of write attempts\n", producer_block_rate);
    }
    
    if (a->total_consumed > 0) {
        double consumer_block_rate = 100.0 * a->consumer_blocks / a->total_consumed;
        printf("Consumer Block Rate:      %.2f%% of read attempts\n", consumer_block_rate);
    }
    
    // System utilization assessment
    printf("\n--- System Utilization Assessment ---\n");
    if (a->producer_blocks > a->total_produced * 0.2) {
        printf("Queue Status:             FREQUENTLY FULL (consider increasing size)\n");
    } else if (a->consumer_blocks > a->total_consumed * 0.2) {
        printf("Queue Status:             FREQUENTLY EMPTY (underutilized/consumers starved)\n");
    } else {
        printf("Queue Status:             WELL-BALANCED\n");
    }
    
    // Efficiency metrics
    if (a->total_produced > 0 && a->total_consumed > 0) {
        double efficiency = 100.0 * a->total_consumed / a->total_produced;
        printf("System Efficiency:        %.1f%% (consumed/produced)\n", efficiency);
    }
    
    pthread_mutex_unlock(&a->mutex);
}

/*
 * Destroy analytics and free resources
 */
void analytics_destroy(Analytics *a) {
    if (a == NULL) {
        return;
    }
    
    if (DEBUG_MODE) {
        printf("[ANALYTICS] Destroying analytics structure...\n");
    }
    
    pthread_mutex_destroy(&a->mutex);
    free(a);
    
    if (DEBUG_MODE) {
        printf("[ANALYTICS] Analytics resources released\n");
    }
}

/*
 * Get current statistics snapshot (thread-safe)
 */
void analytics_get_snapshot(Analytics *a, long *produced, long *consumed, 
                            long *prod_blocks, long *cons_blocks) {
    if (a == NULL) return;
    
    pthread_mutex_lock(&a->mutex);
    
    if (produced) *produced = a->total_produced;
    if (consumed) *consumed = a->total_consumed;
    if (prod_blocks) *prod_blocks = a->producer_blocks;
    if (cons_blocks) *cons_blocks = a->consumer_blocks;
    
    pthread_mutex_unlock(&a->mutex);
}
