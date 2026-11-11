#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>

static int read_path(const char *prompt, char **path)
{
    size_t capacity = 0;

    if (printf("%s", prompt) < 0 || fflush(stdout) == EOF) {
        perror("print prompt");
        return -1;
    }

    ssize_t length = getline(path, &capacity, stdin);

    if (length == -1) {
        if (feof(stdin))
            fprintf(stderr, "Error: no file path provided.\n");
        else
            perror("getline");
        return -1;
    }

    if (length > 0 && (*path)[length - 1] == '\n')
        (*path)[--length] = '\0';

    if (length == 0) {
        fprintf(stderr, "Error: file path cannot be empty.\n");
        return -1;
    }

    return 0;
}

int main(void)
{
    char *source_path = NULL;
    char *destination_path = NULL;
    char buffer[4096];
    uintmax_t total = 0;

    int source_fd = -1;
    int destination_fd = -1;
    int result = EXIT_FAILURE;

    if (read_path("Source file path: ", &source_path) == -1)
        goto cleanup;

    if (read_path("Destination file path: ", &destination_path) == -1)
        goto cleanup;

    source_fd = open(source_path, O_RDONLY);

    if (source_fd == -1) {
        perror("open source");
        goto cleanup;
    }

    destination_fd = open(destination_path,
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);

    if (destination_fd == -1) {
        perror("open destination");
        goto cleanup;
    }

    while (1) {
        ssize_t bytes_read = read(source_fd, buffer, sizeof(buffer));

        if (bytes_read == -1) {
            if (errno == EINTR)
                continue;

            perror("read");
            goto cleanup;
        }

        if (bytes_read == 0)
            break;  /* End of file. */

        ssize_t written = 0;

        /* A single write() may not write the entire block. */
        while (written < bytes_read) {
            ssize_t count = write(destination_fd,
                                  buffer + written,
                                  (size_t)(bytes_read - written));

            if (count == -1) {
                if (errno == EINTR)
                    continue;

                perror("write");
                goto cleanup;
            }

            if (count == 0) {
                fprintf(stderr, "Error: write made no progress.\n");
                goto cleanup;
            }

            written += count;
            total += (uintmax_t)count;
        }
    }

    result = EXIT_SUCCESS;

cleanup:
    if (source_fd != -1 && close(source_fd) == -1) {
        perror("close source");
        result = EXIT_FAILURE;
    }

    if (destination_fd != -1 && close(destination_fd) == -1) {
        perror("close destination");
        result = EXIT_FAILURE;
    }

    if (result == EXIT_SUCCESS) {
        if (printf("Total bytes copied: %" PRIuMAX "\n", total) < 0
            || fflush(stdout) == EOF) {
            perror("print total");
            result = EXIT_FAILURE;
        }
    }

    free(source_path);
    free(destination_path);

    return result;
}

