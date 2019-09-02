#ifndef ARRAY_H
#define ARRAY_H

#define ALLOC_GLOB 1<<24
#include "Allocator.h"

#define CTASSERTT(ctpred) int Ok[(ctpred) ? 1 : 0];

typedef unsigned char ST; // smallest type size of 1 byte;
typedef short SM; // size of 2 bytes 
typedef int SW; // size of 4 bytes;
typedef long SB;// size of 8 bytes;

#define CArray(t) Array(sizeof(t));
struct Array
{
	void * mem;
	int count;
	int size;
	int sizePerElement;
	Array(int elementSize)
	{
		size = 0;
		count = 0;
		mem = 0;
		sizePerElement = elementSize;
	}
	void * operator[](const u32 &i)
	{
		assert(mem);
		u32 addr = (u32)mem;
		return (void*)(addr + i *sizePerElement);
	}
	void AddElement(void * e, int index)
	{
		count++;
		assert(index>=0 && index<=count);

		if(size == count-1)
		{
			size = size*2 + 1;
			b32 fail = false;
			mem = alloc.UpdateSize(mem,size*sizePerElement,fail);
			assert(!fail);
		}
		if(index != count -1)
		{
			ST * tmem  = (ST *)mem;
			
			if(sizePerElement % sizeof(SB) == 0)
			{
				SB * tmeml = (SB *)mem;
				for(int i = count; i>index; i--)
					tmeml[i] = tmeml[i-1];
			}
			else if(sizePerElement % sizeof(SW) == 0)
			{
				SW * tmemi = (SW *)mem;
				for(int i = count; i>index; i--)
					tmemi[i] = tmemi[i-1];
			}
			else if (sizePerElement % sizeof(SM) == 0)
			{
				SM * tmemm = (SM*) mem;
				for(int i = count; i>index; i--)
					tmemm[i] = tmemm[i-1];
			}
			else 
			{
				for(int i = count; i>index; i--)
					for(int j = 0; j<sizePerElement ; j++)			
						*(tmem + i*sizePerElement + j) = *(tmem+(i-1)*sizePerElement+j);
			}
		}

		
		memcpy((ST*)mem+index*sizePerElement,e,sizePerElement);
	}
	int FindElement(void * e)
	{
		assert(e);
		ST * tmem = ((ST *)mem);
		for( int i= 0 ; i<count;i++)
		{
			if(memcmp(tmem + i *sizePerElement,e,sizePerElement) == 0)
				return i;
		}
		return -1;
	}
	void DeleteElement(int index)
	{
		if(count == 0)
			return;
		assert(index>=0 && index < count);
		ST * tmem = ((ST *)mem);
		memcpy(tmem+index*sizePerElement,tmem +(index+1)*sizePerElement,(count-index-1)*sizePerElement);
		count--;
		if(size>>1 == count)
		{
			size >>= 1;
			if(size == 0)
				free(mem);
			else
			{
				b32 fail = false;
				mem = alloc.UpdateSize(mem,size*sizePerElement,fail);
				assert(!fail);
			}
		}
	}
};
void ArrayTest()
{
	Array a = CArray(int);
	
	for (int i = 0; i<20; i++)
	{
		a.AddElement(&i,rand()%(i+1));
	}
	for(int i =0 ; i<20;i++)
	{
		a.DeleteElement(a.FindElement(&i));
	}
}
#endif