#include "lib-header/currentdirectorystack.h"

void init_current_dir_stack(struct CURRENT_DIR_STACK *cds){
    cds->top = 0;
    cds->cd_stack[0] = 2;
    cds->cd_stack_name[0] = "root";
}

void push_current_dir(struct CURRENT_DIR_STACK * cds, uint8_t dir, char* dir_name){
    cds->top++;
    cds->cd_stack[cds->top] = dir;
    cds->cd_stack_name[cds->top] = dir_name;
}

uint8_t pop_current_dir(struct CURRENT_DIR_STACK * cds){
    int dir = cds->cd_stack[cds->top];
    cds->top--;
    return dir;
}

char * get_top_current_dir_name(struct CURRENT_DIR_STACK * cds){
    return cds->cd_stack_name[cds->top];
}

uint8_t get_top_current_dir(struct CURRENT_DIR_STACK * cds){
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

char * convert_to_paths(struct CURRENT_DIR_STACK * cds){
    int i = 1;
    int j = 0;
    char temp[255];
    char * path = temp;
    while (i <= cds->top){
        path[j] = '/';
        j++;
        int k = 0;
        while (cds->cd_stack_name[i][k] != '\0'){
            path[j] = cds->cd_stack_name[i][k];
            j++;
            k++;
        }
        i++;
    }
    path[j] = '\0';
    return path;
}