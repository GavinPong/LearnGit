#include <stdint.h>
#include <stdio.h>
#include <typeinfo>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#pragma warning(disable:4996) 
#endif

#ifdef __linux__
#include <unistd.h>
#define	Sleep(m) usleep(1000 * (m))
#endif



using namespace std;

void archiveTest(const char* pstrJsonFile);
int32_t main(int32_t i32Argc, char *argv[])
{
	archiveTest("./archiveJsonTest.json");
	while (1)
	{
		Sleep(100);
	}

	return 0;
}
