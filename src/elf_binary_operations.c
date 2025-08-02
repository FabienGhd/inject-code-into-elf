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
#include <bfd.h>
#include "elf64_types.h"
#include "elf_section_operations.h"

int get_binary_size_with_padding(char *bin)
{
    int fbin = open(bin, O_RDONLY);
    if (fbin < 0)
    {
        return -1; // Return immediately if file cannot be opened
    }

    int size = lseek(fbin, 0, SEEK_END);
    if (size == -1)
    {
        close(fbin);
        return -1;
    }

    close(fbin);     // Ensure file descriptor is closed
    return size + 8; // Include +8 only if necessary and justified
}

// Inject the assembly code in the ELF file, inject the original entry point if
// the parameter MODIFIED is true. Return the offset of the injected code
long inject_and_modify_code(char *file, char *bin, char *modified)
{
    long offset = 0;
    int fd = open(file, O_RDWR);
    if (fd < 0)
    {
        return -1;
    }

    int fbin = open(bin, O_RDWR);
    if (fbin < 0)
    {
        close(fd);
        return -1;
    }

    offset = lseek(fd, 0, SEEK_END);
    long binary_size = lseek(fbin, 0, SEEK_END);
    if (binary_size == -1)
    {
        close(fd);
        close(fbin);
        return -1;
    }

    char *buffer = malloc(binary_size);
    if (!buffer)
    {
        close(fd);
        close(fbin);
        return -1;
    }

    lseek(fbin, 0, SEEK_SET);
    if (read(fbin, buffer, binary_size) != binary_size)
    {
        free(buffer);
        close(fd);
        close(fbin);
        return -1;
    }

    if (write(fd, buffer, binary_size) != binary_size)
    {
        free(buffer);
        close(fd);
        close(fbin);
        return -1;
    }

    free(buffer);

    if (strcmp(modified, "true") == 0)
    {
        // \x68 (push immediate) followed by the address to push and then \xff\xe3 (jmp reg)
        const char opcode[] = "\x68\xe0\x22\x40\x00\x5b\xff\xe3";
        lseek(fd, 0, SEEK_END);
        if (write(fd, opcode, sizeof(opcode) - 1) == -1)
        {
            close(fd);
            close(fbin);
            return -1;
        }
    }

    close(fd);
    close(fbin);
    return offset;
}

// Map the file in the memory, localize the .got section and modify the
//   corresponding entry of getenv by the custom base address
int got_hijack(Elf64_Ehdr *elf_header,
               Elf64_Shdr *section_header, char *file, uint64_t base_address)
{
    int fd = -1;
    int shstrtab_offset = 0;
    int shstrtab_size = 0;
    int offset = 0;
    int i = 0;
    int filesize = elf_header->e_shoff +
                   (elf_header->e_shentsize * elf_header->e_shnum);
    Elf64_Shdr *tmp;

    fd = open(file, O_RDWR);
    if (fd < 0)
    {
        // fd = close(fd);
        return -1;
    }
    uint8_t *elf_file = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (elf_file == MAP_FAILED)
    {
        // fd = close(fd);
        munmap(elf_file, filesize);
        return -1;
    }
    if (elf_file == MAP_FAILED)
    {
        // fd = close(fd);
        munmap(elf_file, filesize);
        return -1;
    }
    tmp = (void *)&elf_file[elf_header->e_shoff + (elf_header->e_shstrndx * elf_header->e_shentsize)];
    shstrtab_offset = tmp->sh_offset;
    shstrtab_size = tmp->sh_size;
    char buffer[shstrtab_size];

    while (i < elf_header->e_shnum)
    {
        tmp = (void *)&elf_file[elf_header->e_shoff + (i * elf_header->e_shentsize)];
        int string_table_offset = shstrtab_offset + tmp->sh_name;
        snprintf(buffer, shstrtab_size, "%s", &elf_file[string_table_offset]);

        if (strcmp(buffer, ".got.plt") == 0)
        {
            copy_section_header(section_header, tmp);
            break;
        }
        i++;
    }
    offset = tmp->sh_offset;

    lseek(fd, offset + GET_ENV_OFFSET, SEEK_SET);
    if (write(fd, &base_address, 8) == -1)
    {
        // fd = close(fd);
        munmap(elf_file, filesize);
        return -1;
    }
    // fd = close(fd);
    munmap(elf_file, filesize);
    return 0;
}

uint64_t adjust_base_address(const char *arg_base_address, long file_offset, int alignment)
{
    char *endptr;
    uint64_t base_address = strtoull(arg_base_address, &endptr, 10);
    if (*endptr != '\0')
    {
        fprintf(stderr, "Error: Invalid base address input '%s'.\n", arg_base_address);
        return 0; // Return 0 or another specific error code to indicate failure.
    }

    int shift = (file_offset - base_address) % alignment;
    if (shift != 0)
    {
        base_address += shift;
    }

    return base_address;
}
