#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int A[1024] = { 0 };
int sum = 0;

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
int page_faults = 0;
int page_allocations = 0;
int internal_fragmentation = 0; // In KB

typedef struct {
    uintptr_t start_addr;
    uintptr_t end_addr;
    void *mem_ptr;
    off_t file_offset;
} SegmentInfo;

SegmentInfo *segment_info;

void allocate_page(SegmentInfo *segment, uintptr_t fault_addr) {
    uintptr_t page_start = fault_addr & ~(getpagesize() - 1);
    void *allocated_memory = mmap((void *)page_start, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (allocated_memory == MAP_FAILED) {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }

    page_allocations++;

    size_t page_offset = page_start - segment->start_addr;
    lseek(fd, segment->file_offset + page_offset, SEEK_SET);
    read(fd, allocated_memory, getpagesize());

    segment->mem_ptr = allocated_memory;

    internal_fragmentation += (getpagesize() - page_offset % getpagesize());
}

void segfault_handler(int signal, siginfo_t *info, void *context) {
    ucontext_t *ucontext = (ucontext_t *)context;
    void *fault_address = (void *)info->si_addr;

    int i;
    for (i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uintptr_t seg_start = phdr[i].p_vaddr;
            uintptr_t seg_end = seg_start + phdr[i].p_memsz;

            if ((uintptr_t)fault_address >= seg_start && (uintptr_t)fault_address < seg_end) {
                if (!segment_info[i].mem_ptr) {
                    allocate_page(&segment_info[i], (uintptr_t)fault_address);
                    return;
                } else {
                    allocate_page(&segment_info[i], (uintptr_t)fault_address);
                    return;
                }
            }
        }
    }

    fprintf(stderr, "Segmentation fault at address %p\n", fault_address);
    exit(EXIT_FAILURE);
}

void report_statistics() {
    printf("Total Page Faults: %d\n", page_faults);
    printf("Total Page Allocations: %d\n", page_allocations);
    printf("Total Internal Fragmentation: %d KB\n", internal_fragmentation);
}

void loader_cleanup() {
    if (fd != -1) {
        close(fd);
    }
    if (ehdr != NULL) {
        free(ehdr);
    }
    if (phdr != NULL) {
        free(phdr);
    }
    if (segment_info != NULL) {
        free(segment_info);
    }
}

void load_and_run_elf(char **exe) {
    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file.");
        exit(EXIT_FAILURE);
    }

    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (ehdr == NULL) {
        perror("Error allocating memory for ELF header");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("Error reading ELF header");
        free(ehdr);
        close(fd);
        exit(EXIT_FAILURE);
    }

    phdr = (Elf32_Phdr *)malloc(ehdr->e_phentsize * ehdr->e_phnum);
    if (phdr == NULL) {
        perror("Error allocating memory for program header");
        free(ehdr);
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (lseek(fd, ehdr->e_phoff, SEEK_SET) != ehdr->e_phoff) {
        perror("Error seeking to program header table");
        free(ehdr);
        free(phdr);
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (read(fd, phdr, ehdr->e_phentsize * ehdr->e_phnum) != ehdr->e_phentsize * ehdr->e_phnum) {
        perror("Error reading program header table");
        free(ehdr);
        free(phdr);
        close(fd);
        exit(EXIT_FAILURE);
    }

    segment_info = (SegmentInfo *)malloc(ehdr->e_phnum * sizeof(SegmentInfo));
    if (segment_info == NULL) {
        perror("Error allocating memory for segment information");
        free(ehdr);
        free(phdr);
        close(fd);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < ehdr->e_phnum; i++) {
        segment_info[i].start_addr = phdr[i].p_vaddr;
        segment_info[i].end_addr = phdr[i].p_vaddr + phdr[i].p_memsz;
        segment_info[i].mem_ptr = NULL;
        segment_info[i].file_offset = phdr[i].p_offset;
    }

    int i = 0;
    void *segment_mem = NULL;

    while (i < ehdr->e_phnum) {
        if (phdr[i].p_type == PT_LOAD) {
            if (ehdr->e_entry >= phdr[i].p_vaddr && ehdr->e_entry <= phdr[i].p_vaddr + phdr[i].p_filesz) {
                segment_mem = mmap((void *)phdr[i].p_vaddr, phdr[i].p_memsz,
                                   PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
                if (segment_mem == MAP_FAILED) {
                    perror("Error allocating memory");
                    free(ehdr);
                    free(phdr);
                    free(segment_info);
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                if (segment_mem == NULL) {
                    fprintf(stderr, "Segment memory is NULL\n");
                    free(ehdr);
                    free(phdr);
                    free(segment_info);
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                if (segment_mem != (void *)phdr[i].p_vaddr) {
                    fprintf(stderr, "Segment memory mismatch\n");
                    free(ehdr);
                    free(phdr);
                    free(segment_info);
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                if (lseek(fd, phdr[i].p_offset, SEEK_SET) != phdr[i].p_offset) {
                    perror("Error seeking to segment offset");
                    free(ehdr);
                    free(phdr);
                    free(segment_info);
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                if (read(fd, segment_mem, phdr[i].p_filesz) != phdr[i].p_filesz) {
                    perror("Error reading segment content");
                    free(ehdr);
                    free(phdr);
                    free(segment_info);
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                break;
            }
        }
        i++;
    }

    int (*_start)() = (int (*)())ehdr->e_entry;
    int result = _start();
    printf("User _start return value = %d\n", result);

    if (munmap(segment_mem, phdr[i].p_memsz) == -1) {
        perror("Error unmapping memory");
    }

    free(ehdr);
    free(phdr);
    free(segment_info);
    close(fd);
    exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segfault_handler;
    sigaction(SIGSEGV, &sa, NULL);

    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv);

    report_statistics();

    return 0;
}
