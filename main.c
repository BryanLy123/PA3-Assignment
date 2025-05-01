#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global variables for static sizes
#define PHYSICAL_PAGES 32
#define PAGE_TABLE_ENTRIES 128
#define OFFSET_BITS 9
#define REFERENCE_RESET_INTERVAL 200
#define READ_AHEAD 10

// Page Table Entry struct
typedef struct
{
    int frame_num;
    int present;
    int dirty;
    int referenced;
} PTE;

// Frame in physical memory struct
typedef struct
{
    int pid;
    int virtual_pn; // virtual page number
    int load_time;
    int last_acc_time;
    int dirty;
} FRAME;

// Function to extract virtual page number and offset from a virtual address
void PAGE_OFFSET(int virtual_addr, int *page_num, int *offset)
{
    *page_num = virtual_addr >> OFFSET_BITS; // shift right by 9 bits
    *offset = virtual_addr & ((1 << OFFSET_BITS) - 1);
}

// Function to simulate the Random (RAND) page replacement algorithm
void RAND(FILE *fp)
{
    PTE page_tables[10][PAGE_TABLE_ENTRIES] = {0}; // Assuming max 10 processes
    FRAME physical_memory[PHYSICAL_PAGES] = {0};
    int next_aframe = 0, rand_pf = 0, rand_ref = 0, rand_dw = 0, ref_count = 0; // aframe = available frame
    char line[256];

    // Initialize random seed
    srand(1); // seed set to 1 so it can be replicated

    // Set process ID to -1 to mark frames as empty
    for (int i = 0; i < PHYSICAL_PAGES; i++)
    {
        physical_memory[i].pid = -1;
    }

    printf("\nRandom (RAND) Algorithm: \n");

    // Read each line of file fp
    while (fgets(line, sizeof(line), fp))
    {
        int pid, virtual_addr;
        char access_type;
        if (sscanf(line, "%d %d %c", &pid, &virtual_addr, &access_type) != 3)
        {
            continue;
        }
        ref_count++;

        int virtual_pn, offset;
        PAGE_OFFSET(virtual_addr, &virtual_pn, &offset);

        // Check if page is present
        if (!page_tables[pid][virtual_pn].present)
        {
            rand_pf++;  // if page not present, count page fault
            rand_ref++; // count disk reference

            // If there are still empty frames to use
            int use_frame;
            if (next_aframe < PHYSICAL_PAGES)
            {
                use_frame = next_aframe++;
            }
            else
            {
                use_frame = rand() % PHYSICAL_PAGES;
                if (physical_memory[use_frame].pid != -1 &&
                    page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].dirty)
                {
                    rand_ref++;
                    rand_dw++;
                    page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].dirty = 0;
                }
                page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].present = 0;
            }

            // Update page table for the new page
            page_tables[pid][virtual_pn].present = 1;
            page_tables[pid][virtual_pn].frame_num = use_frame;
            page_tables[pid][virtual_pn].dirty = (access_type == 'W');
            page_tables[pid][virtual_pn].referenced = 1;

            // Update physical memory
            physical_memory[use_frame].pid = pid;
            physical_memory[use_frame].virtual_pn = virtual_pn;
            physical_memory[use_frame].dirty = (access_type == 'W');
        }
        else
        { // verify corresponding entry
            page_tables[pid][virtual_pn].referenced = 1;
            if (access_type == 'W')
            {
                page_tables[pid][virtual_pn].dirty = 1;
                int frame_num = page_tables[pid][virtual_pn].frame_num;
                physical_memory[frame_num].dirty = 1;
            }
            int frame_num = page_tables[pid][virtual_pn].frame_num;
        }
    }

    printf("\tTotal Page Faults: %d\n", rand_pf);
    printf("\tTotal Disk References: %d\n", rand_ref);
    printf("\tTotal Dirty Page Writes: %d\n", rand_dw);
    rewind(fp); // Reset file pointer for the next simulation
}

// Funtion to simulate the First In First Out (FIFO) page replacement algorithm
void FIFO(FILE *fp)
{
    PTE page_tables[10][PAGE_TABLE_ENTRIES] = {0};
    FRAME physical_memory[PHYSICAL_PAGES] = {0};
    int next_aframe = 0, FIFO_pf = 0, FIFO_ref = 0, FIFO_dw = 0, ref_count = 0, oldest_frame = 0;
    char line[256];

    printf("\nFIFO Algorithm: \n");

    while (fgets(line, sizeof(line), fp))
    {
        int pid, virtual_addr;
        char access_type;
        if (sscanf(line, "%d %d %c", &pid, &virtual_addr, &access_type) != 3)
        {
            continue;
        }
        ref_count++;

        int virtual_pn, offset;
        PAGE_OFFSET(virtual_addr, &virtual_pn, &offset);

        if (!page_tables[pid][virtual_pn].present)
        {
            FIFO_pf++;  // add page fault
            FIFO_ref++; // add page reference

            int use_frame;
            if (next_aframe < PHYSICAL_PAGES)
            {
                use_frame = next_aframe++;
            }
            else
            {
                use_frame = oldest_frame;
                if (physical_memory[use_frame].pid != -1 &&
                    page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].dirty)
                {
                    FIFO_ref++;
                    FIFO_dw++;
                    page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].dirty = 0;
                }
                page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].present = 0;

                oldest_frame = (oldest_frame + 1) % PHYSICAL_PAGES; // Next oldest frame (+1) since oldest one was just used
            }

            // Update page table for the new page
            page_tables[pid][virtual_pn].present = 1;
            page_tables[pid][virtual_pn].frame_num = use_frame;
            page_tables[pid][virtual_pn].dirty = (access_type == 'W');
            page_tables[pid][virtual_pn].referenced = 1;

            // Update physical memory
            physical_memory[use_frame].pid = pid;
            physical_memory[use_frame].virtual_pn = virtual_pn;
            physical_memory[use_frame].dirty = (access_type == 'W');
        }
        else
        {
            // Page already present
            page_tables[pid][virtual_pn].referenced = 1;
            if (access_type == 'W')
            {
                page_tables[pid][virtual_pn].dirty = 1;
                int frame_num = page_tables[pid][virtual_pn].frame_num;
                physical_memory[frame_num].dirty = 1;
            }
        }
    }

    printf("\tTotal Page Faults: %d\n", FIFO_pf);
    printf("\tTotal Disk References: %d\n", FIFO_ref);
    printf("\tTotal Dirty Page Writes: %d\n", FIFO_dw);
    rewind(fp);
}

// Function to simulate the Least Recently Used (LRU) page replacement algorithm
void LRU(FILE *fp)
{
    PTE page_tables[10][PAGE_TABLE_ENTRIES] = {0};
    FRAME physical_memory[PHYSICAL_PAGES] = {0};
    int next_available_frame = 0, LRU_pf = 0, LRU_references = 0, LRU_dw = 0, reference_count = 0, current_time = 0;
    char line[256];

    for (int i = 0; i < PHYSICAL_PAGES; i++)
    {
        physical_memory[i].pid = -1;
    }

    printf("\nLRU Algorithm:  \n");

    while (fgets(line, sizeof(line), fp))
    {
        int process_id, virtual_address;
        char access_type;

        if (sscanf(line, "%d %d %c", &process_id, &virtual_address, &access_type) != 3)
        {
            continue;
        }
        current_time++;
        reference_count++;

        int virtual_page_number, offset;
        PAGE_OFFSET(virtual_address, &virtual_page_number, &offset);

        if (!page_tables[process_id][virtual_page_number].present)
        {
            LRU_pf++; // fault check
            LRU_references++;

            int frame_to_use;
            if (next_available_frame < PHYSICAL_PAGES)
            {
                frame_to_use = next_available_frame++;
            }
            else
            { // LRU page location
                int lru_frame = -1;
                int min_last_used = current_time + 1;
                for (int i = 0; i < PHYSICAL_PAGES; i++)
                {
                    if (physical_memory[i].pid != -1 && physical_memory[i].last_acc_time < min_last_used)
                    {
                        min_last_used = physical_memory[i].last_acc_time;
                        lru_frame = i;
                    }
                }

                if (lru_frame != -1)
                {
                    frame_to_use = lru_frame;
                    if (physical_memory[frame_to_use].dirty)
                    {
                        LRU_references++;
                        LRU_dw++;
                        page_tables[physical_memory[frame_to_use].pid][physical_memory[frame_to_use].virtual_pn].dirty = 0;
                    }
                    page_tables[physical_memory[frame_to_use].pid][physical_memory[frame_to_use].virtual_pn].present = 0;
                }
                else
                {
                    fprintf(stderr, "Error: No LRU frame found for replacement\n");
                    exit(1);
                }
            }

            page_tables[process_id][virtual_page_number].present = 1;
            page_tables[process_id][virtual_page_number].frame_num = frame_to_use;
            page_tables[process_id][virtual_page_number].dirty = (access_type == 'W');
            page_tables[process_id][virtual_page_number].referenced = 1;

            physical_memory[frame_to_use].pid = process_id;
            physical_memory[frame_to_use].virtual_pn = virtual_page_number;
            physical_memory[frame_to_use].last_acc_time = current_time;
            physical_memory[frame_to_use].dirty = (access_type == 'W');
        }
        else
        {
            page_tables[process_id][virtual_page_number].referenced = 1;
            if (access_type == 'W')
            {
                page_tables[process_id][virtual_page_number].dirty = 1;
                int frame_num = page_tables[process_id][virtual_page_number].frame_num;
                physical_memory[frame_num].dirty = 1;
            }

            int frame_num = page_tables[process_id][virtual_page_number].frame_num;
            physical_memory[frame_num].last_acc_time = current_time;
        }
    }

    printf("\tTotal Page Faults: %d\n", LRU_pf);
    printf("\tTotal Disk References: %d\n", LRU_references);
    printf("\tTotal Dirty Page Writes: %d\n", LRU_dw);
    rewind(fp); // Reset file pointer for the next simulation
}

// Function to simulate the Periodic Reference Reset (PER) page replacement algorithm
void PER(FILE *fp)
{
    PTE page_tables[10][PAGE_TABLE_ENTRIES] = {0};
    FRAME physical_memory[PHYSICAL_PAGES] = {0};
    int next_aframe = 0, PER_pf = 0, PER_ref = 0, PER_dw = 0, ref_count = 0, curr_time = 0;
    char line[256];

    for (int i = 0; i < PHYSICAL_PAGES; i++)
    {
        physical_memory[i].pid = -1;
    }

    printf("\nPeriodic Reference Reset (PER) Algorithm: \n");

    while (fgets(line, sizeof(line), fp))
    {
        int pid, virtual_addr;
        char access_type;
        if (sscanf(line, "%d %d %c", &pid, &virtual_addr, &access_type) != 3)
        {
            continue;
        }
        curr_time++;
        ref_count++;

        int virtual_pn, offset;
        PAGE_OFFSET(virtual_addr, &virtual_pn, &offset);

        if (!page_tables[pid][virtual_pn].present)
        {
            PER_pf++; // Page Fault
            PER_ref++;

            int use_frame;
            if (next_aframe < PHYSICAL_PAGES)
            {
                use_frame = next_aframe++;
            }
            else
            {
                int check_frame = -1; // locate page

                for (int i = 0; i < PHYSICAL_PAGES; i++)
                { // identify unallocated pages
                    if (physical_memory[i].pid == -1)
                    {
                        check_frame = i;
                        break;
                    }
                }

                if (check_frame == -1)
                {
                    for (int i = 0; i < PHYSICAL_PAGES; i++)
                    { // check for unreference and clean
                        if (physical_memory[i].pid != -1 &&
                            !page_tables[physical_memory[i].pid][physical_memory[i].virtual_pn].referenced &&
                            !page_tables[physical_memory[i].pid][physical_memory[i].virtual_pn].dirty)
                        {
                            check_frame = i;
                            break;
                        }
                    }
                }

                if (check_frame == -1)
                {
                    for (int i = 0; i < PHYSICAL_PAGES; i++)
                    { // check for unferenced and dirty
                        if (physical_memory[i].pid != -1 &&
                            !page_tables[physical_memory[i].pid][physical_memory[i].virtual_pn].referenced &&
                            page_tables[physical_memory[i].pid][physical_memory[i].virtual_pn].dirty)
                        {
                            check_frame = i;
                            break;
                        }
                    }
                }

                if (check_frame == -1)
                {
                    for (int i = 0; i < PHYSICAL_PAGES; i++)
                    { // check for referenced and clean
                        if (physical_memory[i].pid != -1 &&
                            page_tables[physical_memory[i].pid][physical_memory[i].virtual_pn].referenced &&
                            !page_tables[physical_memory[i].pid][physical_memory[i].virtual_pn].dirty)
                        {
                            check_frame = i;
                            break;
                        }
                    }
                }

                if (check_frame == -1)
                {
                    for (int i = 0; i < PHYSICAL_PAGES; i++)
                    { // check for referenced and dirty
                        if (physical_memory[i].pid != -1 &&
                            page_tables[physical_memory[i].pid][physical_memory[i].virtual_pn].referenced &&
                            page_tables[physical_memory[i].pid][physical_memory[i].virtual_pn].dirty)
                        {
                            check_frame = i;
                            break;
                        }
                    }
                }

                if (check_frame != -1)
                {
                    use_frame = check_frame;
                    if (physical_memory[use_frame].dirty)
                    {
                        PER_ref++;
                        PER_dw++;
                        page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].dirty = 0;
                    }
                    page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].present = 0;
                }
                else
                {
                    fprintf(stderr, "Error: No victim frame found for PER replacement\n");
                    exit(1);
                }
            }

            page_tables[pid][virtual_pn].present = 1;
            page_tables[pid][virtual_pn].frame_num = use_frame;
            page_tables[pid][virtual_pn].dirty = (access_type == 'W');
            page_tables[pid][virtual_pn].referenced = 1;

            physical_memory[use_frame].pid = pid;
            physical_memory[use_frame].virtual_pn = virtual_pn;
            physical_memory[use_frame].last_acc_time = curr_time;
            physical_memory[use_frame].dirty = (access_type == 'W');
        }
        else
        {
            page_tables[pid][virtual_pn].referenced = 1;
            if (access_type == 'W')
            {
                page_tables[pid][virtual_pn].dirty = 1;
                int frame_num = page_tables[pid][virtual_pn].frame_num;
                physical_memory[frame_num].dirty = 1;
            }
            int frame_num = page_tables[pid][virtual_pn].frame_num;
            physical_memory[frame_num].last_acc_time = curr_time;
        }

        if (ref_count == REFERENCE_RESET_INTERVAL)
        {
            for (int i = 0; i < 10; i++)
            {
                for (int j = 0; j < PAGE_TABLE_ENTRIES; j++)
                {
                    page_tables[i][j].referenced = 0;
                }
            }
            ref_count = 0;
        }
    }

    printf("\tTotal Page Faults: %d\n", PER_pf);
    printf("\tTotal Disk References: %d\n", PER_ref);
    printf("\tTotal Dirty Page Writes: %d\n", PER_dw);
    rewind(fp);
}

void CUSTOM_ALG(FILE *fp)
{
    fflush(stdout);
    PTE page_tables[10][PAGE_TABLE_ENTRIES] = {0};
    FRAME physical_memory[PHYSICAL_PAGES] = {0};
    // printf("allocating array\n");
    char (*buffer)[256] = malloc(sizeof(char[50000][256]));
    int frame_to_use = 0, next_aframe = 0, CUST_pf = 0, CUST_ref = 0, CUST_dw = 0, ref_count = 0, curr_time = 0;
    char line[256];
    for (int i = 0; i < PHYSICAL_PAGES; i++)
    {
        physical_memory[i].pid = -1;
    }

    printf("\nCustom Page Replacement Algorithm: \n");

    // printf("Populating buffer\n");
    for (int i = 0; i < 50000; i++)
    {
        fgets(buffer[i], sizeof(buffer[i]), fp);
    }

    rewind(fp);

    int mem_access_count = 0;
    while (fgets(line, sizeof(line), fp)) // reset to 50000
    {
        int process_id, virtual_address;
        char access_type;

        // Grab the next memory op
        if (sscanf(line, "%d %d %c", &process_id, &virtual_address, &access_type) != 3)
        {
            continue;
        }
        curr_time++;
        ref_count++;

        // Grab VPN & OFFSET from VA
        int virtual_page_number, offset;
        PAGE_OFFSET(virtual_address, &virtual_page_number, &offset);

        if (!page_tables[process_id][virtual_page_number].present)
        {
            CUST_pf++;
            CUST_ref++;

            if (next_aframe < PHYSICAL_PAGES)
            {
                frame_to_use = next_aframe++;
                // printf("Adding Page\n");
                // fflush(stdout);
            }
            else // PHYSICAL MEM FULL
            {
                // Paradigm: Replaces furthest VPN from current VPN.
                // REPLACE ALGO
                int tracking_buffer[PHYSICAL_PAGES][2] = {-1};
                int use_frame = -1;
                int use_count_max = 0;
                int temp_addr;
                for (int j = 0; j < PHYSICAL_PAGES; j++)
                {
                    tracking_buffer[j][0] = physical_memory[j].virtual_pn;
                    int found = 0;
                    int k = 0;
                    while (found == 0)
                    {
                        int temp_process_id;
                        int temp_virtual_address;
                        char temp_access_type;
                        sscanf(buffer[k], "%d %d %c", &temp_process_id, &temp_virtual_address, &temp_access_type);

                        int temp_vpn;
                        int temp_offset;
                        PAGE_OFFSET(temp_virtual_address, &temp_vpn, &temp_offset);
                        // printf("physical vpn: %d, table entry vpn: %d\n", tracking_buffer[j][0], temp_vpn);
                        if (tracking_buffer[j][0] == temp_vpn)
                        {
                            found = 1;
                        }
                        else
                        {
                            k++;
                        }
                    }
                    tracking_buffer[j][1] = k;
                    // printf("%d-----------------------------------\n", tracking_buffer[j][1]);
                }

                int furthest = 0;
                for (int i = 0; i < PHYSICAL_PAGES; i++)
                {
                    if (tracking_buffer[i][1] > furthest)
                    {
                        furthest = tracking_buffer[i][1];
                        use_frame = i;
                    }
                }

                if (use_frame != -1) // change this
                {
                    if (physical_memory[use_frame].dirty)
                    {
                        CUST_ref++;
                        CUST_dw++;
                        page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].dirty = 0;
                    }
                    page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].present = 0;
                }
                else
                {
                    perror("No page");
                    // use_frame = 1;
                }
            }

            page_tables[process_id][virtual_page_number].present = 1;
            page_tables[process_id][virtual_page_number].frame_num = frame_to_use;
            page_tables[process_id][virtual_page_number].dirty = (access_type == 'W');
            page_tables[process_id][virtual_page_number].referenced = 1;

            physical_memory[frame_to_use].pid = process_id;
            physical_memory[frame_to_use].virtual_pn = virtual_page_number;
            physical_memory[frame_to_use].last_acc_time = curr_time;
            physical_memory[frame_to_use].dirty = (access_type == 'W');
        }
        else
        {
            page_tables[process_id][virtual_page_number].referenced = 1;
            if (access_type == 'W')
            {
                page_tables[process_id][virtual_page_number].dirty = 1;
                int frame_num = page_tables[process_id][virtual_page_number].frame_num;
                physical_memory[frame_num].dirty = 1;
            }
            int frame_num = page_tables[process_id][virtual_page_number].frame_num;
            physical_memory[frame_num].last_acc_time = curr_time;
        }
        mem_access_count++;
    }

    // while (fgets(line, sizeof(line), fp))
    // {
    //     int process_id, virtual_address;
    //     char access_type;

    //     // Grab the next memory op
    //     if (sscanf(line, "%d %d %c", &process_id, &virtual_address, &access_type) != 3)
    //     {
    //         continue;
    //     }
    //     curr_time++;
    //     ref_count++;

    //     // Grab VPN & OFFSET from VA
    //     int virtual_page_number, offset;
    //     PAGE_OFFSET(virtual_address, &virtual_page_number, &offset);

    //     if (!page_tables[process_id][virtual_page_number].present)
    //     {
    //         CUST_pf++;
    //         CUST_ref++;

    //         if (next_aframe < PHYSICAL_PAGES)
    //         {
    //             frame_to_use = next_aframe++;
    //         }
    //         else // PHYSICAL MEM FULL
    //         {
    //             // Paradigm: Replaces furthest VPN from current VPN.
    //             // REPLACE ALGO
    //             int furthest_distance = 0;
    //             int furthest_vpn = -1;
    //             int use_frame = -1;
    //             for (int i = 1; i < PHYSICAL_PAGES; i++)
    //             {
    //                 int current_distance = abs(virtual_page_number - physical_memory[i].virtual_pn);
    //                 if (furthest_distance < current_distance)
    //                 {
    //                     furthest_vpn = physical_memory[i].virtual_pn;
    //                     use_frame = i;
    //                 }
    //             }

    //             if (furthest_vpn != -1)
    //             {
    //                 if (physical_memory[use_frame].dirty)
    //                 {
    //                     CUST_ref++;
    //                     CUST_dw++;
    //                     page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].dirty = 0;
    //                 }
    //                 page_tables[physical_memory[use_frame].pid][physical_memory[use_frame].virtual_pn].present = 0;
    //             }
    //             else
    //             {
    //                 perror("Unable to find frame to swap...");
    //             }
    //         }

    //         page_tables[process_id][virtual_page_number].present = 1;
    //         page_tables[process_id][virtual_page_number].frame_num = frame_to_use;
    //         page_tables[process_id][virtual_page_number].dirty = (access_type == 'W');
    //         page_tables[process_id][virtual_page_number].referenced = 1;

    //         physical_memory[frame_to_use].pid = process_id;
    //         physical_memory[frame_to_use].virtual_pn = virtual_page_number;
    //         physical_memory[frame_to_use].last_acc_time = curr_time;
    //         physical_memory[frame_to_use].dirty = (access_type == 'W');
    //     }
    //     else
    //     {
    //         page_tables[process_id][virtual_page_number].referenced = 1;
    //         if (access_type == 'W')
    //         {
    //             page_tables[process_id][virtual_page_number].dirty = 1;
    //             int frame_num = page_tables[process_id][virtual_page_number].frame_num;
    //             physical_memory[frame_num].dirty = 1;
    //         }
    //         int frame_num = page_tables[process_id][virtual_page_number].frame_num;
    //         physical_memory[frame_num].last_acc_time = curr_time;
    //     }
    // }
    free(buffer);
    printf("\tTotal Page Faults: %d\n", CUST_pf);
    printf("\tTotal Disk References: %d\n", CUST_ref);
    printf("\tTotal Dirty Page Writes: %d\n", CUST_dw);
    rewind(fp);
}

int main()
{
    char filename[20];
    int data;
    FILE *fp;

    printf("Select a data file to open:\n");
    printf("1. data1.txt\n");
    printf("2. data2.txt\n");
    printf("3. data3.txt\n");
    printf("Enter desired dataset (1, 2, or 3): ");
    // printf("Enter desired dataset (1 or 2): ");
    scanf("%d", &data);

    if (data == 1)
    {
        strcpy(filename, "data1.txt");
    }
    else if (data == 2)
    {
        strcpy(filename, "data2.txt");
    }
    else if (data == 3)
    { // Our test file to verify
        strcpy(filename, "data3.txt");
    }
    else
    {
        printf("Invalid Selection!!!\n");
        return 1;
    }

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Error Opening File!!!\n");
        return 1;
    }

    // Prompt functions to simulation for algorithms
    RAND(fp);
    FIFO(fp);
    LRU(fp);
    PER(fp);
    CUSTOM_ALG(fp);

    fclose(fp);
    return 0;
}
