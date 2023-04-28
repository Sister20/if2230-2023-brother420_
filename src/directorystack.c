#include "lib-header/directorystack.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"

void init_dir_stack(struct DIR_STACK *stack) {
    stack->top = -1;
}

void push_dir(struct DIR_STACK * ds, char *path){
    ds->top++;
    memcpy(ds->stack[ds->top].path, path, 255);
}

char * pop_dir(struct DIR_STACK * ds){
    if (ds->top == -1)
        return "\0";
    char * path = ds->stack[ds->top].path;
    ds->top--;
    return path;
}

char* get_top_dir(struct DIR_STACK * ds){
    return ds->stack[ds->top].path;
}

void reverse_dir(struct DIR_STACK * ds){
    int i = 0;
    int j = ds->top;
    while (i < j){
        char temp[8];
        memcpy(temp, ds->stack[i].path, 8);
        memcpy(ds->stack[i].path, ds->stack[j].path, 8);
        memcpy(ds->stack[j].path, temp, 8);
        i++;
        j--;
    }
}

struct DIR_STACK get_dir_stack(char * request, int * validate){
    struct DIR_STACK ds;
    init_dir_stack(&ds);
    int i = 0;

    while (request[i] != '\0'){
        if (request[i] == '/'){
            i++;
            continue;
        }
        char path[8];
        int j = 0;
        while (request[i] != '/' && request[i] != '\0'){
            path[j] = request[i];
            i++;
            j++;
        }
        if (j > 8){
            *validate = 0;
            return ds;
        }
        while (j < 8){
            path[j] = '\0';
            j++;
        }
        push_dir(&ds, path);
    }
    
    return ds;
}