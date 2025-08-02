#ifndef ELF_UTILITIES_H
#define ELF_UTILITIES_H

#include <stdint.h>  
#include "elf64_types.h" 

/**
 * Finds the index of the .not.ABI-tag section in the section header string table.
 * @param elf_header Pointer to the Elf64_Ehdr structure of the ELF file.
 * @param section_header Pointer to the Elf64_Shdr structure where the section data will be copied.
 * @param file Path to the ELF file.
 * @return The index of the .not.ABI-tag section or -1 if not found or on error.
 */
int find_index_of_note_ABI_section(Elf64_Ehdr *elf_header, Elf64_Shdr *section_header, char *file);

/**
 * Modifies a program header based on a given section header's properties.
 * @param res Pointer to the Elf64_Ehdr structure of the ELF file.
 * @param section_header Pointer to the Elf64_Shdr used for updating the program header.
 * @param file Path to the ELF file.
 * @param index Index of the program header to modify.
 * @return 0 on success, or -1 if an error occurred.
 */
int modify_pt_note(Elf64_Ehdr *res, Elf64_Shdr *section_header, char *file, int index);

#endif // ELF_UTILITIES_H
