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
    char line[1024];
    char entry[1100];
    size_t length = 0;
    int result = EXIT_FAILURE;

    int fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    const char prompt[] = "Enter a log entry: ";

    if (write_all(STDOUT_FILENO, prompt, sizeof(prompt) - 1) == -1)
        goto cleanup;

    /* Read one line using read(), stopping at newline or EOF. */
    while (length < sizeof(line)) {
        char byte;
        ssize_t n = read(STDIN_FILENO, &byte, 1);

        if (n == -1) {
            if (errno == EINTR)
                continue;

            perror("read");
            goto cleanup;
        }

        if (n == 0)
            break;

        line[length++] = byte;

        if (byte == '\n')
            break;
    }

    int prefix_length = snprintf(entry, sizeof(entry),
                                 "PID=%jd: ", (intmax_t)getpid());

    if (prefix_length < 0 ||
        (size_t)prefix_length >= sizeof(entry)) {
        fprintf(stderr, "Error: could not format PID prefix.\n");
        goto cleanup;
    }

    size_t entry_length = (size_t)prefix_length;

    if (entry_length + length + 1 > sizeof(entry)) {
        fprintf(stderr, "Error: log entry is too long.\n");
        goto cleanup;
    }

    for (size_t i = 0; i < length; i++)
        entry[entry_length++] = line[i];

    if (length == 0 || line[length - 1] != '\n')
        entry[entry_length++] = '\n';

    if (write_all(fd, entry, entry_length) == -1)
        goto cleanup;

    /*
     * O_APPEND moves the offset to EOF before each write.
     * The write then advances the offset past the written bytes.
     * Therefore, SEEK_CUR returns a growing offset as we append.
     */
    off_t offset = lseek(fd, 0, SEEK_CUR);

    if (offset == (off_t)-1) {
        perror("lseek");
        goto cleanup;
    }

    if (printf("Final file offset: %jd bytes\n",
               (intmax_t)offset) < 0 || fflush(stdout) == EOF) {
        perror("print offset");
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

