#pragma once

class Mnemosyne final
{
public:

	static void Initialize(size_t numBytes);
	static Mnemosyne* GetInstance();
	static void ShowHeap();
	static void Shutdown();

private:

	Mnemosyne() {}

	friend void* operator new(size_t size);
	friend void operator delete(void* ptr);

	static Mnemosyne* m_pInstance;

	struct HeapPool
	{
		struct MemoryBlock
		{
			// Bit field; the alive is a single bit in the uint while the length is the rest.
			// Length includes the memory block itself.
			// the bit pattern is as follows:
			// ALLL LLLL   LLLL LLLL   LLLL LLLL   LLLL LLLL
			unsigned int length : 31, alive : 1;
		};

		unsigned char* memory;
		unsigned int memoryLength;

		MemoryBlock* first;

	};

	HeapPool m_pool;

	char m_debugString[1024];

};