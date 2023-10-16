#ifndef DUMMY_MAIN_H
#define DUMMY_MAIN_H

#include <stdio.h>

// Define the entry function for user programs
int dummy_main(int argc, char **argv);

// The following code should not be modified by the user
int main(int argc, char **argv) {
    /* You can add any code here you want to support your SimpleScheduler implementation */


    int ret = dummy_main(argc, argv);
    return ret;
}

#define main dummy_main

#endif
