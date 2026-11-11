# OS File Operations Homework

Seven C programs demonstrating POSIX file operations.

## Tasks

| File | Description |
|---|---|
| task1.c | Copy a file using read/write loops and print the number of bytes copied. |
| task2.c | Write the alphabet, truncate to 10 bytes, and print the remaining content. |
| task3.c | Print a file's bytes in reverse order using lseek. |
| task4.c | Append a log entry with the process ID and print the final offset. |
| task5.c | Create a sparse file by seeking forward 1 MiB before writing. |
| task6.c | Replace the fourth number with 100 and preserve the following lines. |
| task7.c | Compare files in chunks and report the first differing byte index. |

## Compile

On Linux with GCC:

```bash
for i in 1 2 3 4 5 6 7; do
    gcc -Wall -Wextra -std=c11 "task${i}.c" -o "task${i}"
done
```

## Run

Run a program using its task number:

```bash
./task1
```

Tasks 1, 3, and 7 prompt for file paths.
Task 4 prompts for a log entry.

Tasks 2, 4, 5, and 6 use data.txt, log.txt, sparse.bin,
and numbers.txt respectively, in the current directory.

Tasks 2, 5, and 6 recreate their output files on each run.
Task 4 appends to the existing log.

For Task 1, use different source and destination files:
the destination is truncated when opened.

## Sample Input Files

```bash
printf 'Hello OS!\nFile copy test.\n' > source.txt
printf 'ABCDE' > compare1.txt
printf 'ABXDE' > compare2.txt
printf '' > empty.txt
```

## Verified Results

- Task 1 copied 26 bytes; cmp confirmed identical contents.
- Task 2 reduced the file from 26 to 10 bytes: ABCDEFGHIJ.
- Task 3 printed JIHGFEDCBA and handled an empty file.
- Task 4 preserved both log entries; the final offset matched the file size.
- Task 5 created a file with an apparent size of 1,048,584 bytes.
  Disk usage on the test server was 8 KiB.
- Task 6 replaced line 4 with 100, preserved lines 5–10,
  and produced a 23-byte file.
- Task 7 detected identical files, a difference at byte 2,
  a shorter file ending at byte 3, and identical empty files.

Task 7 uses zero-based byte indexes and returns:
- 0 for identical files
- 1 for different files
- 2 for an error

Generated files and compiled executables are excluded from Git.
