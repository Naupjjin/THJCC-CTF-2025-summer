#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

# define NOTE_ARR 3

struct NOTE* NOTE_ARRAY[NOTE_ARR]={NULL};

struct NOTE{
    char title[0x10];
    char description[0x10];
};

void Menu(){
    puts("==== NAUPMD ====");
    puts("(1) CREATE NOTE");
    puts("(2) DELETE NOTE");
    puts("(3) SHOW NOTE");
    puts("==== NAUPMD ====");
    printf("USER @ NAUPMDshell > ");
}

void build_note(const char* title, int index) {
    
    printf("Description for '%s' > ", title);
    struct NOTE* new_note = (struct NOTE*)malloc(sizeof(struct NOTE));
    if (new_note == NULL) {
        puts("[!]Failed to allocate memory for note.");
        exit(0);
    }

    strncpy(new_note->title, title, sizeof(new_note->title) - 1);
    new_note->title[sizeof(new_note->title) - 1] = '\0';

   
    scanf("%15s", new_note->description);

    NOTE_ARRAY[index] = new_note;
    printf("Note[%d] '%s' created successfully.\n", index, title);
}

void flush_stdin(){
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


void CREATE_NOTE() {
    
    int input_size;
    printf("Size > ");
    scanf("%d", &input_size);
    
    char* titles = (char*)calloc(1, input_size);

    if (input_size && titles){
        flush_stdin();
        printf("Enter titles > ");
        fgets(titles, input_size, stdin);
        titles[strcspn(titles, "\n")] = '\0';
    }
    
    char* token = strtok(titles, ",");
    int index = 0;

    while (token != NULL) {
        if (index >= NOTE_ARR) {
            puts("[!]Your note space is full... please purchase NAUPMD premium");
            break;
        }

        if (NOTE_ARRAY[index] != NULL) {
            index++;
            continue;
        }

        build_note(token, index);
        index++;

        token = strtok(NULL, ",");
    }
    
    free(titles);
}

int read_choice(){
    int choice;
    scanf("%d",&choice);
    return choice;
}

void SHOW_NOTES() {
    int DEBUG = 0;
    if (DEBUG){
        puts("==== NOTES ====");
        for (int i = 0; i < NOTE_ARR; i++) {
            if (NOTE_ARRAY[i] != NULL) {
                printf("[%d] Title: %s | Description: %s\n", i, NOTE_ARRAY[i]->title, NOTE_ARRAY[i]->description);
            }
        }
    }
    else {
        puts("I forget to delete debug...Because I go to watch AVE Mujica.");
    }
}

void DELETE_NOTE() {
    int index;
    printf("Index > ");
    scanf("%d", &index);

    if (index < 0 || index >= NOTE_ARR || NOTE_ARRAY[index] == NULL) {
        puts("[!] Invalid note index.");
        exit(0);
    } else {
        free(NOTE_ARRAY[index]);
        NOTE_ARRAY[index] = NULL;
        printf("Note[%d] deleted successfully.\n", index);
    }
}

void BACKDOOR(){
    unsigned long your_input;
    unsigned long admin_password = (unsigned long)&atoi;
    void *ptr = NULL;
    char username[0x60];
    char buffer[0x10];

    flush_stdin();
    puts("Verify...");
    printf("Input username > ");
    scanf("%95s",username);
    
    flush_stdin();
    printf("Input password > ");
    if (scanf("%lu", &your_input) != 1) { 
        puts("[!] Input error ...");
        exit(0);
    }

    if (your_input == admin_password) {
        puts("[!] Hi Naup ... This is system debug backdoor.");
        system("/bin/sh");
    } else {
        puts("[!] BAD Hacker !!!!!");
        exit(0);
    }
    
    
}

void init_IOBUF(){
    setvbuf(stdin, 0, _IONBF, 0);
    setvbuf(stdout, 0, _IONBF, 0);    
}

int main()
{

    init_IOBUF();
    int choice;
    
    
    while(true){
        Menu();
        choice = read_choice();
        
        if(choice == 1) {
            CREATE_NOTE();
        } else if (choice == 2) {
            DELETE_NOTE();
        } else if (choice == 3) {
            SHOW_NOTES(); 
        } else if (choice == 4) {
            BACKDOOR();  
        } else {
            puts("EXIT NAUPMD thanks ~");
            break;
        }
        
    }
    
    return 0;
}
