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

#define ELF_ARCHITECTURE 64

// Checks if a given binary file descriptor points to an ELF format file.
bool is_ELF(bfd *file_ptr)
{
    if (file_ptr == NULL)
    {
        printf("Error: Null pointer provided to is_ELF.\n");
        return false;
    }

    if (!bfd_check_format(file_ptr, bfd_object))
    {
        // bfd_check_format verifies if the file matches a specific format (bfd_object), covering object files like ELF.
        printf("The opened binary is not in ELF format.\n");
        bfd_close(file_ptr);
        return false;
    }
    printf("[+] SUCCESS: The file is an ELF.\n");
    return true;
}

// Checks if the provided BFD pointer references a binary of 64-bit architecture.
bool has_architecture64(bfd *file_ptr)
{
    if (file_ptr == NULL)
    {
        printf("Error: Null pointer provided to has_architecture64.\n");
        return false;
    }
    if (bfd_get_arch_size(file_ptr) != ELF_ARCHITECTURE)
    {
        printf("The opened binary is not of 64-bit architecture.\n");
        bfd_close(file_ptr);
        return false;
    }
    return true;
}

// Checks if the provided BFD pointer references an executable file.
bool is_executable(bfd *file_ptr)
{
    if (file_ptr == NULL)
    {
        printf("Error: Null pointer provided to is_executable.\n");
        return false;
    }

    // Check if the file's flags include EXEC_P, indicating it is executable.
    if ((bfd_get_file_flags(file_ptr) & EXEC_P) == 0)
    {
        // If the EXEC_P flag is not set, print an error message to stderr.
        printf("The opened binary is not marked as executable.\n");
        return false;
    }

    return true;
}

void process_file(bfd *current_file)
{
    if (is_ELF(current_file))
    {
        if (has_architecture64(current_file))
        {
            printf("[+] SUCCESS: The file has architecture 64\n");
        }
        if (is_executable(current_file))
        {
            printf("[+] SUCCESS: The file is an executable\n");
        }
    }
    else
    {
        printf("[-] This file is not executable.\n");
    }
}