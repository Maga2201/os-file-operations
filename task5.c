#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

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
    int result = EXIT_FAILURE;
    int fd = open("sparse.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("open for writing");
        return EXIT_FAILURE;
    }

    if (write_all(fd, "START", 5) == -1)
        goto cleanup;

    /* Skip 1 MiB from the current position (offset 5). */
    if (lseek(fd, (off_t)1024 * 1024, SEEK_CUR) == (off_t)-1) {
        perror("lseek forward");
        goto cleanup;
    }

    /*
     * Writing beyond EOF creates a hole in the skipped region.
     * On filesystems supporting sparse files, the hole does not
     * need data blocks and reads as zeros. Therefore, du reports
     * much less disk usage than the apparent file size.
     */
    if (write_all(fd, "END", 3) == -1)
        goto cleanup;

    if (close(fd) == -1) {
        perror("close after writing");
        return EXIT_FAILURE;
    }

    fd = open("sparse.bin", O_RDONLY);

    if (fd == -1) {
        perror("reopen");
        return EXIT_FAILURE;
    }

    off_t size = lseek(fd, 0, SEEK_END);

    if (size == (off_t)-1) {
        perror("lseek to end");
        goto cleanup;
    }

    if (printf("Apparent file size: %jd bytes\n",
               (intmax_t)size) < 0 || fflush(stdout) == EOF) {
        perror("print size");
        goto cleanup;
    }

    result = EXIT_SUCCESS;

cleanup:
    if (close(fd) == -1) {
        perror("close");
        result = EXIT_FAILURE;
    }

    return result;
}

