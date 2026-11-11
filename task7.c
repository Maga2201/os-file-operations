#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
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
        fprintf(stderr, "Error: empty file path.\n");
        return -1;
    }

    return 0;
}

/*
 * Fill a chunk unless EOF is reached.
 * A short read alone does not necessarily mean EOF.
 */
static ssize_t read_chunk(int fd, unsigned char *buffer, size_t capacity)
{
    size_t total = 0;

    while (total < capacity) {
        ssize_t n = read(fd, buffer + total, capacity - total);

        if (n == -1) {
            if (errno == EINTR)
                continue;

            return -1;
        }

        if (n == 0)
            break;

        total += (size_t)n;
    }

    return (ssize_t)total;
}

int main(void)
{
    char *path1 = NULL;
    char *path2 = NULL;
    unsigned char buffer1[4096];
    unsigned char buffer2[4096];

    int fd1 = -1;
    int fd2 = -1;
    int result = 2;  /* 0 = identical, 1 = different, 2 = error */
    uintmax_t offset = 0;

    if (read_path("First file path: ", &path1) == -1)
        goto cleanup;

    if (read_path("Second file path: ", &path2) == -1)
        goto cleanup;

    fd1 = open(path1, O_RDONLY);

    if (fd1 == -1) {
        perror("open first file");
        goto cleanup;
    }

    fd2 = open(path2, O_RDONLY);

    if (fd2 == -1) {
        perror("open second file");
        goto cleanup;
    }

    while (1) {
        ssize_t n1 = read_chunk(fd1, buffer1, sizeof(buffer1));

        if (n1 == -1) {
            perror("read first file");
            goto cleanup;
        }

        ssize_t n2 = read_chunk(fd2, buffer2, sizeof(buffer2));

        if (n2 == -1) {
            perror("read second file");
            goto cleanup;
        }

        size_t common = (size_t)(n1 < n2 ? n1 : n2);

        for (size_t i = 0; i < common; i++) {
            if (buffer1[i] != buffer2[i]) {
                printf("Files differ at byte %ju\n",
                       offset + (uintmax_t)i);
                result = 1;
                goto cleanup;
            }
        }

        /* Matching prefix, but one file ended earlier. */
        if (n1 != n2) {
            printf("Files differ at byte %ju\n",
                   offset + (uintmax_t)common);
            result = 1;
            goto cleanup;
        }

        /* Both files reached EOF with no differences. */
        if (n1 == 0) {
            printf("Files are identical\n");
            result = 0;
            goto cleanup;
        }

        offset += (uintmax_t)n1;
    }

cleanup:
    if (fd1 != -1 && close(fd1) == -1) {
        perror("close first file");
        result = 2;
    }

    if (fd2 != -1 && close(fd2) == -1) {
        perror("close second file");
        result = 2;
    }

    if (fflush(stdout) == EOF) {
        perror("flush stdout");
        result = 2;
    }

    free(path1);
    free(path2);

    return result;
}
