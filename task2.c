#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

/* Keep writing until all requested bytes have been written. */
static int write_all(int fd, const char *buffer, size_t length)
{
    size_t total = 0;

    while (total < length) {
        ssize_t n = write(fd, buffer + total, length - total);

        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("write");
            return -1;
        }

        if (n == 0) {
            fprintf(stderr, "Error: write made no progress.\n");
            return -1;
        }

        total += (size_t)n;
    }

    return 0;
}

int main(void)
{
    const char text[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char buffer[11];
    size_t total = 0;
    int result = EXIT_FAILURE;

    int fd = open("data.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("open for writing");
        return EXIT_FAILURE;
    }

    /* Exclude the terminating '\0': write exactly 26 bytes. */
    if (write_all(fd, text, sizeof(text) - 1) == -1)
        goto cleanup;

    if (close(fd) == -1) {
        perror("close");
        return EXIT_FAILURE;
    }

    fd = open("data.txt", O_RDWR);

    if (fd == -1) {
        perror("reopen");
        return EXIT_FAILURE;
    }

    off_t size = lseek(fd, 0, SEEK_END);

    if (size == (off_t)-1) {
        perror("lseek before truncation");
        goto cleanup;
    }

    printf("Original size: %jd bytes\n", (intmax_t)size);

    /* Keep only the first 10 bytes. */
    if (ftruncate(fd, 10) == -1) {
        perror("ftruncate");
        goto cleanup;
    }

    size = lseek(fd, 0, SEEK_END);

    if (size == (off_t)-1) {
        perror("lseek after truncation");
        goto cleanup;
    }

    printf("New size: %jd bytes\n", (intmax_t)size);

    /* Move back to the beginning before reading. */
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        perror("lseek rewind");
        goto cleanup;
    }

    while (total < sizeof(buffer) - 1) {
        ssize_t n = read(fd, buffer + total,
                         sizeof(buffer) - 1 - total);

        if (n == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            goto cleanup;
        }

        if (n == 0)
            break;

        total += (size_t)n;
    }

    buffer[total] = '\0';
    printf("Remaining content: %s\n", buffer);

    result = EXIT_SUCCESS;

cleanup:
    if (close(fd) == -1) {
        perror("close");
        result = EXIT_FAILURE;
    }

    return result;
}
