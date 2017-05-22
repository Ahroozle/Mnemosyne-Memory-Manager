#include "Mnemosyne.h"

#include <iostream>
#include <exception>
#include <sstream>

#include <Windows.h>

//DEBUGOUTPUT(sprintf_s(Mnemosyne::GetInstance()->m_debugString, "", ...))
#define DEBUGOUTPUT(sprintf_call)								\
{																\
	sprintf_call;												\
	OutputDebugString(Mnemosyne::GetInstance()->m_debugString);	\
}

#define PrintByteHex(c) cout << (unsigned int)((c & 0xF0) >> 4) << (unsigned int)(c & 0x0F) /*<< ' '*/

using namespace std;

#pragma region Specialized Exceptions

class heap_pool_bad_alloc : public exception
{
public:
	heap_pool_bad_alloc(string str) throw() : exception(str.c_str(), 1) {}
};

#pragma endregion

Mnemosyne* Mnemosyne::m_pInstance = nullptr;

void Mnemosyne::Initialize(size_t numBytes)
{
	m_pInstance = (Mnemosyne*)malloc(sizeof(Mnemosyne));

	//TODO: Initialize heap pool

	m_pInstance->m_pool.memory = (unsigned char*)malloc(numBytes);
	m_pInstance->m_pool.memoryLength = numBytes;

	m_pInstance->m_pool.first = (Mnemosyne::HeapPool::MemoryBlock*)m_pInstance->m_pool.memory;
	m_pInstance->m_pool.first->alive = 0;
	m_pInstance->m_pool.first->length = numBytes;
}

Mnemosyne* Mnemosyne::GetInstance()
{
	return m_pInstance;
}

void Mnemosyne::ShowHeap()
{
	unsigned char* currByte = m_pInstance->m_pool.memory;
	unsigned char* lastByte = currByte + m_pInstance->m_pool.memoryLength;

	cout << hex;

	HANDLE  hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO sbi;

	SetConsoleCursorPosition(hConsole, { 0, 0 });


	GetConsoleScreenBufferInfo(hConsole, &sbi);


	WORD liveHeaderPalette = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE;
	WORD liveBodyPalette = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_RED;
	WORD deadHeaderPalette = BACKGROUND_BLUE;
	WORD deadBodyPalette = BACKGROUND_RED;

	unsigned int memBlockSize = sizeof(HeapPool::MemoryBlock);
	unsigned int currentLength;
	bool currentAlive;
	while (currByte != lastByte)
	{
		currentLength = ((HeapPool::MemoryBlock*)currByte)->length;
		currentAlive = ((HeapPool::MemoryBlock*)currByte)->alive;

		if (currentAlive)
			SetConsoleTextAttribute(hConsole, liveHeaderPalette);
		else
			SetConsoleTextAttribute(hConsole, deadHeaderPalette);


		for (unsigned int i = 0; i < memBlockSize; ++i)
		{
			PrintByteHex((*currByte));
			++currByte;
		}

		if (currentAlive)
			SetConsoleTextAttribute(hConsole, liveBodyPalette);
		else
			SetConsoleTextAttribute(hConsole, deadBodyPalette);

		for (unsigned int i = memBlockSize; i < currentLength; ++i)
		{
			PrintByteHex((*currByte));
			++currByte;
		}
	}

	cout << dec << '\n';

	SetConsoleTextAttribute(hConsole, sbi.wAttributes);
}

void Mnemosyne::Shutdown()
{
	unsigned char* currentHeader = m_pInstance->m_pool.memory;
	unsigned char* finalByte = currentHeader + m_pInstance->m_pool.memoryLength;


	DEBUGOUTPUT(sprintf_s(Mnemosyne::GetInstance()->m_debugString, "\n"))

	int leaksDetected = 0;
	while (currentHeader != finalByte)
	{
		if (((HeapPool::MemoryBlock*)currentHeader)->alive)
		{
			++leaksDetected;

			DEBUGOUTPUT(sprintf_s(Mnemosyne::GetInstance()->m_debugString, "MNEMOSYNE: Memory Leaked from address {0x%x}! %i bytes leaked.\n", (unsigned int)currentHeader, ((HeapPool::MemoryBlock*)currentHeader)->length - sizeof(HeapPool::MemoryBlock)))
		}

		currentHeader += ((HeapPool::MemoryBlock*)currentHeader)->length;
	}

	if (leaksDetected)
		DEBUGOUTPUT(sprintf_s(Mnemosyne::GetInstance()->m_debugString, "\nMNEMOSYNE: %i memory leaks were detected and dumped!\n\n", leaksDetected))
	else
		DEBUGOUTPUT(sprintf_s(Mnemosyne::GetInstance()->m_debugString, "MNEMOSYNE: No memory leaks were detected.\n\n"))

	free(m_pInstance->m_pool.memory);
	free(m_pInstance);

	m_pInstance = nullptr;
}

void* operator new(size_t size)
{
	// TODO:
	//	Scout out memory across the pool

	unsigned char* currentByte = Mnemosyne::GetInstance()->m_pool.memory;
	unsigned char* finalByte = currentByte + Mnemosyne::GetInstance()->m_pool.memoryLength;

	Mnemosyne::HeapPool::MemoryBlock* currHeader = nullptr;
	Mnemosyne::HeapPool::MemoryBlock* bestHeader = nullptr;
	unsigned int idealSize = size + sizeof(Mnemosyne::HeapPool::MemoryBlock);
	bool perfectLength = false;
	while (currentByte != finalByte)
	{
		currHeader = (Mnemosyne::HeapPool::MemoryBlock*)currentByte;

		if (!currHeader->alive)
		{
			if (currHeader->length == idealSize)
			{
				perfectLength = true;
				bestHeader = currHeader;
				break;
			}
			else if (currHeader->length >= idealSize)
			{
				if (bestHeader == nullptr || (bestHeader->length > currHeader->length))
					bestHeader = currHeader;
			}
		}

		currentByte += currHeader->length;
	}

	if (bestHeader != nullptr)
	{
		if (!perfectLength)
		{
			if (bestHeader->length - idealSize <= sizeof(Mnemosyne::HeapPool::MemoryBlock))
			{
				DEBUGOUTPUT(sprintf_s(Mnemosyne::GetInstance()->m_debugString, "\nMNEMOSYNE ERROR: Managed heap pool has run out of memory! Attempt to allocate new header chunk after allocating %i bytes has failed\n\n", size))
				throw heap_pool_bad_alloc("");
			}
			else
			{
				Mnemosyne::HeapPool::MemoryBlock* newheader = (Mnemosyne::HeapPool::MemoryBlock*)(((unsigned char*)bestHeader) + idealSize);
				newheader->alive = 0;
				newheader->length = bestHeader->length - idealSize;

				bestHeader->alive = 1;
				bestHeader->length = idealSize;
			}
		}

		//cout << "Successfully partitioned " << size << " bytes, starting at address " << bestHeader << '\n';
		return (((unsigned char*)bestHeader) + sizeof(Mnemosyne::HeapPool::MemoryBlock));
	}
	else
	{
		DEBUGOUTPUT(sprintf_s(Mnemosyne::GetInstance()->m_debugString, "\nMNEMOSYNE ERROR: Managed heap pool has run out of memory! Attempt to allocate %i bytes has failed.\n\n", size))
		throw heap_pool_bad_alloc("");
	}
}

void operator delete(void* ptr)
{
	Mnemosyne* instance = Mnemosyne::GetInstance();

	unsigned char* targetLocation = ((unsigned char*)ptr) - sizeof(Mnemosyne::HeapPool::MemoryBlock);

	//cout << "Reclaiming memory starting at address " << ptr
	//	 << " and ending theoretically " << ((Mnemosyne::HeapPool::MemoryBlock*)targetLocation)->length - sizeof(Mnemosyne::HeapPool::MemoryBlock)
	//	 << " bytes later.\n";

	unsigned char* prevHeader = nullptr;
	unsigned char* currHeader = instance->m_pool.memory;
	unsigned char* nextHeader = currHeader + ((Mnemosyne::HeapPool::MemoryBlock*)currHeader)->length;

	unsigned char* finalByte = currHeader + instance->m_pool.memoryLength;

	while (nextHeader != finalByte)
	{
		if (currHeader == targetLocation)
			break;

		prevHeader = currHeader;
		currHeader = nextHeader;
		nextHeader = currHeader + ((Mnemosyne::HeapPool::MemoryBlock*)currHeader)->length;
	}

	if (currHeader != targetLocation)
	{
		

		if (!(ptr > instance->m_pool.memory && ptr < finalByte))
		{
			DEBUGOUTPUT(sprintf_s(instance->m_debugString, "\nMNEMOSYNE WARNING: Could not find address {0x%x} inside of the managed heap pool! Did you use malloc() instead of new?\n\n", (unsigned int)ptr))
			free(ptr);
		}
		else
			DEBUGOUTPUT(sprintf_s(instance->m_debugString, "\nMNEMOSYNE WARNING: Could not find address {0x%x} inside of the managed heap pool! Are you trying to delete memory that has already been deleted?\n\n", (unsigned int)ptr))

	}
	else
	{
		if (nextHeader == finalByte)
			nextHeader = nullptr;

		if (nextHeader != nullptr && !((Mnemosyne::HeapPool::MemoryBlock*)nextHeader)->alive)
		{
			((Mnemosyne::HeapPool::MemoryBlock*)currHeader)->length += ((Mnemosyne::HeapPool::MemoryBlock*)nextHeader)->length;
		}

		if (prevHeader != nullptr && !((Mnemosyne::HeapPool::MemoryBlock*)prevHeader)->alive)
		{
			((Mnemosyne::HeapPool::MemoryBlock*)prevHeader)->length += ((Mnemosyne::HeapPool::MemoryBlock*)currHeader)->length;
		}

		((Mnemosyne::HeapPool::MemoryBlock*)currHeader)->alive = 0;
	}
}
