#ifndef ELF_OPERATIONS_H
#define ELF_OPERATIONS_H

#include <stdint.h>  // For uint64_t, int64_t
#include "elf64_types.h" 

/**
 * Returns the size of a binary file with an additional padding of 8 bytes.
 * @param bin The path to the binary file.
 * @return The size of the file plus an additional 8 bytes or -1 if an error occurred.
 */
int get_binary_size_with_padding(char *bin);

/**
 * Injects assembly code into an ELF file and optionally modifies the original entry point.
 * @param file Path to the target ELF file.
 * @param bin Path to the binary file containing the code to inject.
 * @param modified A string that indicates if the original entry point should be modified ("true" or other).
 * @return The offset at which the code was injected or -1 if an error occurred.
 */
long inject_and_modify_code(char *file, char *bin, char *modified);

/**
 * Modifies a .got section entry to a custom base address in an ELF file.
 * @param elf_header A pointer to the Elf64_Ehdr structure of the ELF file.
 * @param section_header A pointer to the Elf64_Shdr structure of the section to be hijacked.
 * @param file The path to the ELF file.
 * @param base_address The new base address to set for the .got section entry.
 * @return 0 on success, or -1 if an error occurred.
 */
int got_hijack(Elf64_Ehdr *elf_header, Elf64_Shdr *section_header, char *file, uint64_t base_address);

/**
 * Adjusts the base address to ensure it aligns with a given alignment requirement based on a file offset.
 * @param arg_base_address The string representation of the initial base address.
 * @param file_offset The offset of the file where the code was injected.
 * @param alignment The alignment requirement (typically a power of 2).
 * @return The adjusted base address.
 */
uint64_t adjust_base_address(const char *arg_base_address, long file_offset, int alignment);


#endif // ELF_OPERATIONS_H
