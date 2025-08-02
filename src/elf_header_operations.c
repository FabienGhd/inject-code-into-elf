#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <argp.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "elf64_types.h"
#include <bfd.h>

void copy_elf_header(Elf64_Ehdr *destination, Elf64_Ehdr *source)
{
    // Copy the source ELF header to the destination using a memory-safe operation.
    memcpy(destination, source, sizeof(Elf64_Ehdr));
}

int read_elf_header_from_file(Elf64_Ehdr *header, const char *filepath)
{
    int fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        perror("Failed to open file");
        return -1;
    }

    Elf64_Ehdr *mapped_header = mmap(NULL, sizeof(Elf64_Ehdr), PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_header == MAP_FAILED)
    {
        perror("Failed to map ELF header");
        close(fd);
        return -1;
    }

    // Copy the mapped ELF header to the provided header structure
    copy_elf_header(header, mapped_header);

    // Clean up resources
    munmap(mapped_header, sizeof(Elf64_Ehdr));
    close(fd);

    return 0;
}

int locate_pt_note_header_index(Elf64_Ehdr *res, char *file, int *index)
{
    int fd = -1;
    int status = -1;
    fd = open(file, O_RDONLY);
    if (fd < 0)
    {
        // fd = close(fd);
        return -1;
    }
    int p_header_size = (res->e_phnum * res->e_phentsize);
    uint32_t *elf_file_32 = mmap(NULL, 64 + p_header_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (elf_file_32 == MAP_FAILED)
    {
        // fd = close(fd);
        munmap(elf_file_32, 64 + p_header_size);
        return status;
    }
    for (int i = 0; i < res->e_phnum; i++)
    {
        uint32_t type = elf_file_32[PH_HEADER_ALIGNMENT + (i * FIELD_TYPE)];
        if (type == PT_NOTE_TYPE)
        {
            *index = i + 1;
            status = 0;
            break;
        }
    }
    return status;
}

int update_entry_point(Elf64_Ehdr *ehdr, uint64_t new_address, char *file)
{
    int fd = open(file, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open file");
        return -1;
    }

    // Directly modify the entry point in the passed ehdr structure
    ehdr->e_entry = new_address;

    // Seek to the beginning of the file where the ELF header starts
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("Failed to seek to beginning of file");
        close(fd);
        return -1;
    }

    // Write the modified ELF header back to the file
    if (write(fd, ehdr, sizeof(Elf64_Ehdr)) == -1)
    {
        perror("Failed to write ELF header");
        close(fd);
        return -1;
    }

    // Close the file descriptor
    if (close(fd) == -1)
    {
        perror("Failed to close file descriptor");
        return -1;
    }

    return 0;
}
