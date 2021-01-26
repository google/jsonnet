#include <stdio.h>

int main()
{
	__builtin_trap();
	printf("hello world\n");
	return 0;
}
