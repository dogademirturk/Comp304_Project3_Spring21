/**
 * Doğa Demirtürk
 * İrem Şahin
 * part2.c 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 1024
#define PAGE_MASK 1023 /* TODO */

#define FRAMES 256 /* Page frame number for physical memory */

#define PAGE_SIZE 1024
#define OFFSET_BITS 10
#define OFFSET_MASK 1023 /* TODO */

#define MEMORY_SIZE FRAMES * PAGE_SIZE /* Memory size changed since frame size is decreased */

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

int MEM_FULL = 0; /* Flag indicating whether frames are full */
int OPTION = 0;   /* Flag indicating whether FIFO or LRU is in use */

struct tlbentry
{
    int logical;
    int physical;
};

struct LRU_queue_node
{
    /* Doubly linked list implementation for finding LRU elements */
    int page; /* Physical page */
    struct LRU_queue_node *next;
    struct LRU_queue_node *prev;
};

struct LRU_queue_node *head; /* Start of the LRU list */
struct LRU_queue_node *tail; /* End of the LRU list */

struct LRU_queue_node *search_LRU_queue(int physical_page)
{

    struct LRU_queue_node *ptr = head;
    /* While we are not at the end of our list, look for physical_page */
    while (ptr != NULL)
    {
        if (ptr->page == physical_page)
            /* When found, return it's pointer. */
            return ptr;
        ptr = ptr->next;
    }

    return NULL; /* If not found, return NULL */
}

void insert_LRU_queue(int physical_page)
{
    /* Inserts the physical_page into LRU list */
    if (head == NULL)
    { /* Inserts it as the head */
        struct LRU_queue_node *node = (struct LRU_queue_node *)malloc(sizeof(struct LRU_queue_node));
        node->page = physical_page;
        node->prev = NULL;
        node->next = NULL;
        head = node;
        tail = node;
        return;
    }
    /* Checks whether the given physical_page is in the LRU list already */
    struct LRU_queue_node *current = search_LRU_queue(physical_page);

    /* If it is not in the list yet, creates a new node for that page and inserts it to list */
    if (current == NULL)
    {

        struct LRU_queue_node *node = (struct LRU_queue_node *)malloc(sizeof(struct LRU_queue_node));
        node->page = physical_page;
        node->prev = tail;
        node->next = NULL;
        tail->next = node;
        tail = node;
    }
    else
    {
        /* If that page was inserted to the list before, takes it and puts it to the end of the list.
    This way, most recently used elements are kept at the end of the list, and least recently used ones 
    are stuck at the starting nodes of the list. Then, we can just pop the head node of the list whenever 
    we need to make room for a new node*/
        if(current == head) {
            head = head->next;
        } else {
            current->prev->next = current->next;
        }    

        if(current == tail) {
            tail = current->prev;
        } else {
            current->next->prev = current->prev;
        }

        current->next = NULL;
        current->prev = tail;
        if(tail != NULL) tail->next = current;
        tail = current;
    }
}

int getLRU()
{
    /* Pops the starting element of the LRU list and returns its value */
    struct LRU_queue_node *delete_node = head;
    if(head->next == NULL)
        tail = NULL;
    else
        head->next->prev = NULL;
    
    head = head->next;
	int val = delete_node->page;
    free(delete_node);

    return val;
}

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];
// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(int logical_page)
{
    /* TODO */
    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb[i].logical == logical_page)
            return tlb[i].physical;
    }
    return -1;
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(int logical, int physical)
{
    /* TODO */
    int index = tlbindex % TLB_SIZE;
    tlb[index].logical = logical;
    tlb[index].physical = physical;
    tlbindex++;
}

int main(int argc, const char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage ./virtmem backingstore input -p option\nOptions: 0 for FIFO, 1 for LRU\n");
        exit(1);
    }

    OPTION = atoi(argv[4]);

    const char *backing_filename = argv[1];
    int backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, PAGES * PAGE_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

    const char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");

    // Fill page table entries with -1 for initially empty table.
    int i;
    for (i = 0; i < PAGES; i++)
    {
        pagetable[i] = -1;
    }

    // Character buffer for reading lines of input file.
    char buffer[BUFFER_SIZE];

    // Data we need to keep track of to compute stats at end.
    int total_addresses = 0;
    int tlb_hits = 0;
    int page_faults = 0;

    // Number of the next unallocated physical page in main memory
    int free_page = 0;

    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL)
    {   
        /* TODO */
        total_addresses++;
        int logical_address = atoi(buffer);

        /* Calculate the page offset and logical page number from logical_address */
        int offset = logical_address & OFFSET_MASK;
        int logical_page = (logical_address >> 10) & PAGE_MASK;
        ///////

        int physical_page = search_tlb(logical_page);
        // TLB hit
        if (physical_page != -1)
        {
            tlb_hits++;
            // TLB miss
        }
        else
        {
            physical_page = pagetable[logical_page];
            // Page fault
            if (physical_page == -1)
            {
                /* TODO */
                page_faults++;
                physical_page = free_page;

                /* Writes the values to the main memory */
                for (int i = 0; i < PAGE_SIZE; i++)
                {
                    main_memory[physical_page * PAGE_SIZE + i] = *(backing + logical_page * PAGE_SIZE + i);
                }
                /* If memory is full, resets the physical_page's previous place.
                Then, page replacement is done */
                if (MEM_FULL)
                {
                    for (int i = 0; i < PAGES; i++)
                    {
                        if (pagetable[i] == physical_page)
                        {
                            pagetable[i] = -1;
                        }
                    }
                }
                pagetable[logical_page] = physical_page;

                if (page_faults == FRAMES) /* If page fault is equal to frame number, then the main memory is full and we need to use page replacement for next free page */
                {
                    MEM_FULL = 1;
                }
                /* If memory is full, calculates the next free page number according to our 
                current implementation: FIFO or LRU. */
                if (MEM_FULL)
                {

                    if (OPTION == 0)
                    {
                        /* If FIFO, takes the mod of the free_page with the number of frames 
                        to go back to the beginning of the table  */
                        free_page++;
                        free_page = free_page % FRAMES;
                    }
                    else
                        /* If LRU, gets the least recently used elements index and assigns it 
                        as the free page */
                        free_page = getLRU();
                }
                else
                {
                    /* If memory is not full, simply increase the free_page by one.*/
                    free_page++;
                }
            }
            /* Inserts the current physical page to LRU list */
            insert_LRU_queue(physical_page);
            add_to_tlb(logical_page, physical_page);
        }

        int physical_address = (physical_page << OFFSET_BITS) | offset;
        signed char value = main_memory[physical_page * PAGE_SIZE + offset];

        printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
    }

    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

    return 0;
}