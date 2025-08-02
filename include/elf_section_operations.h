#ifndef ELF_SECTION_OPERATIONS_H
#define ELF_SECTION_OPERATIONS_H

#include <stdint.h> // For fixed-width integer types
#include "elf64_types.h" 

// Copies one section header to another.
void copy_section_header(Elf64_Shdr *destination, Elf64_Shdr *source);

// Modifies a section header to make it executable with specified properties.
int modify_section_header_properties(Elf64_Ehdr *elf_header, Elf64_Shdr *section_header, int index,
                                     int binsize, char *file, Elf64_Addr new_addr, Elf64_Off new_offset);

// Swaps two section headers.
void swap_section_headers(Elf64_Shdr *s1, Elf64_Shdr *s2);

// Sorts an array of section headers by their address.
void sort_section_headers_by_address(Elf64_Shdr tab[], int indexsh, size_t size);

// Maps an ELF file, stores all section headers in an array, and sorts them by address.
int reorder_section_headers_and_write(Elf64_Ehdr *elf_header, char *file, int indexsh);

// Modifies the name of a specified section.
int modify_section_name(Elf64_Ehdr *elf_header, char *file, char *name);

// Formats a buffer to match a given section name, adding or truncating characters as needed.
void format_buffer_for_section_name(char *buffer, const char *name);

#endif // ELF_SECTION_OPERATIONS_H
