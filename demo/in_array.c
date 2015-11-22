#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int32_t in_array(const char* test_string, const char* test_array[]) {
    const char** needle = test_array;
    while (*needle) {
        printf("%s\n", *needle);
        if (strcmp(test_string, *needle) == 0) {
            return 1;
        }
        needle += 1;
    }
    return 0;
}

int main() {
    const char* test_array[] = {"aaa", "bbb", "ccc", NULL};
    //const char* test_array[] = {"aaa", NULL};
    printf("%s, addr:%lu\n", test_array[0], &test_array[0]);
    printf("%s, addr:%lu\n", test_array[1], &test_array[1]);
    printf("%s, addr:%lu\n", test_array[2], &test_array[2]);
    printf("sizeof(const char*)=%u\n", sizeof(const char*));
    if (in_array("ccc", test_array)) {
        printf("ccc in array\n");
    }

    //if (in_array("bbb", {"ccc", "bbb", "aaa", NULL})) {
        //printf("bbb in array\n");
    //}
    return 0;
}
