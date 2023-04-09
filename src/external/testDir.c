// #include "stdmem.c"

#include <string.h>
#include <stdio.h>
#include "../directorystack.c"

int main() {
    char * request = "/home/abc/def/ghi";
    int validate = 1;
    struct DIR_STACK ds = get_dir_stack(request, &validate);
    if (validate == 0){
        printf("Invalid request\n");
        return 0;
    }
    reverse_dir(&ds);
    char * name = pop_dir(&ds);
    while (name[0] != '\0'){
        printf("%s\n", name);
        name = pop_dir(&ds);
    }
    return 0;
}