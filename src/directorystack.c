#include "lib-header/directorystack.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"

void init_dir_stack(struct DIR_STACK *stack) {
    stack->top = -1;
}

void push_dir(struct DIR_STACK * ds, char name[8]){
    ds->top++;
    memcpy(ds->stack[ds->top].name, name, 8);
}

char * pop_dir(struct DIR_STACK * ds){
    if (ds->top == -1)
        return "\0";
    char * name = ds->stack[ds->top].name;
    ds->top--;
    return name;
}

char* get_top_dir(struct DIR_STACK * ds){
    return ds->stack[ds->top].name;
}

void reverse_dir(struct DIR_STACK * ds){
    int i = 0;
    int j = ds->top;
    while (i < j){
        char temp[8];
        memcpy(temp, ds->stack[i].name, 8);
        memcpy(ds->stack[i].name, ds->stack[j].name, 8);
        memcpy(ds->stack[j].name, temp, 8);
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
        char name[8];
        int j = 0;
        while (request[i] != '/' && request[i] != '\0'){
            name[j] = request[i];
            i++;
            j++;
        }
        if (j > 8){
            *validate = 0;
            return ds;
        }
        while (j < 8){
            name[j] = '\0';
            j++;
        }
        push_dir(&ds, name);
    }
    
    return ds;
}