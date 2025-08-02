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

// Find the index in the shstrtab of .not.ABI-tag section
int find_index_of_note_ABI_section(Elf64_Ehdr *elf_header, Elf64_Shdr *section_header, char *file)
{
	Elf64_Shdr *tmp;
	int filesize = elf_header->e_shoff + (elf_header->e_shentsize *
										  elf_header->e_shnum);
	int fd = -1;
	int res = -1;
	int i = 0;
	fd = open(file, O_RDONLY);
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
			copy_section_header(section_header, tmp);
			res = i;
			break;
		}
		i++;
	}

	// fd = close(fd);
	munmap(elf_file, filesize);
	return res;
}

int modify_pt_note(Elf64_Ehdr *res, Elf64_Shdr *section_header, char *file, int index)
{
	int fd = open(file, O_RDWR);
	if (fd < 0)
	{
		perror("Failed to open file");
		return -1;
	}

	Elf64_Phdr current_phdr;
	current_phdr.p_type = PT_LOAD;
	current_phdr.p_offset = section_header->sh_offset;
	current_phdr.p_vaddr = section_header->sh_addr;
	current_phdr.p_paddr = section_header->sh_addr;
	current_phdr.p_filesz = section_header->sh_size;
	current_phdr.p_memsz = section_header->sh_size;
	current_phdr.p_flags = (PF_X | PF_R);
	current_phdr.p_align = 0x1000;

	if (lseek(fd, res->e_phoff + (index * res->e_phentsize), SEEK_SET) == (off_t)-1)
	{
		perror("Failed to seek to program header");
		close(fd);
		return -1;
	}

	if (write(fd, &current_phdr, sizeof(current_phdr)) == -1)
	{
		perror("Failed to write program header");
		close(fd);
		return -1;
	}

	if (close(fd) == -1)
	{
		perror("Failed to close file descriptor");
		return -1;
	}

	return 0;
}