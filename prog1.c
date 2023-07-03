#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]){
    if(argc == 2){
        printf("arg1: %s\n", argv[1]);
    }
    else if(argc == 3){
        printf("arg1: %s // arg2: %s\n", argv[1], argv[2]);
    }
    else if(argc == 4){
        printf("arg1: %s // arg2: %s // arg3: %s\n", argv[1], argv[2], argv[3]);
    }
    
    printf("Executing program aux 'prog1'\n");
    int i = 1;
    while (1){
        // printf("Looping (prog1)...\n");
        // sleep(1);
        // i++;
    }
    return 0;
}