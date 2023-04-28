#ifndef _CURRENT_DIRECTORY_STACK_H_
#define _CURRENT_DIRECTORY_STACK_H_
#include "stdtype.h"

struct CURRENT_DIR_STACK {
    uint8_t cd_stack[256];
    char* cd_stack_name[256];
    int top;
};

void init_current_dir_stack(struct CURRENT_DIR_STACK *cds);

void push_current_dir(struct CURRENT_DIR_STACK * cds, uint8_t dir, char* dir_name);

uint8_t pop_current_dir(struct CURRENT_DIR_STACK * cds);

char * get_top_current_dir_name(struct CURRENT_DIR_STACK * cds);

uint8_t get_top_current_dir(struct CURRENT_DIR_STACK * cds);

void reverse_current_dir(struct CURRENT_DIR_STACK * cds);

char* convert_to_paths(struct CURRENT_DIR_STACK * cds);

#endif