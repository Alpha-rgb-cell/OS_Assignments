#include "loader.h"
#include <elf.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;


void loader_cleanup() {
    if (fd != -1) {
        close(fd); // Close the file descriptor if it's open
    }
    if (ehdr != NULL) {
        free(ehdr); // free allocated memory for ELF header
    }
    if (phdr != NULL) {
        free(phdr); // Free allocated memory for program header
    }
}

// ld and execute an ELF executable
void load_and_run_elf(char **exe) {
    // Open the ELF file for reading
    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file.");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for and read the ELF header
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    read(fd, ehdr, sizeof(Elf32_Ehdr));

    // Allocate memory for and read the program header table
    phdr = (Elf32_Phdr *)malloc(ehdr->e_phentsize * ehdr->e_phnum);
    lseek(fd, ehdr->e_phoff, SEEK_SET);
    read(fd, phdr, ehdr->e_phentsize * ehdr->e_phnum);

    int i = 0;
    while (i < ehdr->e_phnum) {
        if (phdr[i].p_type == PT_LOAD) { // Check for PT_LOAD segment
            if (ehdr->e_entry >= phdr[i].p_vaddr && ehdr->e_entry <= phdr[i].p_vaddr + phdr[i].p_filesz) {
                // load ELF file segment content and mapping
                void *virtual_mem = mmap((void *)phdr[i].p_vaddr, phdr[i].p_memsz,
                PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
                if (virtual_mem == MAP_FAILED) {
                    perror("Error mapping memory");
                    close(fd);
                    loader_cleanup();
                    exit(EXIT_FAILURE);
                }
                
                // read segment content from ELF file and ld into mem
                lseek(fd, phdr[i].p_offset, SEEK_SET);
                read(fd, virtual_mem, phdr[i].p_filesz);

                // calculate entry point offset and execute _start function
                uintptr_t entry_offset = ehdr->e_entry - phdr[i].p_vaddr;
                char *entry_address = (char *)((uintptr_t)virtual_mem + entry_offset);
                int (*_start)() = (int (*)())entry_address;
                int result = _start();
                printf("User _start return value = %d\n", result);

                // Clean up: unmap memory
                munmap(virtual_mem, phdr[i].p_memsz);
                break; // Exit the loop after the first PT_LOAD segment is found
            }
        }
        i++;
    }

    // Clean up resources
    loader_cleanup();
    close(fd);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv);

    loader_cleanup();
    return 0;
}
