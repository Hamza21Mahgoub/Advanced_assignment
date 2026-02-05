/*
 * Utility Functions Implementation
 * 
 * Provides helper functions for random number generation, timing,
 * and system information display.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <string.h>
#include "utils.h"
#include "config.h"

// Static flag to ensure random seed is initialized only once
static int random_initialized = 0;

/*
 * Initialize random number generator with current time
 * Called automatically by random_range if not already initialized
 */
static void init_random(void) {
    if (!random_initialized) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        srand((unsigned int)(tv.tv_sec * 1000000 + tv.tv_usec + getpid()));
        random_initialized = 1;
        
        if (DEBUG_MODE) {
            printf("[UTILS] Random number generator initialized\n");
        }
    }
}

/*
 * Generate a random integer in range [min, max] inclusive
 * Uses global random seed
 */
int random_range(int min, int max) {
    if (!random_initialized) {
        init_random();
    }
    
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    
    if (min == max) {
        return min;
    }
    
    // Generate random number in range
    int range = max - min + 1;
    int random_val = (rand() % range) + min;
    
    return random_val;
}

/*
 * Generate a random integer using a provided seed (thread-safe)
 * Useful for per-thread random generation without interference
 */
int random_range_seed(int min, int max, unsigned int *seed) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    
    if (min == max) {
        return min;
    }
    
    // Generate random number in range using thread-local seed
    int range = max - min + 1;
    int random_val = (rand_r(seed) % range) + min;
    
    return random_val;
}

/*
 * Get current timestamp in seconds (with microsecond precision)
 */
double get_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

/*
 * Sleep for a random duration up to max_seconds
 */
void sleep_random(int max_seconds) {
    if (max_seconds <= 0) {
        return;
    }
    
    int sleep_time = random_range(1, max_seconds);
    sleep(sleep_time);
}

/*
 * Get current time as a formatted string
 */
char* get_current_time_string(void) {
    static char time_buffer[64];
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    
    if (timeinfo == NULL) {
        snprintf(time_buffer, sizeof(time_buffer), "[Time unavailable]\n");
        return time_buffer;
    }
    
    // Format: "Wed Feb 04 15:30:45 2026\n"
    strftime(time_buffer, sizeof(time_buffer), "%a %b %d %H:%M:%S %Y\n", timeinfo);
    
    return time_buffer;
}

/*
 * Print system information (username, hostname, time/date)
 * Demonstrates use of system calls getpwuid(), getuid(), gethostname()
 */
void print_system_info(void) {
    char hostname[HOST_NAME_MAX + 1];
    struct passwd *pw;
    time_t now;
    struct tm *timeinfo;
    
    printf("--- System Information ---\n");
    
    // Get current username
    pw = getpwuid(getuid());
    if (pw != NULL) {
        printf("Username:              %s\n", pw->pw_name);
    } else {
        fprintf(stderr, "Warning: Could not retrieve username\n");
        printf("Username:              [Unknown]\n");
    }
    
    // Get hostname
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        hostname[HOST_NAME_MAX] = '\0'; // Ensure null termination
        printf("Hostname:              %s\n", hostname);
    } else {
        fprintf(stderr, "Warning: Could not retrieve hostname\n");
        printf("Hostname:              [Unknown]\n");
    }
    
    // Get current time and date
    time(&now);
    timeinfo = localtime(&now);
    if (timeinfo != NULL) {
        char time_str[100];
        strftime(time_str, sizeof(time_str), "%A, %B %d, %Y at %H:%M:%S", timeinfo);
        printf("Execution Time:        %s\n", time_str);
    } else {
        fprintf(stderr, "Warning: Could not retrieve current time\n");
        printf("Execution Time:        [Unknown]\n");
    }
    
    // Get process ID
    printf("Process ID (PID):      %d\n", getpid());
}

/*
 * Print runtime parameters in a formatted manner
 */
void print_run_parameters(int n_producers, int n_consumers, int queue_size, int timeout) {
    printf("Number of Producers:   %d\n", n_producers);
    printf("Number of Consumers:   %d\n", n_consumers);
    printf("Queue Capacity:        %d entries\n", queue_size);
    printf("Timeout Duration:      %d seconds\n", timeout);
}

/*
 * Convert seconds to hours:minutes:seconds format
 */
void format_time_hms(double seconds, char *buffer, size_t buffer_size) {
    int hours = (int)(seconds / 3600);
    int minutes = (int)((seconds - hours * 3600) / 60);
    int secs = (int)(seconds - hours * 3600 - minutes * 60);
    int millis = (int)((seconds - (int)seconds) * 1000);
    
    if (hours > 0) {
        snprintf(buffer, buffer_size, "%02d:%02d:%02d.%03d", hours, minutes, secs, millis);
    } else if (minutes > 0) {
        snprintf(buffer, buffer_size, "%02d:%02d.%03d", minutes, secs, millis);
    } else {
        snprintf(buffer, buffer_size, "%d.%03d sec", secs, millis);
    }
}