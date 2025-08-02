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

void copy_section_header(Elf64_Shdr *destination, Elf64_Shdr *source)
{
	// Copy the source section header to the destination using a memory-safe operation.
	memcpy(destination, source, sizeof(Elf64_Shdr));
}

// Modify the current section header in order to make it executable with the
// good parameters such as the address, the offset etc..
int modify_section_header_properties(Elf64_Ehdr *elf_header, Elf64_Shdr *section_header, int index,
									 int binsize, char *file, Elf64_Addr new_addr, Elf64_Off new_offset)
{
	int fd = open(file, O_RDWR);
	if (fd < 0)
	{
		perror("Failed to open file");
		return -1;
	}

	// Update section header properties
	section_header->sh_type = SHT_PROGBITS;
	section_header->sh_addr = new_addr;
	section_header->sh_offset = new_offset;
	section_header->sh_size = binsize;
	section_header->sh_addralign = 16;		  // Enforce 16-byte alignment
	section_header->sh_flags |= SHF_EXECINST; // Set executable instructions flag

	// Set file position to the location of the section header to be modified
	if (lseek(fd, elf_header->e_shoff + (index * elf_header->e_shentsize), SEEK_SET) == -1)
	{
		perror("Failed to seek to section header");
		close(fd);
		return -1;
	}

	// Write updated section header back to the file
	if (write(fd, section_header, elf_header->e_shentsize) == -1)
	{
		perror("Failed to write section header");
		close(fd);
		return -1;
	}

	// Clean up: close file descriptor
	close(fd);
	return 0;
}

void swap_section_headers(Elf64_Shdr *s1, Elf64_Shdr *s2)
{
	Elf64_Shdr tmp;
	memcpy(&tmp, s1, sizeof(Elf64_Shdr)); // Copy s1 to tmp
	memcpy(s1, s2, sizeof(Elf64_Shdr));	  // Copy s2 to s1
	memcpy(s2, &tmp, sizeof(Elf64_Shdr)); // Copy tmp to s2
}

// Sort the section header array depending of their address
void sort_section_headers_by_address(Elf64_Shdr tab[], int indexsh, size_t size)
{
    // Ensure the array is large enough to access tab[5] and tab[6]
    if (size < 7) {
        fprintf(stderr, "Error: Insufficient number of section headers for sorting.\n");
        return; // Exit if there are not enough sections.
    }

    // Assuming each element of the tab is properly initialized prior to this function call.
    Elf64_Word dyn_sym = tab[5].sh_name;
    Elf64_Word dyn_str = tab[6].sh_name;
    int dynsym_index = 0;
    int dynstr_index = 0;

    // Perform the initial swap to start sorting.
    swap_section_headers(&tab[1], &tab[indexsh]);

    for (size_t i = 1; i < size - 3; i++)
    {
        if (tab[i].sh_addr > tab[i + 1].sh_addr)
        {
            swap_section_headers(&tab[i], &tab[i + 1]);
        }
    }

    // Recalculate indexes for dynamic symbols and strings.
    for (size_t i = 0; i < size; i++)
    {
        if (tab[i].sh_name == dyn_str)
        {
            dynstr_index = i;
        }
        if (tab[i].sh_name == dyn_sym)
        {
            dynsym_index = i;
        }
    }

    // Adjust sh_link based on new indices
    for (size_t i = 0; i < size; i++)
    {
        if (tab[i].sh_link == 5)
        {
            tab[i].sh_link = dynsym_index;
        }
        if (tab[i].sh_link == 6)
        {
            tab[i].sh_link = dynstr_index;
        }
    }
}

// Map the ELF file in the memory, store all the sections header of the file in
//	an array, sort the sections headers according to their address
int reorder_section_headers_and_write(Elf64_Ehdr *elf_header, char *file, int indexsh)
{
	Elf64_Shdr *current_shdr;
	Elf64_Shdr tab[elf_header->e_shnum];
	int filesize = elf_header->e_shoff + (elf_header->e_shentsize *
										  elf_header->e_shnum);
	int fd = -1;
	int i = 0;
	fd = open(file, O_RDWR);
	if (fd < 0)
	{
		close(fd);
		return -1;
	}
	uint8_t *elf_file = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
	if (elf_file == MAP_FAILED)
	{
		close(fd);
		munmap(elf_file, filesize);
		return -1;
	}

	while (i < elf_header->e_shnum)
	{
		current_shdr = (void *)&elf_file[elf_header->e_shoff +
										 (i * elf_header->e_shentsize)];

		copy_section_header(&tab[i], current_shdr);
		i++;
	}

	sort_section_headers_by_address(tab, indexsh, elf_header->e_shnum);
	i = 1;

	while (i < elf_header->e_shnum)
	{
		lseek(fd, elf_header->e_shoff + (i * elf_header->e_shentsize), SEEK_SET);
		if (write(fd, &tab[i], elf_header->e_shentsize) == -1)
		{
			close(fd);
			munmap(elf_file, filesize);
			return -1;
		}
		i++;
	}

	// fd = close(fd);
	munmap(elf_file, filesize);
	return 0;
}

// Format the buffer to add or truncate characters according to the original
// section header name (.not.ABI-tag)
void format_buffer_for_section_name(char *buffer, const char *name)
{
	const char *target_name = ".note.ABI-tag";
	int target_length = strlen(target_name);
	int name_length = strlen(name);

	// Copy name to buffer up to the length of target_name, or the length of name, whichever is smaller
	int copy_length = (name_length < target_length) ? name_length : target_length;
	memcpy(buffer, name, copy_length);

	// If name is shorter than target_name, pad the remaining space with spaces
	for (int i = copy_length; i < target_length; i++)
	{
		buffer[i] = ' ';
	}
}

// Modify the name of the section .not.ABI-tag
int modify_section_name(Elf64_Ehdr *elf_header, char *file, char *name)
{
	Elf64_Shdr *tmp;
	int filesize = elf_header->e_shoff + (elf_header->e_shentsize * elf_header->e_shnum);
	int fd = -1;
	int res = -1;
	int i = 0;
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
	tmp = (void *)&elf_file[elf_header->e_shoff + (elf_header->e_shstrndx * elf_header->e_shentsize)];
	int shstrtab_offset = tmp->sh_offset;
	int shstrtab_size = tmp->sh_size;
	char buffer[shstrtab_size];

	while (i < elf_header->e_shnum)
	{
		tmp = (void *)&elf_file[elf_header->e_shoff + (i * elf_header->e_shentsize)];
		int string_table_offset = shstrtab_offset + tmp->sh_name;
		snprintf(buffer, shstrtab_size, "%s", &elf_file[string_table_offset]);
		if (strcmp(buffer, ".note.ABI-tag") == 0)
		{
			lseek(fd, string_table_offset, SEEK_SET);
			format_buffer_for_section_name(buffer, name);
			if (write(fd, buffer, strlen(".note.ABI-tag")) == -1)
			{
				// fd = close(fd);
				munmap(elf_file, filesize);
				return -1;
			}
			res = 0;
			break;
		}
		i++;
	}

	// fd = close(fd);
	munmap(elf_file, filesize);
	return res;
}
