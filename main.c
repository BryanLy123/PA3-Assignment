#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global variables for static sizes
#define PHYSICAL_PAGES 32
#define PAGE_TABLE_ENTRIES 128
#define OFFSET_BITS 9
#define REFERENCE_RESET_INTERVAL 200

// Page Table Entry struct
typedef struct {
    int frame_num;
    int present;
    int dirty;
    int referenced;
} PTE;

// Frame in physical memory struct
typedef struct {
    int process_id;
    int virtual_page_number;
    int load_time;
    int last_access_time;
    int dirty;
} FRAME;

// Function to extract virtual page number and offset from a virtual address
void PAGE_OFFSET(int virtual_address, int *page_number, int *offset) {
    *page_number = virtual_address >> OFFSET_BITS; // shift right by 9 bits
    *offset = virtual_address & ((1 << OFFSET_BITS) - 1);
}

// Function to simulate the Random (RAND) page replacement algorithm
void RAND(FILE *fp) {
    PTE page_tables[10][PAGE_TABLE_ENTRIES] = {0}; // Assuming max 10 processes
    FRAME physical_memory[PHYSICAL_PAGES] = {0};
    int next_available_frame = 0;
    int rand_pf = 0;
    int rand_references = 0;
    int rand_dw = 0; // dw = dirty write
    int reference_count = 0;
    char line[256];

    // Initialize random seed
    srand(1); // seed set to 1 so it can be replicated

    // Set process ID to -1 to mark frames as empty
    for (int i = 0; i < PHYSICAL_PAGES; i++) {
        physical_memory[i].process_id = -1; 
    }

    printf("\n Random (RAND) Algorithm: \n");

    // Read each line of file fp
    while (fgets(line, sizeof(line), fp)) {
        int process_id, virtual_address;
        char access_type;
        if (sscanf(line, "%d %d %c", &process_id, &virtual_address, &access_type) != 3) {
            continue;
        }
        reference_count++;

        int virtual_page_number, offset;
        PAGE_OFFSET(virtual_address, &virtual_page_number, &offset);
        
        // Check if page is present
        if (!page_tables[process_id][virtual_page_number].present) {
            rand_pf++; // if page not present, count page fault
            rand_references++; // count disk reference

            // If there are still empty frames to use
            int frame_to_use;
            if (next_available_frame < PHYSICAL_PAGES) {
                frame_to_use = next_available_frame++;
            } else {
                frame_to_use = rand() % PHYSICAL_PAGES;
                if (physical_memory[frame_to_use].process_id != -1 &&
                    page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].dirty) {
                    rand_references++;
                    rand_dw++;
                    page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].dirty = 0;
                }
                page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].present = 0;
            }

            // Update page table for the new page
            page_tables[process_id][virtual_page_number].present = 1;
            page_tables[process_id][virtual_page_number].frame_num = frame_to_use;
            page_tables[process_id][virtual_page_number].dirty = (access_type == 'W');
            page_tables[process_id][virtual_page_number].referenced = 1;

            // Update physical memory
            physical_memory[frame_to_use].process_id = process_id;
            physical_memory[frame_to_use].virtual_page_number = virtual_page_number;
            physical_memory[frame_to_use].dirty = (access_type == 'W');
        } else { //verify corresponding entry 
            page_tables[process_id][virtual_page_number].referenced = 1;
            if (access_type == 'W') {
                page_tables[process_id][virtual_page_number].dirty = 1;
                int frame_num = page_tables[process_id][virtual_page_number].frame_num;
                physical_memory[frame_num].dirty = 1;
            }
            int frame_num = page_tables[process_id][virtual_page_number].frame_num;
        }

        if (reference_count == REFERENCE_RESET_INTERVAL) {
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < PAGE_TABLE_ENTRIES; j++) {
                    page_tables[i][j].referenced = 0;
                }
            }
            reference_count = 0;
        }
    }

    printf("Total Page Faults: %d\n", rand_pf);
    printf("Total Disk References: %d\n", rand_references);
    printf("Total Dirty Page Writes: %d\n", rand_dw);
    rewind(fp); // Reset file pointer for the next simulation
}

// Funtion to simulate the First In First Out (FIFO) page replacement algorithm
void FIFO(FILE*fp){
    PTE page_tables[10][PAGE_TABLE_ENTRIES] = {0};
    FRAME physical_memory[PHYSICAL_PAGES] = {0};
    int next_available_frame = 0;
    int FIFO_pf = 0;
    int FIFO_references = 0;
    int FIFO_dw = 0;
    int reference_count = 0;
    int oldest_frame = 0;
    char line[256];

    printf("\n FIFO Algorithm: \n");

    while (fgets(line, sizeof(line), fp)){
        int process_id, virtual_address;
        char access_type;
        if (sscanf(line, "%d %d %c", &process_id, &virtual_address, &access_type) != 3) {
            continue;
        }
        reference_count++;

        int virtual_page_number, offset;
        PAGE_OFFSET(virtual_address, &virtual_page_number, &offset);
    
        if (!page_tables[process_id][virtual_page_number].present) {
            FIFO_pf++; // add page fault
            FIFO_references++; // add page reference

            int frame_to_use;
            if (next_available_frame < PHYSICAL_PAGES) {
                frame_to_use = next_available_frame++;
            } else {
                frame_to_use = oldest_frame;
                if (physical_memory[frame_to_use].process_id != -1 &&
                    page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].dirty) {
                    FIFO_references++;
                    FIFO_dw++;
                    page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].dirty = 0;
                }
                page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].present = 0;

                // Next oldest frame (+1) since oldest one was just used
                oldest_frame = (oldest_frame + 1) % PHYSICAL_PAGES;
            }

            // Update page table for the new page
            page_tables[process_id][virtual_page_number].present = 1;
            page_tables[process_id][virtual_page_number].frame_num = frame_to_use;
            page_tables[process_id][virtual_page_number].dirty = (access_type == 'W');
            page_tables[process_id][virtual_page_number].referenced = 1;

            // Update physical memory
            physical_memory[frame_to_use].process_id = process_id;
            physical_memory[frame_to_use].virtual_page_number = virtual_page_number;
            physical_memory[frame_to_use].dirty = (access_type == 'W');
        } else { 
            // Page already present
            page_tables[process_id][virtual_page_number].referenced = 1;
            if (access_type == 'W') {
                page_tables[process_id][virtual_page_number].dirty = 1;
                int frame_num = page_tables[process_id][virtual_page_number].frame_num;
                physical_memory[frame_num].dirty = 1;
            }
        }

        if (reference_count == REFERENCE_RESET_INTERVAL) {
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < PAGE_TABLE_ENTRIES; j++) {
                    page_tables[i][j].referenced = 0;
                }
            }
            reference_count = 0;
        }
    }

    printf("Total Page Faults: %d\n", FIFO_pf);
    printf("Total Disk References: %d\n", FIFO_references);
    printf("Total Dirty Page Writes: %d\n", FIFO_dw);
    rewind(fp);
}


// Function to simulate the Least Recently Used (LRU) page replacement algorithm
void LRU(FILE *fp) {
    PTE page_tables[10][PAGE_TABLE_ENTRIES] = {0};
    FRAME physical_memory[PHYSICAL_PAGES] = {0};
    int next_available_frame = 0;
    int LRU_pf = 0;
    int LRU_references = 0;
    int LRU_dw = 0;
    int reference_count = 0;
    int current_time = 0;
    char line[256];

    for (int i = 0; i < PHYSICAL_PAGES; i++) {
        physical_memory[i].process_id = -1;
    }

    printf("\n LRU Algorithm:  \n");

    while (fgets(line, sizeof(line), fp)) {
        int process_id, virtual_address;
        char access_type;
        if (sscanf(line, "%d %d %c", &process_id, &virtual_address, &access_type) != 3) {
            continue;
        }
        current_time++;
        reference_count++;

        int virtual_page_number, offset;
        PAGE_OFFSET(virtual_address, &virtual_page_number, &offset);

        if (!page_tables[process_id][virtual_page_number].present) {
            LRU_pf++; // fault check
            LRU_references++;

            int frame_to_use;
            if (next_available_frame < PHYSICAL_PAGES) {
                frame_to_use = next_available_frame++;
            } else { //LRU page location
                int lru_frame = -1; 
                int min_last_used = current_time + 1; 
                for (int i = 0; i < PHYSICAL_PAGES; i++) {
                    if (physical_memory[i].process_id != -1 && physical_memory[i].last_access_time < min_last_used) {
                        min_last_used = physical_memory[i].last_access_time;
                        lru_frame = i;
                    }
                }

                if (lru_frame != -1) {
                    frame_to_use = lru_frame;
                    if (physical_memory[frame_to_use].dirty) {
                        LRU_references++;
                        LRU_dw++;
                        page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].dirty = 0;
                    }
                    page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].present = 0;
                } else {
                    fprintf(stderr, "Error: No LRU frame found for replacement\n");
                    exit(1);
                }
            }

            page_tables[process_id][virtual_page_number].present = 1;
            page_tables[process_id][virtual_page_number].frame_num = frame_to_use;
            page_tables[process_id][virtual_page_number].dirty = (access_type == 'W');
            page_tables[process_id][virtual_page_number].referenced = 1;

            physical_memory[frame_to_use].process_id = process_id;
            physical_memory[frame_to_use].virtual_page_number = virtual_page_number;
            physical_memory[frame_to_use].last_access_time = current_time;
            physical_memory[frame_to_use].dirty = (access_type == 'W');
        } else {
            page_tables[process_id][virtual_page_number].referenced = 1;
            if (access_type == 'W') {
                page_tables[process_id][virtual_page_number].dirty = 1;
                int frame_num = page_tables[process_id][virtual_page_number].frame_num;
                physical_memory[frame_num].dirty = 1;
            }
            int frame_num = page_tables[process_id][virtual_page_number].frame_num;
            physical_memory[frame_num].last_access_time = current_time;
        }

        if (reference_count == REFERENCE_RESET_INTERVAL) {
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < PAGE_TABLE_ENTRIES; j++) {
                    page_tables[i][j].referenced = 0;
                }
            }
            reference_count = 0;
        }
    }

    printf("Total Page Faults: %d\n", LRU_pf);
    printf("Total Disk References: %d\n", LRU_references);
    printf("Total Dirty Page Writes: %d\n", LRU_dw);
    rewind(fp); // Reset file pointer for the next simulation
}

// Function to simulate the Periodic Reference Reset (PER) page replacement algorithm
void PER(FILE *fp) {
    PTE page_tables[10][PAGE_TABLE_ENTRIES] = {0};
    Frame physical_memory[PHYSICAL_PAGES] = {0};
    int next_available_frame = 0;
    int PER_pf = 0;
    int PER_references = 0;
    int PER_dw = 0;
    int reference_count = 0;
    int current_time = 0;
    char line[256];

    for (int i = 0; i < PHYSICAL_PAGES; i++) {
        physical_memory[i].process_id = -1;
    }

    printf("\n Periodic Reference Reset (PER) Algorithm: n");

    while (fgets(line, sizeof(line), fp)) {
        int process_id, virtual_address;
        char access_type;
        if (sscanf(line, "%d %d %c", &process_id, &virtual_address, &access_type) != 3) {
            continue;
        }
        current_time++;
        reference_count++;

        int virtual_page_number, offset;
        get_page_offset(virtual_address, &virtual_page_number, &offset);

        if (!page_tables[process_id][virtual_page_number].present) {
            PER_pf++; // Page Fault
            PER_references++;

            int frame_to_use;
            if (next_available_frame < PHYSICAL_PAGES) {
                frame_to_use = next_available_frame++;
            } else {
                int victim_frame = -1; //locate page

                for (int i = 0; i < PHYSICAL_PAGES; i++) { //identify unallocated pages
                    if (physical_memory[i].process_id == -1) {
                        victim_frame = i;
                        break;
                    }
                }

                if (victim_frame == -1) {
                    for (int i = 0; i < PHYSICAL_PAGES; i++) { //check for unreference and clean 
                        if (physical_memory[i].process_id != -1 &&
                            !page_tables[physical_memory[i].process_id][physical_memory[i].virtual_page_number].referenced &&
                            !page_tables[physical_memory[i].process_id][physical_memory[i].virtual_page_number].dirty) {
                            victim_frame = i;
                            break;
                        }
                    }
                }

                if (victim_frame == -1) {
                    for (int i = 0; i < PHYSICAL_PAGES; i++) { //check for unferenced and dirty
                        if (physical_memory[i].process_id != -1 &&
                            !page_tables[physical_memory[i].process_id][physical_memory[i].virtual_page_number].referenced &&
                            page_tables[physical_memory[i].process_id][physical_memory[i].virtual_page_number].dirty) {
                            victim_frame = i;
                            break;
                        }
                    }
                }

                if (victim_frame == -1) {
                    for (int i = 0; i < PHYSICAL_PAGES; i++) { //check for referenced and clean
                        if (physical_memory[i].process_id != -1 &&
                            page_tables[physical_memory[i].process_id][physical_memory[i].virtual_page_number].referenced &&
                            !page_tables[physical_memory[i].process_id][physical_memory[i].virtual_page_number].dirty) {
                            victim_frame = i;
                            break;
                        }
                    }
                }

                if (victim_frame == -1) {
                    for (int i = 0; i < PHYSICAL_PAGES; i++) { //check for referenced and dirty
                        if (physical_memory[i].process_id != -1 &&
                            page_tables[physical_memory[i].process_id][physical_memory[i].virtual_page_number].referenced &&
                            page_tables[physical_memory[i].process_id][physical_memory[i].virtual_page_number].dirty) {
                            victim_frame = i;
                            break;
                        }
                    }
                }

                if (victim_frame != -1) {
                    frame_to_use = victim_frame;
                    if (physical_memory[frame_to_use].dirty) {
                        PER_references++;
                        PER_dw++;
                        page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].dirty = 0;
                    }
                    page_tables[physical_memory[frame_to_use].process_id][physical_memory[frame_to_use].virtual_page_number].present = 0;
                } else {
                    fprintf(stderr, "Error: No victim frame found for PER replacement\n");
                    exit(1);
                }
            }

            page_tables[process_id][virtual_page_number].present = 1;
            page_tables[process_id][virtual_page_number].frame_num = frame_to_use;
            page_tables[process_id][virtual_page_number].dirty = (access_type == 'W');
            page_tables[process_id][virtual_page_number].referenced = 1;

            physical_memory[frame_to_use].process_id = process_id;
            physical_memory[frame_to_use].virtual_page_number = virtual_page_number;
            physical_memory[frame_to_use].last_access_time = current_time;
            physical_memory[frame_to_use].dirty = (access_type == 'W');
        } else {
            page_tables[process_id][virtual_page_number].referenced = 1;
            if (access_type == 'W') {
                page_tables[process_id][virtual_page_number].dirty = 1;
                int frame_num = page_tables[process_id][virtual_page_number].frame_num;
                physical_memory[frame_num].dirty = 1;
            }
            int frame_num = page_tables[process_id][virtual_page_number].frame_num;
            physical_memory[frame_num].last_access_time = current_time;
        }

        if (reference_count == REFERENCE_RESET_INTERVAL) {
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < PAGE_TABLE_ENTRIES; j++) {
                    page_tables[i][j].referenced = 0;
                }
            }
            reference_count = 0;
        }
    }

    printf("Total Page Faults: %d\n", PER_pf);
    printf("Total Disk References: %d\n", PER_references);
    printf("Total Dirty Page Writes: %d\n", PER_dw);
    rewind(fp);
}

int main() {
    char filename[20];
    int data;
    FILE *fp;

    printf("Select a data file to open:\n");
    printf("1. data1.txt\n");
    printf("2. data2.txt\n");
    printf("Enter desired dataset (1 or 2): ");
    scanf("%d", &data);

    if (data == 1) {
        strcpy(filename, "data1.txt");
    } else if (data == 2) {
        strcpy(filename, "data2.txt");
    } else {
        printf("Invalid Selection!!!\n");
        return 1;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error Opening File!!!");
        return 1;
    }

    // Prompt functions to simulation for algorithms
    RAND(fp);
    FIFO(fp);
    LRU(fp);
    PER(fp);

    fclose(fp);
    return 0;
}
