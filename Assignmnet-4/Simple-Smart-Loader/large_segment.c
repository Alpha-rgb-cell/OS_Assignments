// large_segment.c
#include <stdio.h>

int main() {
    char arr[8192];
    for (int i = 0; i < 8192; i++) {
        arr[i] = 'A';
    }

    printf("Large segment program\n");
    return 0;
}
