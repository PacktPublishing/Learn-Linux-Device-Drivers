#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <ctype.h>

#define MAP_SIZE 4096

void hex_dump(const void *buffer, size_t length)
{
    const unsigned char *buf = (const unsigned char *)buffer;
    size_t offset = 0;

    while (offset < length) {
        // Print offset
        printf("%08zx  ", offset);

        // Print hex bytes
        for (size_t i = 0; i < 16; ++i) {
            if (offset + i < length) {
                printf("%02x ", buf[offset + i]);
            } else {
                printf("   ");
            }

            // Add extra space between 8-byte groups
            if (i == 7) printf(" ");
        }

        printf(" |");

        // Print ASCII representation
        for (size_t i = 0; i < 16 && offset + i < length; ++i) {
            unsigned char c = buf[offset + i];
            printf("%c", isprint(c) ? c : '.');
        }

        printf("|\n");
        offset += 16;
    }
}

int main(int argc, char **argv)
{

    if (argc < 4) {
	fprintf(stderr, "Usage: %s uio-device start-offset num-bytes-to-display\n", argv[0]);
	exit(1);
    }

    int fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    void *base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

/*
    uint32_t val = *((volatile uint32_t *)base);
    printf("Read value: 0x%08x\n", val);
*/
    // Optional: write something (check whether MMIO region is writable)
    // *((volatile uint32_t *)base) = 0xbeaaface;

    hex_dump(base+atoi(argv[2]), atoi(argv[3]));

    munmap(base, MAP_SIZE);
    close(fd);
    exit(0);
}
