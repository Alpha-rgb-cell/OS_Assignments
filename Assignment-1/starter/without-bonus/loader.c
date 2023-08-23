#include "loader.h"
#include <elf.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */

/*void loader_cleanup() {
  free(ehdr);
  free(phdr);
  close(fd);
  printf("Loader cleanup done\n");
}*/

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

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char **exe)
{
  fd = open(exe[1], O_RDONLY);
  // 1. Load entire binary content into the memory from the ELF file.
  // 2. Iterate through the PHDR table and find the section of PT_LOAD
  //    type that contains the address of the entrypoint method in fib.c
  // 3. Allocate memory of the size "p_memsz" using mmap function
  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"

  // Loading entire binary content into the memory from the ELF
  if (fd == -1)
  {
    perror("Error opening file.");
    exit(EXIT_FAILURE);
  }

  Elf32_Ehdr *ehdr;

  ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));


  ssize_t read_size = read(fd, ehdr, sizeof(Elf32_Ehdr));
  if (read_size != sizeof(Elf32_Ehdr))
  {
    perror("Error reading ELF header");
    free(ehdr);
    close(fd);
    exit(EXIT_FAILURE);
  }

  // Iterate through program headers to find the LOAD segment
  for (int i = 0; i < ehdr->e_phnum; ++i)
  {

    phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));

    // Elf32_Phdr ph;
    ssize_t ph_read_size = read(fd, phdr, sizeof(Elf32_Phdr));
    if (ph_read_size != sizeof(Elf32_Phdr))
    {
      perror("Error reading program header");
      close(fd);
      exit(EXIT_FAILURE);
    }

    if (phdr->p_type == PT_LOAD)
    {
      // Allocate memory and copy segment content
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
        perror("Error reading segment content"); // Clean up memory before exiting
        
        munmap(segment_addr, phdr->p_memsz);
        close(fd);
        exit(EXIT_FAILURE);
      }

      //set the entry point
      void (*entry_point)() = (void (*)())(intptr_t)((char*)segment_addr + (ehdr->e_entry - phdr->p_vaddr));
      /*entry_point = (void (*)())(intptr_t)((char*)segment_addr + (ehdr->e_entry - phdr->p_vaddr));*/

      // Clean up and exit
      munmap(segment_addr, phdr->p_memsz);
      loader_cleanup();
      return;
    }
    
    free(phdr);
  }


  // If we get here, we didn't find a LOAD segment
  perror("Error: No LOAD segment found");
  loader_cleanup();
  exit(EXIT_FAILURE);
}

/*      // Cast to function pointer and call the entry point
      void (*entry_point)() = (void (*)())(uintptr_t)(segment_addr + (ehdr->e_entry - phdr->p_vaddr));
      entry_point();
      
      

      // Clean up and exit
      munmap(segment_addr, phdr->p_memsz);
      break; // could possibly wrong
      close(fd);
      exit(EXIT_SUCCESS);
    }

    // If we get here, we didn't find a LOAD segment
    perror("Error: No LOAD segment found");
    close(fd);
    exit(EXIT_FAILURE);
  }

      int (*_start)() = (int(*)())entry_point;
  int result = _start();
  printf("User _start return value = %d\n",result);

}*/



int main(int argc, char** argv)
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);

  // 3. invoke the cleanup routine inside the loader
  loader_cleanup();
  return 0;
}
