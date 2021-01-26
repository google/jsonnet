#include <stdio.h>

#include "debugbreak.h"

int fib(int n)
{
	int r;
	if (n == 0 || n == 1)
		return 1;
	r = fib(n-1) + fib(n-2);
	if (r == 89) {
		debug_break();
	}
	return r;
}

int main()
{
	printf("%d\n", fib(15));
	return 0;
}
