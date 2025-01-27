#include <stdio.h>

extern int foo;

int main(int argc, char** argv) {
    fprintf(stderr, "Hello, world! %d\n", foo);
    return 0;
}
