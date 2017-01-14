#include "precompiled.h"
#include "ciextra.h"

void createConsole()
{
	AllocConsole();
	std::fstream* fs = new std::fstream("CONOUT$");
	std::cout.rdbuf(fs->rdbuf());
}