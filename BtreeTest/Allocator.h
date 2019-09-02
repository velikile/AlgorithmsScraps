
#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "TypeDefinitions.h"

struct mem_chunk
{	
	b32 free;
	int start;
	int end;
	void * mem;
	mem_chunk * prev;
	mem_chunk * next;
	u32 Size(){return end-start;};
};

typedef mem_chunk free_mem;
typedef mem_chunk allocated_mem;

struct allocator
{
#define CHUNKS_PTR_COUNT 1024
	allocated_mem *allocated[CHUNKS_PTR_COUNT];
	free_mem *freed[CHUNKS_PTR_COUNT];
	
	int freedChunksCount;
	int allocatedChunksCount;
	int totalMemorySize;
	void * memory;
	allocator(int initialSize)
	{	
		u32 sizeOfFreed = sizeof(freed)/sizeof(mem_chunk*);
		u32 sizeOfAllocated = sizeof(allocated)/sizeof(mem_chunk*);
		memset(freed,0,sizeOfFreed);
		memset(allocated,0,sizeOfAllocated);
		freedChunksCount = 1;
		allocatedChunksCount = 0;		
		memory = calloc(1,initialSize);
		mem_chunk chunk;
		
		chunk.start = sizeof(mem_chunk);
		chunk.end = initialSize;
		chunk.prev = 0;
		chunk.next = 0;
		chunk.free = true;
		//write a header data to the memory
		u32 memoryPtr = (u32)memory;
		chunk.mem = (void*)(memoryPtr + sizeof(mem_chunk)) ;
		memcpy(memory,&chunk,sizeof(mem_chunk));
		freed[0] = (mem_chunk*)memory;

		totalMemorySize = initialSize;
	}
	void * PushSize(u32 size)
	{
		u32 minSizeFit = 1<<31;
		s32 minIndexFit = -1;
		s32 si = FindInsertionFreeIndexBySize(size+sizeof(mem_chunk));
		for(s32 i = si;
			i<freedChunksCount;
			i++)
		{
			int fSize = freed[i]->Size();
			if(fSize >= size && minSizeFit > fSize)
			{
				minIndexFit = i;
				minSizeFit = fSize;
				break;
			}
		}
		if(minIndexFit == -1)
			return 0;


		u32 freedEnd = freed[minIndexFit]->end;
		mem_chunk *freedNext = freed[minIndexFit]->next;
		mem_chunk *freedPrev = freed[minIndexFit]->prev;

		allocated_mem toInsert = *freed[minIndexFit];
		toInsert.free = false;		
		toInsert.end = toInsert.start + size;
		*((mem_chunk *)toInsert.mem-1) = toInsert;

		s32 index = InsertToAllocatedSorted((mem_chunk*)toInsert.mem-1);
		
		if(freedEnd == toInsert.end + sizeof(mem_chunk))
		{
			RemoveFromFreed(minIndexFit);
		}
		else
		{
			freed[minIndexFit] = (mem_chunk*)((u8*)toInsert.mem + toInsert.Size());
			freed[minIndexFit]->free = true;
			freed[minIndexFit]->start = toInsert.end + sizeof(mem_chunk);
			freed[minIndexFit]->end = freedEnd;
			freed[minIndexFit]->mem = (u8*)freed[minIndexFit] + sizeof(mem_chunk);
			freed[minIndexFit]->next = freedNext;
			freed[minIndexFit]->prev = allocated[index];
		}
		allocated[index]->next = freed[minIndexFit];
		allocated[index]->prev = freedPrev;

		if(allocated[index]->prev)
			allocated[index]->prev->next = allocated[index];

		if(freed[minIndexFit]->next)
			freed[minIndexFit]->next->prev = freed[minIndexFit];
		return allocated[index]->mem;
	}

	void * UpdateSize(void * mem, u32 size,b32 & fail)
	{
		if(!mem)
		{
			mem = PushSize(size);
			return mem;
		}
		mem_chunk * currentChunk = (mem_chunk*)mem - 1;
		u32 chunkSize = currentChunk->Size();
		fail = false;
		if(chunkSize > size)
		{
			s32 freedSize = chunkSize - size;
			if(freedSize > sizeof(mem_chunk))
			{
				SplitChunk(currentChunk,size);
			}
			else 
			{
				currentChunk->end = currentChunk->start + size;
			}
		}
		else if(chunkSize < size)
		{
			void * retMem = PushSize(size);
			if(retMem)
				memcpy(retMem,mem,chunkSize);
			FreeBlock(mem);
			mem = retMem;
		}
		
		return mem;
	}
	u32 SplitChunk(mem_chunk * chunk,u32 splitIndex)
	{
		if(!chunk)
			return -1;
		mem_chunk * nextChunk = chunk->next;
		mem_chunk * freedChunk = (mem_chunk*)((u32)chunk->mem + splitIndex);
		freedChunk->free = true;
		freedChunk->end = chunk->end;
		freedChunk->start = chunk->start + splitIndex + sizeof(mem_chunk);
		freedChunk->mem = (void*)((u32)chunk->mem + splitIndex + sizeof(mem_chunk));
		
		freedChunk->prev = chunk;
		freedChunk->next = nextChunk;
		if(nextChunk)
		{
			nextChunk->prev = freedChunk;
		}
		chunk->next = freedChunk;
		chunk->end = chunk->start + splitIndex;
		return InsertToFreedSorted(freedChunk);
	}
	void ResetFreeBlock()
	{
		freed[0]->start =  sizeof(mem_chunk);
		freed[0]->end = totalMemorySize;
		freed[0]->prev = 0;
		freed[0]->next = 0;
		freed[0]->mem = (u8*)memory + sizeof(mem_chunk);
		freedChunksCount = 1;
	}
	u32 FindFreeIndex(mem_chunk *free)
	{
		u32 i = FindInsertionFreeIndexBySize(free->Size());

		for(u32 j = i ; j < freedChunksCount ; j++)
		{
			if(free == freed[j])
			{
				return j;
			}
		}
	}
	void FreeBlock(void * mem)
	{
		if (mem == 0)
			return;
		int i = FindInsertionAllocatedIndexByMem(mem);
		{
			b32 merged = false;
			assert(!allocated[i]->free);
			if(allocated[i]->mem == mem)
			{				
				if(allocated[i]->next && allocated[i]->next->free)
				{
					mem_chunk * free = allocated[i]->next;
					RemoveFromFreed(FindFreeIndex(free));
					allocated[i]->next = free->next;
					if(allocated[i]->next)
						allocated[i]->next->prev = allocated[i];
					allocated[i]->end = free->end;
					allocated[i]->free = true;
					InsertToFreedSorted(allocated[i]);
					merged = true;
				}
				if(allocated[i]->prev && allocated[i]->prev->free)
				{
					mem_chunk * free = allocated[i]->prev;
					if(allocated[i]->next)
						allocated[i]->next->prev = free;
					RemoveFromFreed(FindFreeIndex(free));
					free->next = allocated[i]->next;
					free->end = allocated[i]->end;
					if(merged)
						RemoveFromFreed(FindFreeIndex(allocated[i]));
					InsertToFreedSorted(free);
					merged = true;
				}
				if(!merged)
				{
					mem_chunk *freeChunk = allocated[i];
					freeChunk->free = true;
					InsertToFreedSorted(allocated[i]);
				}
				RemoveFromAllocated(i);
				
				if(allocatedChunksCount == 0) ResetFreeBlock();

				return;
			}
		}
		assert(!"Memory Couldn't be found");
	}

	int FindInsertionFreeIndexBySize(u32 size)
	{
		if(freedChunksCount== 0) return 0;
		int left = 0, 
			right = freedChunksCount-1;
		while (right - left > 1)
		{
			int cmpIndex = (left + right)/2;
			if(freed[cmpIndex]->Size() < size)
				left = cmpIndex;
			else 
				right = cmpIndex;
		}
		if(freed[right]->Size()< size)
				return right+1;
		else if (freed[left]->Size() >= size)
			return left;
		else if (freed[right]->Size() > size && freed[left]->Size() < size || 
				 freed[right]->Size() == size)
			return right;
	return right;
	}

	int FindInsertionAllocatedIndexByMem(void * mem)
	{
		if(allocatedChunksCount== 0) return 0;
		int left = 0, 
			right = allocatedChunksCount-1;
		while (right - left > 1)
		{
			int cmpIndex = (left + right)/2;
			if(allocated[cmpIndex]->mem < mem)
				left = cmpIndex;
			else 
				right = cmpIndex;
		}
		if(allocated[right]->mem < mem)
			return right+1;
		else if (allocated[left]->mem >= mem)
			return left;
		else if ((allocated[right]->mem > mem && allocated[left]->mem  < mem )|| 
			  allocated[right]->mem == mem)
			return right;

	return right;
	}
	s32 InsertToFreedSorted(mem_chunk *m)
	{
		int insertionIndex = FindInsertionFreeIndexBySize(m->Size());
		if(insertionIndex == 0)
		{
			memcpy(freed+1,freed,freedChunksCount*sizeof(void*));
			freed[0] = m;
		}
		else if(insertionIndex == freedChunksCount)
		{
			freed[freedChunksCount] = m;
		}
		else 
		{
			memcpy(freed+insertionIndex+1,freed+insertionIndex,(freedChunksCount - insertionIndex)*sizeof(void*));
			freed[insertionIndex] = m;
		}
		freedChunksCount++;
		return insertionIndex;
	}
	s32 InsertToAllocatedSorted(allocated_mem *am)
	{
		if(allocatedChunksCount == 0)
		{
			allocated[0] = am;
			allocatedChunksCount++;
			return 0;
		}
		else
		{
			int insertionIndex = FindInsertionAllocatedIndexByMem(am->mem);
			if(insertionIndex == 0)
			{
				memcpy(allocated+1,allocated,allocatedChunksCount*sizeof(am));
				allocated[0] = am;
			}
			else if(insertionIndex == allocatedChunksCount)
			{
				allocated[allocatedChunksCount] = am;
			}
			else 
			{
				memcpy(allocated+insertionIndex+1,allocated+insertionIndex,(allocatedChunksCount -insertionIndex)*sizeof(void*));
				allocated[insertionIndex] = am;
			}
			allocatedChunksCount++;
			return insertionIndex;
		}
	}

	void RemoveFromAllocated(int index)
	{
		assert(index < allocatedChunksCount && index >= 0);
		allocated[index]->free = true;
		memcpy(allocated+index,allocated+index+1,(allocatedChunksCount - index -1)*sizeof(void*));
		allocatedChunksCount--;
	}

	void RemoveFromFreed(int index)
	{
		assert(index < freedChunksCount && index >= 0);
		memcpy(freed+index,freed+index+1,(freedChunksCount- index -1)*sizeof(void*));
		freedChunksCount--;
	}
	~allocator()
	{
		if(memory) free(memory);
	}
};

void allocatorTest()	
{
	allocator aloc(1<<29);

	void * mem[120];
	for(int i = 0 ; i < 100;i++)
	{
		mem[i] = aloc.PushSize(1<<16);
	}
	for(int i = 0 ; i < 100;i++)
	{
		b32 fail = false;
		mem[i] = aloc.UpdateSize(mem[i],1<<17,fail);
		//mem[i] = aloc.PushSize(30);
	}
	for(int i = 100; i <120;i++)
	{
		mem[i] = aloc.PushSize(30);
	}
	for(int i = 50,j=49; i < 120 || j >=0 ; i++, j--)
	{
		if(i<120)
			aloc.FreeBlock(mem[i]);
		if(j>=0)
			aloc.FreeBlock(mem[j]);
	}
}
#endif
#ifdef ALLOC_GLOB 
	#ifndef ALLOC
	#define ALLOC
		allocator alloc(ALLOC_GLOB);
	#endif
#endif 