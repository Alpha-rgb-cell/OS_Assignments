#include "loader.h"
#include <elf.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  // 1. Release memory allocated for ELF header
  // 2. Release memory allocated for program header table
  // 3. Close the file descriptor
  free(ehdr);
  free(phdr);
  close(fd);
  printf("Loader cleanup done\n");
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  fd = open(argv[1], O_RDONLY);
  // 1. Load entire binary content into the memory from the ELF file.
  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"


//Loading entire binary content into the memory from the ELF
  if (fd == -1) {
        perror("Error opening file.");
        exit(EXIT_FAILURE);
    }

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) malloc(sizeof(Elf32_Ehdr));//This code maybe wrong
    if (ehdr == NULL) {
        perror("Error allocating memory.");
        close(fd);
        exit(EXIT_FAILURE);
    }

    ssize_t read_size = read(fd, &ehdr, sizeof(Elf32_Ehdr));
    if (read_size != sizeof(Elf32_Ehdr)) {
        perror("Error reading ELF header");
        free(ehdr);
        close(fd);
        exit(EXIT_FAILURE);
    }

// Iterate through program headers to find the LOAD segment
    for (int i = 0; i < ehdr.e_phnum; ++i) {
        Elf32_Phdr ph;
        ssize_t ph_read_size = read(fd, &ph, sizeof(Elf32_Phdr));
        if (ph_read_size != sizeof(Elf32_Phdr)) {
            perror("Error reading program header");
            close(fd);
            exit(EXIT_FAILURE);
    }


    if (ph.p_type == PT_LOAD) {
        // Allocate memory and copy segment content
        void* segment_addr = mmap(NULL, ph.p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (segment_addr == MAP_FAILED) {
              perror("Error allocating memory");
              munmap(segment_addr, ph.p_memsz);
              close(fd);
              exit(EXIT_FAILURE);
          }

        ssize_t segment_read_size = read(fd, segment_addr, ph.p_filesz);
        if (segment_read_size != ph.p_filesz) {
            perror("Error reading segment content"); // Clean up memory before exiting
            close(fd);
            exit(EXIT_FAILURE);
        }

      // Cast to function pointer and call the entry point
        void (*entry_point)() = (void (*)())(uintptr_t)(segment_addr + (ehdr.e_entry - ph.p_vaddr));
        entry_point();

        

        // Clean up and exit
        munmap(segment_addr, ph.p_memsz);
        break; //could possibly wrong
        close(fd);
        exit(EXIT_SUCCESS);
    }
  

    // If we get here, we didn't find a LOAD segment
    perror("Error: No LOAD segment found");
    close(fd);
    exit(EXIT_FAILURE);




  int result = _start();
  printf("User _start return value = %d\n",result);

  // 7. Release memory and other resources
  loader_cleanup();
  return 0;

}
