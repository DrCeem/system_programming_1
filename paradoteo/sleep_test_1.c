#include <stdio.h>
#include <unistd.h>

int main() {

    for(int i = 0; i < 5 ; i++) {
        //printf("Sleeping...\n");
        sleep(10);
    }
    return 0;
}