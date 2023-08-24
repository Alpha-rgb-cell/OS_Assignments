#include "loader.h"
#include <elf.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
int (*entry_point)(); //Declare Entry Point Integer

/*
 * release memory and other cleanups
 */

void loader_cleanup()
{
  if (fd != -1)
  {
    close(fd);
  }
  if (ehdr != NULL)
  {
    free(ehdr);
  }
  if (phdr != NULL)
  {
    free(phdr);
  }
}

void load_and_run_elf(char **exe)
{
  // 1. Load entire binary content into the memory from the ELF file.

  fd = open(exe[1], O_RDONLY);
  
  if (fd == -1)
  {
    perror("Error opening file.");
    exit(EXIT_FAILURE);
  }

  ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));

  ssize_t read_size = read(fd, ehdr, sizeof(Elf32_Ehdr));
  if (read_size != sizeof(Elf32_Ehdr))
  {
    perror("Error reading ELF header");
    free(ehdr);
    close(fd);
    exit(EXIT_FAILURE);
  }

  entry_point = NULL;

 // 2. Iterate through the PHDR table and find the section of PT_LOAD
 //    type that contains the address of the entrypoint method in fib.c 
  

  for (int i = 0; i < ehdr->e_phnum; ++i)
  {
    phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));

    ssize_t ph_read_size = read(fd, phdr, sizeof(Elf32_Phdr));
    if (ph_read_size != sizeof(Elf32_Phdr))
    {
      perror("Error reading program header");
      close(fd);
      exit(EXIT_FAILURE);
    }

   // 3. Allocate memory of the size "p_memsz" using mmap function
   //    and then copy the segment content

   // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
   // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
   // 6. Call the "_start" method and print the value returned from the "_start"

    if (phdr->p_type == PT_LOAD)
    {
      void *segment_addr = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
      if (segment_addr == MAP_FAILED)
      {
        perror("Error allocating memory");
        munmap(segment_addr, phdr->p_memsz);
        close(fd);
        exit(EXIT_FAILURE);
      }

      ssize_t segment_read_size = read(fd, segment_addr, phdr->p_filesz);
      if (segment_read_size != phdr->p_filesz)
      {
        perror("Error reading segment content");
        munmap(segment_addr, phdr->p_memsz);
        close(fd);
        exit(EXIT_FAILURE);
      }

        entry_point = (int (*)())(intptr_t)((char*)segment_addr + (ehdr->e_entry - phdr->p_vaddr + phdr->p_offset));

      // Clean up and exit
      munmap(segment_addr, phdr->p_memsz);
      loader_cleanup();
      return;
    }
  }

  perror("Error: No LOAD segment found");
  loader_cleanup();
  close(fd);
  exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }

  load_and_run_elf(argv);

  int result = entry_point();
  printf("User _start return value = %d\n", result);

  loader_cleanup();
  return 0;
}
