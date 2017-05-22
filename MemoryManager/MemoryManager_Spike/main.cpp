
#include <iostream>
#include <crtdbg.h>

#include "MemoryManager\Mnemosyne.h"

#include <ctime>
#include <Windows.h>
void main(void)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Mnemosyne::Initialize(750);

	// CODE GOES BETWEEN THESE

	Mnemosyne::ShowHeap();
	system("pause");

	void* mempieces[40];

	srand((unsigned int)time(0));

	for (int i = 0; i < 40; ++i)
	{
		switch (rand() % 6)
		{
		case 0:
			mempieces[i] = new char;
			break;
		case 1:
			mempieces[i] = new short;
			break;
		case 2:
			mempieces[i] = new int;
			break;
		case 3:
			mempieces[i] = new long long;
			break;
		case 4:
			mempieces[i] = new char[16];
			break;
		case 5:
			mempieces[i] = new short[16];
			break;
		case 6:
			mempieces[i] = new int[16];
			break;
		case 7:
			mempieces[i] = new long long[16];
			break;
		}

		Mnemosyne::ShowHeap();
		system("pause");
	}


	for (int i = 0; i < 25; ++i)
	{
		delete mempieces[i];

		Mnemosyne::ShowHeap();
		system("pause");

		delete mempieces[39 - i];

		Mnemosyne::ShowHeap();
		system("pause");
	}

	delete malloc(sizeof(int));

	Mnemosyne::Shutdown();

	std::cout << std::endl << std::endl;
	system("pause");
}