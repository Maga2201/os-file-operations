#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static int write_byte(char byte)
{
    ssize_t n;

    do {
        n = write(STDOUT_FILENO, &byte, 1);
    } while (n == -1 && errno == EINTR);

    if (n == -1) {
        perror("write");
        return -1;
    }

    if (n == 0) {
        fprintf(stderr, "Error: write made no progress.\n");
        return -1;
    }

    return 0;
}

int main(void)
{
    char *path = NULL;
    size_t capacity = 0;
    int result = EXIT_FAILURE;

    if (printf("File path: ") < 0 || fflush(stdout) == EOF) {
        perror("print prompt");
        return EXIT_FAILURE;
    }

    ssize_t length = getline(&path, &capacity, stdin);

    if (length == -1) {
        if (feof(stdin))
            fprintf(stderr, "Error: no file path provided.\n");
        else
            perror("getline");

        free(path);
        return EXIT_FAILURE;
    }

    if (length > 0 && path[length - 1] == '\n')
        path[length - 1] = '\0';

    int fd = open(path, O_RDONLY);
    free(path);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    off_t size = lseek(fd, 0, SEEK_END);

    if (size == (off_t)-1) {
        perror("lseek to end");
        goto cleanup;
    }

    /* Visit offsets size - 1, size - 2, ..., 0. */
    for (off_t position = size; position > 0; ) {
        position--;

        if (lseek(fd, position, SEEK_SET) == (off_t)-1) {
            perror("lseek");
            goto cleanup;
        }

        char byte;
        ssize_t n;

        do {
            n = read(fd, &byte, 1);
        } while (n == -1 && errno == EINTR);

        if (n == -1) {
            perror("read");
            goto cleanup;
        }

        if (n == 0) {
            fprintf(stderr, "Error: unexpected end of file.\n");
            goto cleanup;
        }

        if (write_byte(byte) == -1)
            goto cleanup;
    }

    /* For an empty file, the loop is skipped. */
    if (write_byte('\n') == -1)
        goto cleanup;

    result = EXIT_SUCCESS;

cleanup:
    if (close(fd) == -1) {
        perror("close");
        result = EXIT_FAILURE;
    }

    return result;
}

