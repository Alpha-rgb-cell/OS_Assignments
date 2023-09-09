#include <stdio.h>

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int n = 10; // Adjust the Fibonacci number you want to calculate
    printf("Fibonacci(%d) = %d\n", n, fib(n));
    return 0;
}