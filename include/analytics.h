/*
 * Analytics Header File
 * Performance metrics collection and reporting
 */

#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <pthread.h>

// Analytics structure - tracks system performance metrics
typedef struct {
    long total_produced;            // Total items produced
    long total_consumed;            // Total items consumed
    long producer_blocks;           // Times producers blocked (queue full)
    long consumer_blocks;           // Times consumers blocked (queue empty/starved)
    double total_latency;           // Sum of all latencies
    double min_latency;             // Minimum latency observed
    double max_latency;             // Maximum latency observed
    long high_priority_consumed;    // Count of high priority items consumed
    long normal_priority_consumed;  // Count of normal priority items consumed
    long low_priority_consumed;     // Count of low priority items consumed
    pthread_mutex_t mutex;          // Protects analytics data
} Analytics;

// Function declarations
Analytics* analytics_init(void);
void analytics_record_produce(Analytics *a);
void analytics_record_consume(Analytics *a, double latency);
void analytics_record_consume_priority(Analytics *a, int priority, double latency);
void analytics_record_producer_block(Analytics *a);
void analytics_record_consumer_block(Analytics *a);
void analytics_print_summary(Analytics *a, double runtime, int n_producers, int n_consumers);
void analytics_destroy(Analytics *a);
void analytics_get_snapshot(Analytics *a, long *produced, long *consumed, 
                            long *prod_blocks, long *cons_blocks);

#endif
