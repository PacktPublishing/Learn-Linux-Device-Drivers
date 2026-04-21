#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd;
    char sbuf[100];
    char dbuf[] = "This is a test string to write to the TEST file\n";
    ssize_t numRead, numWrite;

/*    if (argc != 2) {
        fprintf(stderr, "Usage: %s <testfile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
*/
    unlink("TEST"); // Remove the file if it exists
    system("echo 'Hello, block IO world!' > TEST"); // Create a test file with some content
    fd = open("TEST", O_RDWR | O_APPEND);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    numRead = read(fd, sbuf, sizeof(sbuf) - 1);
    if (numRead == -1) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    sbuf[numRead] = '\0'; // Null-terminate the buffer
//  printf("Read %zd bytes:\n%s\n", numRead, sbuf);

    numWrite = write(fd, dbuf, strlen(dbuf));
    if (numWrite == -1) {
        perror("write");
    }

    close(fd);
    return EXIT_SUCCESS;
}