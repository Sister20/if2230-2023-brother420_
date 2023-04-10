#include "lib-header/currentdirectorystack.h"

void init_current_dir_stack(struct CURRENT_DIR_STACK *cds){
    cds->top = 0;
    cds->cd_stack[0] = 2;
}

void push_current_dir(struct CURRENT_DIR_STACK * cds, int dir){
    cds->top++;
    cds->cd_stack[cds->top] = dir;
}

int pop_current_dir(struct CURRENT_DIR_STACK * cds){
    int dir = cds->cd_stack[cds->top];
    cds->top--;
    return dir;
}

int get_top_current_dir(struct CURRENT_DIR_STACK * cds){
    return cds->cd_stack[cds->top];
}

void reverse_current_dir(struct CURRENT_DIR_STACK * cds){
    int i = 0;
    int j = cds->top;
    while (i < j){
        int temp = cds->cd_stack[i];
        cds->cd_stack[i] = cds->cd_stack[j];
        cds->cd_stack[j] = temp;
        i++;
        j--;
    }
}