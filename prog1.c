#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
    printf("Executing program aux 'prog1'\n");
    while (1){
        printf("Looping...\n");
        sleep(1);
    }
    return 0;
}