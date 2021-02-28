#include <stdio.h>
#include <stdlib.h>


int sum(int, int);

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("Usage: %s a b \n", argv[0]);
        return 0;
    }

    int a = atoi(argv[1]);
    int b = atoi(argv[2]);

    int result = sum(a, b);

    printf("Result of %d + %d is %d\n", a, b, result);
}


int sum(int a, int b)
{
    printf("a is %d, b is %d\n", a, b);
    return a + b;
}