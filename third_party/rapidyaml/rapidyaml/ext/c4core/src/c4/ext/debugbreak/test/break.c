#include <stdio.h>
#include "debugbreak.h"

int main()
{
	debug_break();
	printf("hello world\n");
	return 0;
}
