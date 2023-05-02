#ifndef _DIRECTORSTACK_H
#define _DIRECTORSTACK_H

struct DIR_STACK_NAME {
    char path[255];
};

struct DIR_STACK {
    struct DIR_STACK_NAME stack[256];
    int top;
};

void init_dir_stack(struct DIR_STACK *stack);

void push_dir(struct DIR_STACK * ds, char *path);

char* pop_dir(struct DIR_STACK * ds);

char* get_top_dir(struct DIR_STACK * ds);

void reverse_dir(struct DIR_STACK * ds);

struct DIR_STACK get_dir_stack(char * request, int * validate);

#endif