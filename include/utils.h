/*
 * Utilities Header File
 * Helper functions for random numbers, timing, and system info
 */

#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include <stddef.h>

// HOST_NAME_MAX might not be defined on all systems
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

// Random number generation
int random_range(int min, int max);
int random_range_seed(int min, int max, unsigned int *seed);

// Timing utilities
double get_timestamp(void);
void sleep_random(int max_seconds);
char* get_current_time_string(void);
void format_time_hms(double seconds, char *buffer, size_t buffer_size);

// System information
void print_system_info(void);
void print_run_parameters(int n_producers, int n_consumers, 
                         int queue_size, int timeout);

#endif
