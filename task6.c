#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

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

static ssize_t read_retry(int fd, char *buffer, size_t length)
{
    ssize_t n;

    do {
        n = read(fd, buffer, length);
    } while (n == -1 && errno == EINTR);

    return n;
}

int main(void)
{
    const char numbers[] = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n";
    char remainder[sizeof(numbers)];
    char buffer[128];
    size_t remainder_length = 0;
    int result = EXIT_FAILURE;

    int fd = open("numbers.txt",
                  O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("open for writing");
        return EXIT_FAILURE;
    }

    if (write_all(fd, numbers, sizeof(numbers) - 1) == -1)
        goto cleanup;

    if (close(fd) == -1) {
        perror("close after writing");
        return EXIT_FAILURE;
    }

    fd = open("numbers.txt", O_RDWR);

    if (fd == -1) {
        perror("reopen");
        return EXIT_FAILURE;
    }

    /* Find line 4 by reading until we count three newlines. */
    int newlines = 0;
    off_t line4_offset = 0;

    while (newlines < 3) {
        char byte;
        ssize_t n = read_retry(fd, &byte, 1);

        if (n == -1) {
            perror("read while finding line 4");
            goto cleanup;
        }

        if (n == 0) {
            fprintf(stderr, "Error: line 4 does not exist.\n");
            goto cleanup;
        }

        line4_offset++;

        if (byte == '\n')
            newlines++;
    }

    /* Read past the old fourth line, including its newline. */
    while (1) {
        char byte;
        ssize_t n = read_retry(fd, &byte, 1);

        if (n == -1) {
            perror("read old line 4");
            goto cleanup;
        }

        if (n == 0) {
            fprintf(stderr, "Error: incomplete fourth line.\n");
            goto cleanup;
        }

        if (byte == '\n')
            break;
    }

    /*
     * Save lines 5 through 10.
     * This buffer is sufficient for the fixed file created above.
     */
    while (1) {
        char byte;
        ssize_t n = read_retry(fd, &byte, 1);

        if (n == -1) {
            perror("read remainder");
            goto cleanup;
        }

        if (n == 0)
            break;

        if (remainder_length == sizeof(remainder)) {
            fprintf(stderr, "Error: remainder buffer is full.\n");
            goto cleanup;
        }

        remainder[remainder_length++] = byte;
    }

    if (lseek(fd, line4_offset, SEEK_SET) == (off_t)-1) {
        perror("lseek to line 4");
        goto cleanup;
    }

    /* Rewrite the fourth line and shift the saved text forward. */
    if (write_all(fd, "100\n", 4) == -1)
        goto cleanup;

    if (write_all(fd, remainder, remainder_length) == -1)
        goto cleanup;

    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        perror("lseek to beginning");
        goto cleanup;
    }

    const char heading[] = "Final file content:\n";

    if (write_all(STDOUT_FILENO, heading, sizeof(heading) - 1) == -1)
        goto cleanup;

    while (1) {
        ssize_t n = read_retry(fd, buffer, sizeof(buffer));

        if (n == -1) {
            perror("read final content");
            goto cleanup;
        }

        if (n == 0)
            break;

        if (write_all(STDOUT_FILENO, buffer, (size_t)n) == -1)
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
