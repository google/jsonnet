#include <iostream>
#include "debugbreak.h"

int main()
{
	debug_break();
	std::cout << "hello, world\n";
	return 0;
}
