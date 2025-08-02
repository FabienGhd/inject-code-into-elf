#include <stdlib.h>
#include <argp.h>
#include <err.h>
#include <string.h>
#include <time.h>
#include <bfd.h>
#include "elf_header_operations.h"
#include "elf_section_operations.h"
#include "elf_binary_operations.h"
#include "elf_utilities.h"
#include "parser.h"

#define ARGS_SIZE 5

// Holds my command-line arguments (from ARG1 to ARG5)
struct arguments
{
	char *args[ARGS_SIZE];
};

// Options structure for argp to define command-line switches.
static struct argp_option options[] = {
	{0} // Marks the end of the options array.
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;

	switch (key)
	{
	case ARGP_KEY_ARG:
		if (state->arg_num >= ARGS_SIZE)
			// Too many argument
			argp_usage(state);

		arguments->args[state->arg_num] = arg;

		break;

	case ARGP_KEY_END:
		if (state->arg_num < ARGS_SIZE)
			argp_usage(state);
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

// Setting up our argp parser
static struct argp argp =
	{
		.options = options,
		.parser = parse_opt,
		.args_doc = "ELF_FILE BINARY_FILE SECTION_NAME BASE_ADDRESS MODIFY_ENTRY",
		.doc = "\nisos_inject -- A tool to inject machine code into ELF binaries.\n"
			   "It takes five arguments:\n\n"
			   "ARG1: The ELF file that will be analyzed.\n"
			   "ARG2: A binary file that contains the machine code to be injected.\n"
			   "ARG3: The name of the newly created section.\n"
			   "ARG4: The base address of the injected code.\n"
			   "ARG5: A boolean that indicates whether the entry function should be modified or not.\n",
};

int main(int argc, char **argv)
{
	int status;
	int index_pt;
	int status_pt;
	struct arguments arguments;

	Elf64_Ehdr elf_header;
	Elf64_Shdr current_shdr;

	uint64_t base_address = 0;
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	bfd_init();
	bfd *current_file = bfd_openr(arguments.args[0], NULL);

	if (current_file == NULL)
	{
		errx(EXIT_FAILURE, "\n[-] Error : the ELF file is NULL.\n");
	}

	process_file(current_file);
	bfd_close(current_file);

	status = read_elf_header_from_file(&elf_header, arguments.args[0]);
	if (status == -1)
	{
		printf("[-] Issue encountered during the parsing of the ELF header.\n");
		return EXIT_FAILURE;
	}
	status_pt = locate_pt_note_header_index(&elf_header, arguments.args[0], &index_pt);
	if (status_pt == -1)
	{
		printf("[-] Unable to locate PT_NOTE in program headers.\n");
		return EXIT_FAILURE;
	}
	else
	{
		printf("[+] SUCCESS: PT_NOTE successfully located. \n");
	}

	long f_offset = inject_and_modify_code(arguments.args[0], arguments.args[1], arguments.args[4]);

	if (f_offset == -1)
	{
		printf("[-] An error occurred during binary injection.\n");
		return -1;
	}
	else
	{
		printf("[+] SUCCESS: Binary successfully injected at the specified offset: %ld\n", f_offset);
		base_address = adjust_base_address(arguments.args[3], f_offset, 4096);
		printf("[+] SUCCESS: Injection address with correct alignment: %ld\n", base_address);
	}

	int indexsh = find_index_of_note_ABI_section(&elf_header, &current_shdr, arguments.args[0]);
	if (indexsh <= 0)
	{
		printf("[-] Error: Unable to locate the .note.abi section.\n");
		return EXIT_FAILURE;
	}
	printf("[+] SUCCESS: Index of the .note.abi section: %d\n", indexsh);
	if (modify_section_header_properties(&elf_header, &current_shdr, indexsh,
										 get_binary_size_with_padding(arguments.args[1]), arguments.args[0],
										 base_address, f_offset) == -1)
	{
		printf("[*] Error: Cannot modify .note.abi section header.\n");
		return EXIT_FAILURE;
	}
	printf("[+] SUCCESS: .note.abi section header successfully updated.\n");
	if (reorder_section_headers_and_write(&elf_header, arguments.args[0], indexsh) == -1)
	{
		printf("[-] Error: Issue encountered while reordering the section headers.\n");
		return EXIT_FAILURE;
	}
	printf("[+] SUCCESS: Section headers successfully calibrated.\n");
	if (modify_section_name(&elf_header, arguments.args[0], arguments.args[2]) != -1)
	{
		printf("[+] SUCCESS: New section name successfully updated.\n");
	}
	if (modify_pt_note(&elf_header, &current_shdr,
					   arguments.args[0], index_pt - 1) == -1)
	{
		printf("[-] Error: Issue encountered while modifying the program header.\n");
		return EXIT_FAILURE;
	}
	printf("[+] SUCCESS: PT.NOTE program header successfully updated.\n");

	if (strcmp(arguments.args[4], "true") == 0)
	{
		if (update_entry_point(&elf_header, base_address, arguments.args[0]) == -1)
		{
			printf("[-] Problem during elf header modification.\n");
			return EXIT_FAILURE;
		}
		printf("[+] SUCCESS: ELF header successfully modified. (e_entry = %lu)\n",
			   elf_header.e_entry);
	}
	else
	{
		if (got_hijack(&elf_header, &current_shdr, arguments.args[0], base_address) != -1)
		{
			printf("[+] SUCCESS: GOT successfully hijacked.\n");
		}
	}

	exit(0);
}
