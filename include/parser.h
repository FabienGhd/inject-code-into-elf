#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <bfd.h>
#include "elf64_types.h"

/**
 * Determines if the provided BFD (Binary File Descriptor) points to an executable file.
 * This function checks whether the file specified by the BFD has executable attributes.
 *
 * @param file_ptr A pointer to a BFD structure representing an open file.
 * @return Returns true if the file is executable, false otherwise.
 */
bool is_executable(bfd *file_ptr);

/**
 * Checks if the file pointed to by the BFD is for a 64-bit architecture.
 * This function assesses the architecture type of the file to determine if it supports 64-bit processing.
 *
 * @param file_ptr A pointer to a BFD structure representing an open file.
 * @return Returns true if the file is of a 64-bit architecture, false otherwise.
 */
bool has_architecture64(bfd *file_ptr);

/**
 * Checks if the file associated with the BFD is in the ELF (Executable and Linkable Format) format.
 * ELF is a common standard file format for executables, object code, shared libraries, and core dumps.
 *
 * @param file_ptr A pointer to a BFD structure representing an open file.
 * @return Returns true if the file is in ELF format, false otherwise.
 */
bool is_ELF(bfd *file_ptr);

/**
 * Processes the provided BFD file by checking if it is an ELF file, 
 * determining whether it supports 64-bit architecture, and verifying if it is executable.
 * Outputs the results of these checks to the standard output.
 *
 * @param current_file A pointer to a bfd structure that represents the file to be processed.
 */
void process_file(bfd *current_file); 

#endif // PARSER_H