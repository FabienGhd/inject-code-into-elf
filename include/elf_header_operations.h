#ifndef ELF_HEADER_OPERATIONS_H
#define ELF_HEADER_OPERATIONS_H

#include <stdint.h> 
#include "elf64_types.h" 

/**
 * Copies the contents of a source ELF header to a destination ELF header.
 * @param destination Pointer to the destination Elf64_Ehdr structure.
 * @param source Pointer to the source Elf64_Ehdr structure.
 */
void copy_elf_header(Elf64_Ehdr *destination, Elf64_Ehdr *source);

/**
 * Reads an ELF header from a file and copies it to a provided Elf64_Ehdr structure.
 * @param header Pointer to an Elf64_Ehdr structure to store the read data.
 * @param filepath Path to the ELF file.
 * @return 0 on success, -1 on failure.
 */
int read_elf_header_from_file(Elf64_Ehdr *header, const char *filepath);

/**
 * Locates the index of the PT_NOTE program header in an ELF file.
 * @param res Pointer to the Elf64_Ehdr structure of the ELF file.
 * @param file Path to the ELF file.
 * @param index Pointer to an integer to store the index of the PT_NOTE header.
 * @return 0 on success, -1 on failure.
 */
int locate_pt_note_header_index(Elf64_Ehdr *res, char *file, int *index);

/**
 * Updates the entry point address of an ELF header and writes the changes back to the file.
 * @param ehdr Pointer to the Elf64_Ehdr structure.
 * @param new_address The new entry point address to be set.
 * @param file Path to the ELF file.
 * @return 0 on success, -1 on failure.
 */
int update_entry_point(Elf64_Ehdr *ehdr, uint64_t new_address, char *file);

#endif // ELF_HEADER_OPERATIONS_H
