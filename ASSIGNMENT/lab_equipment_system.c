/* ============================================================================
   SMART UNIVERSITY LABORATORY EQUIPMENT MONITORING AND ALLOCATION SYSTEM
   ----------------------------------------------------------------------------
   Language : C (C99)
   Concepts demonstrated:
     - Operators, decision-making (if/else, switch), looping (for/while/do-while)
     - Arrays and string processing
     - Searching   : Linear search, Recursive Binary search
     - Sorting     : Bubble sort (multi-key), comparator based selection
     - Merging     : Duplicate-safe merge of two laboratories' equipment lists
     - Functions   : Modular design - one function per operation
     - Recursion   : Recursive binary search, recursive "most used" finder,
                     recursive merge-duplicate-check, recursive report totals
     - Pointers    : Arrays passed/traversed via pointers, pointer to struct
     - File Handling: Persistent storage (equipment_data.txt), reports file
   ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>   /* for strcasecmp */

#define MAX_EQUIPMENT   200
#define DATA_FILE       "equipment_data.txt"
#define REPORT_FILE     "lab_report.txt"
#define LOW_AVAIL_PCT   30      /* below this % of quantity => Low Availability */
#define MAX_USAGE_LIMIT 500     /* usage count beyond which item flagged for replacement */

/* --------------------------------------------------------------------------
   STRUCTURE DEFINITION
   -------------------------------------------------------------------------- */
typedef struct {
    int  id;
    char name[50];
    char category[30];
    char labName[30];
    char condition[20];        /* Good, Damaged, Under Repair */
    char maintenanceDate[15];  /* DD-MM-YYYY */
    int  quantity;
    int  availableUnits;
    int  usageCount;
    char status[20];           /* computed field */
} Equipment;

/* --------------------------------------------------------------------------
   GLOBAL DATA STORE
   -------------------------------------------------------------------------- */
Equipment lab[MAX_EQUIPMENT];
int equipCount = 0;

/* --------------------------------------------------------------------------
   FUNCTION PROTOTYPES  (modular design)
   -------------------------------------------------------------------------- */
void  showMenu(void);
void  addEquipment(Equipment *arr, int *count);
void  updateEquipment(Equipment *arr, int count);
void  displayAll(const Equipment *arr, int count);
void  displayOne(const Equipment *e);

int   linearSearchByID(const Equipment *arr, int count, int id);
int   recursiveBinarySearchByID(const Equipment *arr, int low, int high, int id);
void  searchMenu(Equipment *arr, int count);
void  searchByCategory(const Equipment *arr, int count, const char *cat);
void  searchByLab(const Equipment *arr, int count, const char *labName);
void  searchByCondition(const Equipment *arr, int count, const char *cond);
void  searchByAvailability(const Equipment *arr, int count);

void  sortMenu(Equipment *arr, int count);
void  bubbleSortByUsage(Equipment *arr, int count, int descending);
void  bubbleSortByID(Equipment *arr, int count);
void  bubbleSortByCategory(Equipment *arr, int count);
void  bubbleSortByAvailability(Equipment *arr, int count, int descending);

int   isDuplicateID(const Equipment *arr, int count, int id);
int   mergeLabRecords(Equipment *dest, int destCount,
                       const Equipment *src, int srcCount, int *skipped);

void  analyzeStatus(Equipment *arr, int count);
const char* classify(const Equipment *e);

void  allocateEquipment(Equipment *arr, int count);

int   findMostUsedRecursive(const Equipment *arr, int index, int count, int bestIndex);
void  generateReport(Equipment *arr, int count);

void  saveToFile(const Equipment *arr, int count);
int   loadFromFile(Equipment *arr);

void  toLowerStr(char *dest, const char *src);
void  flushInput(void);
int   readIntSafe(const char *prompt);
void  readLine(const char *prompt, char *buffer, int size);

/* ============================================================================
   MAIN
   ============================================================================ */
int main(void) {
    equipCount = loadFromFile(lab);
    printf("=====================================================================\n");
    printf(" SMART UNIVERSITY LABORATORY EQUIPMENT MONITORING & ALLOCATION SYSTEM\n");
    printf("=====================================================================\n");
    printf("Loaded %d equipment record(s) from '%s'.\n", equipCount, DATA_FILE);

    int choice;
    do {
        showMenu();
        choice = readIntSafe("Enter your choice: ");
        switch (choice) {
            case 1:  addEquipment(lab, &equipCount); break;
            case 2:  updateEquipment(lab, equipCount); break;
            case 3:  displayAll(lab, equipCount); break;
            case 4:  searchMenu(lab, equipCount); break;
            case 5:  sortMenu(lab, equipCount); break;
            case 6: {
                /* Merge demo: merge records coming from another lab file */
                char fname[100];
                readLine("Enter filename of the other lab's data (e.g. lab2_data.txt): ", fname, sizeof(fname));
                Equipment incoming[MAX_EQUIPMENT];
                FILE *fp = fopen(fname, "r");
                int inCount = 0;
                if (fp == NULL) {
                    printf("Could not open '%s'. Merge aborted.\n", fname);
                } else {
                    fclose(fp);
                    /* temporarily point loader at the given file */
                    FILE *f2 = fopen(fname, "r");
                    while (f2 != NULL && !feof(f2)) {
                        Equipment tmp;
                        int r = fscanf(f2, "%d|%49[^|]|%29[^|]|%29[^|]|%19[^|]|%14[^|]|%d|%d|%d\n",
                                       &tmp.id, tmp.name, tmp.category, tmp.labName,
                                       tmp.condition, tmp.maintenanceDate,
                                       &tmp.quantity, &tmp.availableUnits, &tmp.usageCount);
                        if (r == 9) incoming[inCount++] = tmp;
                    }
                    if (f2) fclose(f2);
                    int skipped = 0;
                    equipCount = mergeLabRecords(lab, equipCount, incoming, inCount, &skipped);
                    printf("Merge complete. %d new record(s) added, %d duplicate ID(s) skipped.\n",
                           inCount - skipped, skipped);
                }
                break;
            }
            case 7:  analyzeStatus(lab, equipCount); break;
            case 8:  allocateEquipment(lab, equipCount); break;
            case 9:  generateReport(lab, equipCount); break;
            case 10: saveToFile(lab, equipCount); printf("Data saved to '%s'.\n", DATA_FILE); break;
            case 0:  saveToFile(lab, equipCount); printf("Data auto-saved. Exiting. Goodbye!\n"); break;
            default: printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);

    return 0;
}

/* ============================================================================
   MENU
   ============================================================================ */
void showMenu(void) {
    printf("\n---------------------------------------------------------\n");
    printf(" 1. Add Equipment\n");
    printf(" 2. Update Equipment\n");
    printf(" 3. Display All Equipment\n");
    printf(" 4. Search Equipment (ID / Category / Lab / Condition / Availability)\n");
    printf(" 5. Sort Equipment (ID / Category / Usage / Availability)\n");
    printf(" 6. Merge Records from Another Laboratory File\n");
    printf(" 7. Analyse Equipment Status (Available/Low/Maintenance/Critical)\n");
    printf(" 8. Allocate Equipment to a Requesting Lab/User\n");
    printf(" 9. Generate Consolidated Report\n");
    printf("10. Save Records to File\n");
    printf(" 0. Exit\n");
    printf("---------------------------------------------------------\n");
}

/* ============================================================================
   UTILITY / INPUT-SAFE FUNCTIONS
   ============================================================================ */
void flushInput(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

int readIntSafe(const char *prompt) {
    int value;
    printf("%s", prompt);
    while (scanf("%d", &value) != 1) {
        printf("Invalid input. Please enter a number: ");
        flushInput();
    }
    flushInput();
    return value;
}

void readLine(const char *prompt, char *buffer, int size) {
    printf("%s", prompt);
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';   /* strip trailing newline */
}

void toLowerStr(char *dest, const char *src) {
    int i = 0;
    while (src[i]) {
        dest[i] = (char)tolower((unsigned char)src[i]);
        i++;
    }
    dest[i] = '\0';
}

/* ============================================================================
   1. ADD EQUIPMENT
   ============================================================================ */
void addEquipment(Equipment *arr, int *count) {
    if (*count >= MAX_EQUIPMENT) {
        printf("Equipment storage full! Cannot add more records.\n");
        return;
    }
    Equipment *e = &arr[*count];   /* pointer usage */

    e->id = readIntSafe("Enter Equipment ID: ");
    if (isDuplicateID(arr, *count, e->id)) {
        printf("Error: Equipment ID %d already exists. Use Update instead.\n", e->id);
        return;
    }
    readLine("Enter Equipment Name: ", e->name, sizeof(e->name));
    readLine("Enter Category (e.g. Computer/Sensor/Kit/Instrument): ", e->category, sizeof(e->category));
    readLine("Enter Laboratory Name: ", e->labName, sizeof(e->labName));
    readLine("Enter Condition (Good/Damaged/Under Repair): ", e->condition, sizeof(e->condition));
    readLine("Enter Last Maintenance Date (DD-MM-YYYY): ", e->maintenanceDate, sizeof(e->maintenanceDate));
    e->quantity = readIntSafe("Enter Total Quantity: ");

    do {
        e->availableUnits = readIntSafe("Enter Available Units: ");
        if (e->availableUnits > e->quantity)
            printf("Available units cannot exceed total quantity (%d). Try again.\n", e->quantity);
    } while (e->availableUnits > e->quantity || e->availableUnits < 0);

    e->usageCount = readIntSafe("Enter Current Usage Count: ");
    strcpy(e->status, classify(e));

    (*count)++;
    printf("Equipment '%s' added successfully with status [%s].\n", e->name, e->status);
}

/* ============================================================================
   2. UPDATE EQUIPMENT
   ============================================================================ */
void updateEquipment(Equipment *arr, int count) {
    int id = readIntSafe("Enter Equipment ID to update: ");
    int idx = linearSearchByID(arr, count, id);
    if (idx == -1) {
        printf("Equipment ID %d not found.\n", id);
        return;
    }
    Equipment *e = &arr[idx];
    printf("Current details:\n");
    displayOne(e);

    int sub;
    printf("What do you want to update?\n");
    printf(" 1. Condition   2. Maintenance Date   3. Available Units\n");
    printf(" 4. Usage Count 5. Quantity           6. All fields\n");
    sub = readIntSafe("Choice: ");

    switch (sub) {
        case 1: readLine("New Condition: ", e->condition, sizeof(e->condition)); break;
        case 2: readLine("New Maintenance Date: ", e->maintenanceDate, sizeof(e->maintenanceDate)); break;
        case 3:
            e->availableUnits = readIntSafe("New Available Units: ");
            if (e->availableUnits > e->quantity) e->availableUnits = e->quantity;
            break;
        case 4: e->usageCount = readIntSafe("New Usage Count: "); break;
        case 5: e->quantity = readIntSafe("New Quantity: "); break;
        case 6:
            readLine("New Condition: ", e->condition, sizeof(e->condition));
            readLine("New Maintenance Date: ", e->maintenanceDate, sizeof(e->maintenanceDate));
            e->quantity = readIntSafe("New Quantity: ");
            e->availableUnits = readIntSafe("New Available Units: ");
            e->usageCount = readIntSafe("New Usage Count: ");
            break;
        default: printf("Invalid option.\n"); return;
    }
    strcpy(e->status, classify(e));
    printf("Equipment ID %d updated. New status: [%s]\n", id, e->status);
}

/* ============================================================================
   DISPLAY FUNCTIONS
   ============================================================================ */
void displayOne(const Equipment *e) {
    printf("-----------------------------------------------------------\n");
    printf("ID: %-5d Name: %-15s Category: %-12s\n", e->id, e->name, e->category);
    printf("Lab: %-12s Condition: %-12s Maint.Date: %-10s\n", e->labName, e->condition, e->maintenanceDate);
    printf("Qty: %-4d Available: %-4d Usage: %-5d Status: %s\n",
           e->quantity, e->availableUnits, e->usageCount, e->status);
}

void displayAll(const Equipment *arr, int count) {
    if (count == 0) { printf("No equipment records found.\n"); return; }
    printf("\n================= ALL EQUIPMENT RECORDS (%d) =================\n", count);
    for (int i = 0; i < count; i++) {
        displayOne(&arr[i]);
    }
}

/* ============================================================================
   SEARCHING
   ============================================================================ */
int linearSearchByID(const Equipment *arr, int count, int id) {
    for (int i = 0; i < count; i++) {
        if (arr[i].id == id) return i;
    }
    return -1;
}

/* Recursive binary search - requires array sorted by ID beforehand */
int recursiveBinarySearchByID(const Equipment *arr, int low, int high, int id) {
    if (low > high) return -1;
    int mid = (low + high) / 2;
    if (arr[mid].id == id) return mid;
    else if (arr[mid].id > id) return recursiveBinarySearchByID(arr, low, mid - 1, id);
    else return recursiveBinarySearchByID(arr, mid + 1, high, id);
}

void searchMenu(Equipment *arr, int count) {
    if (count == 0) { printf("No records to search.\n"); return; }
    printf("Search by: 1.ID(Recursive Binary) 2.Category 3.Lab 4.Condition 5.Availability\n");
    int c = readIntSafe("Choice: ");
    char buf[30];
    switch (c) {
        case 1: {
            int id = readIntSafe("Enter Equipment ID: ");
            /* sort a copy by ID for binary search, keep master array untouched */
            Equipment temp[MAX_EQUIPMENT];
            memcpy(temp, arr, sizeof(Equipment) * count);
            bubbleSortByID(temp, count);
            int idx = recursiveBinarySearchByID(temp, 0, count - 1, id);
            if (idx == -1) printf("Equipment ID %d not found.\n", id);
            else { printf("Record found (recursive binary search):\n"); displayOne(&temp[idx]); }
            break;
        }
        case 2: readLine("Enter Category: ", buf, sizeof(buf)); searchByCategory(arr, count, buf); break;
        case 3: readLine("Enter Lab Name: ", buf, sizeof(buf)); searchByLab(arr, count, buf); break;
        case 4: readLine("Enter Condition: ", buf, sizeof(buf)); searchByCondition(arr, count, buf); break;
        case 5: searchByAvailability(arr, count); break;
        default: printf("Invalid choice.\n");
    }
}

void searchByCategory(const Equipment *arr, int count, const char *cat) {
    char target[30], cur[30];
    toLowerStr(target, cat);
    int found = 0;
    for (int i = 0; i < count; i++) {
        toLowerStr(cur, arr[i].category);
        if (strcmp(cur, target) == 0) { displayOne(&arr[i]); found++; }
    }
    if (!found) printf("No equipment found in category '%s'.\n", cat);
    else printf("%d record(s) found.\n", found);
}

void searchByLab(const Equipment *arr, int count, const char *labName) {
    char target[30], cur[30];
    toLowerStr(target, labName);
    int found = 0;
    for (int i = 0; i < count; i++) {
        toLowerStr(cur, arr[i].labName);
        if (strcmp(cur, target) == 0) { displayOne(&arr[i]); found++; }
    }
    if (!found) printf("No equipment found in lab '%s'.\n", labName);
    else printf("%d record(s) found.\n", found);
}

void searchByCondition(const Equipment *arr, int count, const char *cond) {
    char target[20], cur[20];
    toLowerStr(target, cond);
    int found = 0;
    for (int i = 0; i < count; i++) {
        toLowerStr(cur, arr[i].condition);
        if (strcmp(cur, target) == 0) { displayOne(&arr[i]); found++; }
    }
    if (!found) printf("No equipment found with condition '%s'.\n", cond);
    else printf("%d record(s) found.\n", found);
}

void searchByAvailability(const Equipment *arr, int count) {
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (arr[i].availableUnits > 0) { displayOne(&arr[i]); found++; }
    }
    if (!found) printf("No available equipment at the moment.\n");
    else printf("%d record(s) currently available.\n", found);
}

/* ============================================================================
   SORTING  (Bubble sort variants demonstrating multi-key sorting)
   ============================================================================ */
void sortMenu(Equipment *arr, int count) {
    if (count == 0) { printf("No records to sort.\n"); return; }
    printf("Sort by: 1.ID 2.Category(A-Z) 3.Usage(High-Low) 4.Availability(High-Low)\n");
    int c = readIntSafe("Choice: ");
    switch (c) {
        case 1: bubbleSortByID(arr, count); printf("Sorted by ID.\n"); break;
        case 2: bubbleSortByCategory(arr, count); printf("Sorted by Category.\n"); break;
        case 3: bubbleSortByUsage(arr, count, 1); printf("Sorted by Usage (descending).\n"); break;
        case 4: bubbleSortByAvailability(arr, count, 1); printf("Sorted by Availability (descending).\n"); break;
        default: printf("Invalid choice.\n"); return;
    }
    displayAll(arr, count);
}

void bubbleSortByID(Equipment *arr, int count) {
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - 1 - i; j++)
            if (arr[j].id > arr[j + 1].id) {
                Equipment t = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = t;
            }
}

void bubbleSortByCategory(Equipment *arr, int count) {
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - 1 - i; j++)
            if (strcasecmp(arr[j].category, arr[j + 1].category) > 0) {
                Equipment t = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = t;
            }
}

void bubbleSortByUsage(Equipment *arr, int count, int descending) {
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - 1 - i; j++) {
            int cond = descending ? (arr[j].usageCount < arr[j + 1].usageCount)
                                  : (arr[j].usageCount > arr[j + 1].usageCount);
            if (cond) {
                Equipment t = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = t;
            }
        }
}

void bubbleSortByAvailability(Equipment *arr, int count, int descending) {
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - 1 - i; j++) {
            int cond = descending ? (arr[j].availableUnits < arr[j + 1].availableUnits)
                                  : (arr[j].availableUnits > arr[j + 1].availableUnits);
            if (cond) {
                Equipment t = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = t;
            }
        }
}

/* ============================================================================
   MERGING - combine records from another lab, skipping duplicate IDs
   ============================================================================ */
int isDuplicateID(const Equipment *arr, int count, int id) {
    /* recursive duplicate check */
    if (count <= 0) return 0;
    if (arr[count - 1].id == id) return 1;
    return isDuplicateID(arr, count - 1, id);
}

int mergeLabRecords(Equipment *dest, int destCount,
                     const Equipment *src, int srcCount, int *skipped) {
    *skipped = 0;
    for (int i = 0; i < srcCount; i++) {
        if (isDuplicateID(dest, destCount, src[i].id)) {
            (*skipped)++;
            continue;
        }
        if (destCount >= MAX_EQUIPMENT) break;
        dest[destCount] = src[i];
        strcpy(dest[destCount].status, classify(&dest[destCount]));
        destCount++;
    }
    return destCount;
}

/* ============================================================================
   STATUS CLASSIFICATION / ANALYSIS
   ============================================================================ */
const char* classify(const Equipment *e) {
    char condLower[20];
    toLowerStr(condLower, e->condition);

    if (strcmp(condLower, "under repair") == 0) return "Under Maintenance";
    if (e->availableUnits == 0) return "Critical";
    if (e->quantity > 0 && ((e->availableUnits * 100) / e->quantity) < LOW_AVAIL_PCT) return "Low Availability";
    return "Available";
}

void analyzeStatus(Equipment *arr, int count) {
    if (count == 0) { printf("No records to analyse.\n"); return; }
    int available = 0, low = 0, maint = 0, critical = 0;
    printf("\n================= EQUIPMENT STATUS ANALYSIS =================\n");
    for (int i = 0; i < count; i++) {
        strcpy(arr[i].status, classify(&arr[i]));
        printf("[%-18s] ID:%-4d %-15s (Lab: %-10s | Avail: %d/%d)\n",
               arr[i].status, arr[i].id, arr[i].name, arr[i].labName,
               arr[i].availableUnits, arr[i].quantity);

        if (strcmp(arr[i].status, "Available") == 0) available++;
        else if (strcmp(arr[i].status, "Low Availability") == 0) low++;
        else if (strcmp(arr[i].status, "Under Maintenance") == 0) maint++;
        else critical++;
    }
    printf("---------------------------------------------------------------\n");
    printf("Summary => Available: %d | Low Availability: %d | Under Maintenance: %d | Critical: %d\n",
           available, low, maint, critical);
}

/* ============================================================================
   ALLOCATION
   ============================================================================ */
void allocateEquipment(Equipment *arr, int count) {
    if (count == 0) { printf("No records available for allocation.\n"); return; }
    int id = readIntSafe("Enter Equipment ID to allocate: ");
    int idx = linearSearchByID(arr, count, id);
    if (idx == -1) { printf("Equipment ID %d not found.\n", id); return; }

    Equipment *e = &arr[idx];
    strcpy(e->status, classify(e));

    if (strcmp(e->status, "Under Maintenance") == 0) {
        printf("Cannot allocate. Equipment '%s' is currently under maintenance.\n", e->name);
        return;
    }
    if (e->availableUnits <= 0) {
        printf("Cannot allocate. '%s' has 0 available units (Critical).\n", e->name);
        return;
    }

    int reqUnits = readIntSafe("Enter number of units requested: ");
    if (reqUnits <= 0) { printf("Invalid request quantity.\n"); return; }

    if (reqUnits > e->availableUnits) {
        printf("Request denied. Only %d unit(s) available (requested %d).\n",
               e->availableUnits, reqUnits);
        return;
    }
    if (e->usageCount + reqUnits > MAX_USAGE_LIMIT) {
        printf("Warning: Allocation will push usage count beyond safe limit (%d).\n", MAX_USAGE_LIMIT);
        printf("Allocation proceeds, but equipment should be scheduled for replacement soon.\n");
    }

    e->availableUnits -= reqUnits;
    e->usageCount += reqUnits;
    strcpy(e->status, classify(e));

    printf("Allocated %d unit(s) of '%s'. Remaining available: %d. New status: [%s]\n",
           reqUnits, e->name, e->availableUnits, e->status);
}

/* ============================================================================
   REPORTING  (uses recursion to find most-used equipment)
   ============================================================================ */
int findMostUsedRecursive(const Equipment *arr, int index, int count, int bestIndex) {
    if (index == count) return bestIndex;                /* base case */
    if (bestIndex == -1 || arr[index].usageCount > arr[bestIndex].usageCount)
        bestIndex = index;
    return findMostUsedRecursive(arr, index + 1, count, bestIndex);  /* recursive case */
}

void generateReport(Equipment *arr, int count) {
    if (count == 0) { printf("No records available to generate a report.\n"); return; }

    FILE *fp = fopen(REPORT_FILE, "w");
    if (fp == NULL) { printf("Error: could not create report file.\n"); return; }

    fprintf(fp, "======================================================\n");
    fprintf(fp, " CONSOLIDATED LABORATORY EQUIPMENT REPORT\n");
    fprintf(fp, "======================================================\n\n");

    /* Most frequently used equipment - recursive */
    int mostUsedIdx = findMostUsedRecursive(arr, 0, count, -1);
    fprintf(fp, "1) MOST FREQUENTLY USED EQUIPMENT:\n");
    fprintf(fp, "   %s (ID:%d) - Used %d times, Lab: %s\n\n",
            arr[mostUsedIdx].name, arr[mostUsedIdx].id,
            arr[mostUsedIdx].usageCount, arr[mostUsedIdx].labName);

    /* Labs with shortages (any equipment with Low Availability or Critical) */
    fprintf(fp, "2) LABORATORIES WITH SHORTAGES:\n");
    char reportedLabs[MAX_EQUIPMENT][30];
    int labCnt = 0;
    for (int i = 0; i < count; i++) {
        strcpy(arr[i].status, classify(&arr[i]));
        if (strcmp(arr[i].status, "Critical") == 0 || strcmp(arr[i].status, "Low Availability") == 0) {
            int already = 0;
            for (int k = 0; k < labCnt; k++)
                if (strcmp(reportedLabs[k], arr[i].labName) == 0) already = 1;
            if (!already) {
                fprintf(fp, "   - %s (e.g. %s is %s)\n", arr[i].labName, arr[i].name, arr[i].status);
                strcpy(reportedLabs[labCnt++], arr[i].labName);
            }
        }
    }
    if (labCnt == 0) fprintf(fp, "   None. All laboratories are adequately stocked.\n");
    fprintf(fp, "\n");

    /* Equipment requiring maintenance */
    fprintf(fp, "3) EQUIPMENT REQUIRING MAINTENANCE:\n");
    int maintFound = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(arr[i].status, "Under Maintenance") == 0) {
            fprintf(fp, "   - ID:%d %s (Lab: %s, Last Maint: %s)\n",
                    arr[i].id, arr[i].name, arr[i].labName, arr[i].maintenanceDate);
            maintFound++;
        }
    }
    if (!maintFound) fprintf(fp, "   None.\n");
    fprintf(fp, "\n");

    /* Total available equipment (units) */
    int totalAvailable = 0;
    for (int i = 0; i < count; i++) totalAvailable += arr[i].availableUnits;
    fprintf(fp, "4) TOTAL AVAILABLE EQUIPMENT UNITS: %d\n\n", totalAvailable);

    /* Equipment requiring immediate replacement/procurement */
    fprintf(fp, "5) EQUIPMENT REQUIRING IMMEDIATE REPLACEMENT/PROCUREMENT:\n");
    int replaceFound = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(arr[i].status, "Critical") == 0 || arr[i].usageCount >= MAX_USAGE_LIMIT) {
            fprintf(fp, "   - ID:%d %s (Lab: %s, Status: %s, Usage: %d)\n",
                    arr[i].id, arr[i].name, arr[i].labName, arr[i].status, arr[i].usageCount);
            replaceFound++;
        }
    }
    if (!replaceFound) fprintf(fp, "   None.\n");

    fclose(fp);

    /* Also show on screen */
    printf("\nReport generated and saved to '%s'.\n", REPORT_FILE);
    FILE *rf = fopen(REPORT_FILE, "r");
    if (rf) {
        char line[200];
        printf("\n--------------------- REPORT PREVIEW ---------------------\n");
        while (fgets(line, sizeof(line), rf)) printf("%s", line);
        fclose(rf);
    }
}

/* ============================================================================
   FILE HANDLING
   ============================================================================ */
void saveToFile(const Equipment *arr, int count) {
    FILE *fp = fopen(DATA_FILE, "w");
    if (fp == NULL) { printf("Error: cannot open '%s' for writing.\n", DATA_FILE); return; }
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%s|%s|%s|%s|%d|%d|%d\n",
                arr[i].id, arr[i].name, arr[i].category, arr[i].labName,
                arr[i].condition, arr[i].maintenanceDate,
                arr[i].quantity, arr[i].availableUnits, arr[i].usageCount);
    }
    fclose(fp);
}

int loadFromFile(Equipment *arr) {
    FILE *fp = fopen(DATA_FILE, "r");
    if (fp == NULL) return 0;   /* first run - no file yet */
    int cnt = 0;
    Equipment tmp;
    while (cnt < MAX_EQUIPMENT &&
           fscanf(fp, "%d|%49[^|]|%29[^|]|%29[^|]|%19[^|]|%14[^|]|%d|%d|%d\n",
                  &tmp.id, tmp.name, tmp.category, tmp.labName,
                  tmp.condition, tmp.maintenanceDate,
                  &tmp.quantity, &tmp.availableUnits, &tmp.usageCount) == 9) {
        strcpy(tmp.status, classify(&tmp));
        arr[cnt++] = tmp;
    }
    fclose(fp);
    return cnt;
}
