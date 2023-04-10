#ifndef _CURRENT_DIRECTORY_STACK_H_
#define _CURRENT_DIRECTORY_STACK_H_


struct CURRENT_DIR_STACK {
    int cd_stack[256];
    int top;
};

void init_current_dir_stack(struct CURRENT_DIR_STACK *cds);

void push_current_dir(struct CURRENT_DIR_STACK * cds, int dir);

char* pop_current_dir(struct CURRENT_DIR_STACK * cds);

char* get_top_current_dir(struct CURRENT_DIR_STACK * cds);

void reverse_current_dir(struct CURRENT_DIR_STACK * cds);


#endif