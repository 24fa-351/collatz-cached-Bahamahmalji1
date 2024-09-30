#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct CacheEntry {
    int number;
    int steps;
    int frequency;
    struct CacheEntry* prev;
    struct CacheEntry* next;
} CacheEntry;

typedef struct Cache {
    int capacity;
    int size;
    CacheEntry* head;
    CacheEntry* tail;
    char policy[10];
} Cache;

Cache* create_cache(int capacity, const char* policy) {
    Cache* cache = (Cache*)malloc(sizeof(Cache));
    cache->capacity = capacity;
    cache->size = 0;
    cache->head = cache->tail = NULL;
    strncpy(cache->policy, policy, 10);
    return cache;
}

CacheEntry* create_cache_entry(int number, int steps) {
    CacheEntry* entry = (CacheEntry*)malloc(sizeof(CacheEntry));
    entry->number = number;
    entry->steps = steps;
    entry->frequency = 1;
    entry->prev = entry->next = NULL;
    return entry;
}

void move_to_front(Cache* cache, CacheEntry* entry) {
    if (cache->head == entry) return;

    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;

    if (cache->tail == entry) cache->tail = entry->prev;

    entry->next = cache->head;
    entry->prev = NULL;
    if (cache->head) cache->head->prev = entry;
    cache->head = entry;

    if (!cache->tail) cache->tail = entry;
}

void evict(Cache* cache) {
    if (strcmp(cache->policy, "LRU") == 0 || strcmp(cache->policy, "FIFO") == 0) {
        if (cache->tail) {
            CacheEntry* old_tail = cache->tail;
            if (cache->tail->prev) {
                cache->tail = cache->tail->prev;
                cache->tail->next = NULL;
            } else {
                cache->head = cache->tail = NULL;
            }
            free(old_tail);
            cache->size--;
        }
    } else if (strcmp(cache->policy, "LFU") == 0) {
        CacheEntry* lfu_entry = cache->head;
        CacheEntry* current = cache->head;
        while (current) {
            if (current->frequency < lfu_entry->frequency) {
                lfu_entry = current;
            }
            current = current->next;
        }
        if (lfu_entry->prev) lfu_entry->prev->next = lfu_entry->next;
        if (lfu_entry->next) lfu_entry->next->prev = lfu_entry->prev;
        if (cache->head == lfu_entry) cache->head = lfu_entry->next;
        if (cache->tail == lfu_entry) cache->tail = lfu_entry->prev;
        free(lfu_entry);
        cache->size--;
    } else if (strcmp(cache->policy, "MRU") == 0) {
        if (cache->head) {
            CacheEntry* old_head = cache->head;
            cache->head = cache->head->next;
            if (cache->head) {
                cache->head->prev = NULL;
            } else {
                cache->tail = NULL;
            }
            free(old_head);
            cache->size--;
        }
    } else if (strcmp(cache->policy, "RR") == 0) {
        int random_index = rand() % cache->size;
        CacheEntry* current = cache->head;
        for (int i = 0; i < random_index; i++) {
            current = current->next;
        }
        if (current->prev) current->prev->next = current->next;
        if (current->next) current->next->prev = current->prev;
        if (cache->head == current) cache->head = current->next;
        if (cache->tail == current) cache->tail = current->prev;
        free(current);
        cache->size--;
    }
}

void add_to_cache(Cache* cache, int number, int steps) {
    if (cache->size == cache->capacity) {
        evict(cache);
    }

    CacheEntry* entry = create_cache_entry(number, steps);
    entry->next = cache->head;

    if (cache->head) {
        cache->head->prev = entry;
    }

    cache->head = entry;
    if (!cache->tail) {
        cache->tail = entry;
    }
    cache->size++;
}

CacheEntry* find_in_cache(Cache* cache, int number) {
    CacheEntry* current = cache->head;
    while (current) {
        if (current->number == number) {
            current->frequency++;
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int collatz_steps(int n) {
    int steps = 0;
    while (n != 1) {
        if (n % 2 == 0) {
            n = n / 2;
        } else {
            n = 3 * n + 1;
        }
        steps++;
    }
    return steps;
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        printf("Usage: %s <N> <MIN> <MAX> <Cache Size> <Policy>", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int MIN = atoi(argv[2]);
    int MAX = atoi(argv[3]);
    int cache_size = atoi(argv[4]);
    const char* policy = argv[5];

    Cache* cache = create_cache(cache_size, policy);

    FILE *csv_file = fopen("collatz_results.csv", "w");
    if (!csv_file) {
        printf("Failed to open file for writing.");
        perror("Error");
        return 1;
    }
    fprintf(csv_file, "Number,Steps\n");

    int cache_hits = 0;
    int cache_misses = 0;

    clock_t start, end;
    double cpu_time_used;

    start = clock();

    srand(time(NULL));
    for (int i = 0; i < N; i++) {
        int num = rand() % (MAX - MIN + 1) + MIN;

        CacheEntry* entry = find_in_cache(cache, num);
        int steps;
        if (entry) {
            steps = entry->steps;
            cache_hits++;
            if (strcmp(policy, "LRU") == 0 || strcmp(policy, "MRU") == 0) {
                move_to_front(cache, entry);
            }
            printf("Cache hit: Number: %d, Steps: %d\n", num, steps); 
        } else {
            steps = collatz_steps(num);
            cache_misses++;
            printf("Cache miss: Number: %d, Steps: %d\n", num, steps);  
            add_to_cache(cache, num, steps);
        }

        fprintf(csv_file, "%d,%d\n", num, steps);
    }

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken: %f seconds", cpu_time_used);
    double hit_percentage = ((double) cache_hits / (cache_hits + cache_misses)) * 100;
    printf("\nCache hit percentage: %.2f%%", hit_percentage);

    fclose(csv_file);

    return 0;
}
