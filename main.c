#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"
#include <time.h>

struct coro_context {
    char *file_name;
    long coro_overall_time;
    long coro_last_yield_time;
};

void merge(
        int arr[],
        const int left[],
        int leftSize,
        const int right[],
        int rightSize
) {

    int i = 0, j = 0, k = 0;

    while (i < leftSize && j < rightSize)
        if (left[i] <= right[j])
            arr[k++] = left[i++];
        else
            arr[k++] = right[j++];

    while (i < leftSize)
        arr[k++] = left[i++];

    while (j < rightSize)
        arr[k++] = right[j++];
}

void mergeSort(
        int arr[],
        int size,
        void *context
) {

    struct timespec current_time;
    struct coro_context *new_coro_context = (struct coro_context*) context;

    if (size < 2)
        return;

    int mid = size / 2;
    int *left = (int *) calloc(mid, sizeof(int));
    int *right = (int *) calloc(size - mid, sizeof(int));

    for (int i = 0; i < mid; i++)
        left[i] = arr[i];

    for (int i = mid; i < size; i++)
        right[i - mid] = arr[i];

    mergeSort(left, mid, context);
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    new_coro_context -> coro_overall_time += current_time.tv_sec * 1000 * 1000 * 1000 + current_time.tv_nsec - new_coro_context -> coro_last_yield_time;
    coro_yield();
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    new_coro_context -> coro_last_yield_time = current_time.tv_sec * 1000 * 1000 * 1000 + current_time.tv_nsec;

    mergeSort(right, size - mid, context);
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    new_coro_context -> coro_overall_time += current_time.tv_sec * 1000 * 1000 * 1000 + current_time.tv_nsec - new_coro_context -> coro_last_yield_time;
    coro_yield();
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    new_coro_context -> coro_last_yield_time = current_time.tv_sec * 1000 * 1000 * 1000 + current_time.tv_nsec;

    merge(arr, left, mid, right, size - mid);
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    new_coro_context -> coro_overall_time += current_time.tv_sec * 1000 * 1000 * 1000 + current_time.tv_nsec - new_coro_context -> coro_last_yield_time;
    coro_yield();
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    new_coro_context -> coro_last_yield_time = current_time.tv_sec * 1000 * 1000 * 1000 + current_time.tv_nsec;

    free(left);
    free(right);
}

void mergeFiles(
        const char *outputFile,
        char *inputFiles[],
        int numFiles
) {

    FILE *in_files[numFiles];
    FILE *out_file;
    int i, j, min_val, min_val_file;

    for (i = 0; i < numFiles; i++) {
        in_files[i] = fopen(inputFiles[i], "r");
        if (in_files[i] == NULL) {
            printf("Error opening file %s.\n", inputFiles[i]);
            exit(1);
        }
    }

    out_file = fopen(outputFile, "w");
    if (out_file == NULL) {
        printf("Error creating output file.\n");
        exit(1);
    }

    int current_elements[numFiles];

    for (i = 0; i < numFiles; i++) {
        if (!feof(in_files[i])) {
            fscanf(in_files[i], "%d", &j);
            current_elements[i] = j;
        }
    }

    while (1) {

        min_val = -1;
        min_val_file = -1;

        for (i = 0; i < numFiles; i++) {
            if ((min_val == -1 || current_elements[i] < min_val) && current_elements[i] != -1) {
                min_val = current_elements[i];
                min_val_file = i;
            }
        }

        if (min_val_file == -1)
            break;

        fprintf(out_file, "%d\n", min_val);

        if (fscanf(in_files[min_val_file], "%d", &j) == 1)
            current_elements[min_val_file] = j;
        else
            current_elements[min_val_file] = -1;
    }

    for (i = 0; i < numFiles; i++) {
        fclose(in_files[i]);
    }

    fclose(out_file);

    printf("Files merged successfully into output.txt.\n");
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int
coroutine_func_f(void *context)
{
    struct coro_context *new_coro_context = (struct coro_context*) context;

    FILE *file;
    int num, count = 0;

    file = fopen(new_coro_context -> file_name, "r");
    if (file == NULL) {
        printf("Failed to open the file.\n");
        return 1;
    }

    while (fscanf(file, "%d", &num) == 1)
        count++;

    int *arr = (int *)malloc(count * sizeof(int));
    if (arr == NULL) {
        printf("Memory allocation failed.\n");
        fclose(file);
        return 1;
    }

    fseek(file, 0, SEEK_SET);

    for (int i = 0; i < count; i++) {
        if (fscanf(file, "%d", &arr[i]) != 1) {
            printf("Failed to read integer from the file.\n");
            fclose(file);
            free(arr);
            return 1;
        }
    }

    fclose(file);

    mergeSort(arr, count, context);

    file = fopen(new_coro_context -> file_name, "w");
    if (file == NULL) {
        printf("Failed to open the file for writing.\n");
        free(arr);
        return 1;
    }

    for (int i = 0; i < count; i++)
        fprintf(file, "%d ", arr[i]);

    fclose(file);
    free(arr);

    return 0;
}

int
main(int argc, char **argv)
{

    if (argc < 2) {
        printf("No files for sorting provided\n");
        return 1;
    }

    struct coro_context *new_coro_context = (struct coro_context*) calloc(argc, sizeof(struct coro_context));

    coro_sched_init();

    for (int i = 1; i < argc; ++i) {
        printf("%s\n", argv[i]);

        struct timespec coro_start_time;
        clock_gettime(CLOCK_MONOTONIC, &coro_start_time);

        new_coro_context[i].file_name = argv[i];
        new_coro_context[i].coro_overall_time = 0;
        new_coro_context[i].coro_last_yield_time = coro_start_time.tv_sec * 1000 * 1000 * 1000 + coro_start_time.tv_nsec;

        coro_new(coroutine_func_f, &new_coro_context[i]);
    }

    struct coro *c;
    while ((c = coro_sched_wait()) != NULL) {
        printf("Finished %d\n", coro_status(c));
        coro_delete(c);
    }

    char *inputFiles[argc];
    const char *outputFile = "output.txt";

    for (int i = 0; i < argc - 1; i++)
        inputFiles[i] = argv[i + 1];

    mergeFiles(outputFile, inputFiles, argc - 1);

    for (int i = 1; i < argc; i++) {
        printf("coro_%d -> %ld ns\n", i, new_coro_context[i].coro_overall_time);
    }

    free(new_coro_context);

    return 0;
}