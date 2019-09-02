#ifndef RLE_H
#define RLE_H

#include <stdlib.h>
#include <string.h>
#include "TypeDefinitions.h"
#define ALLOC_GLOB 1<<24
#include "Allocator.h"

#define MAX_RUN_LENGTH 128 

struct delta
{
	u8 avg;
};

u8* RLEEncode(u8* str,u32 length,u32 &encodedLength)
{
	u8 * retItr = (u8*)alloc.PushSize(length*2);
	b32 fail = 0 ;
	u8 * ret = retItr;
	if(!ret) 
		return 0;

	u8 run = 0;
	u8 * prev = str;
	for(u32 i = 1 ; i<=length ;i++)
	{
		if(run == MAX_RUN_LENGTH)
		{
			*retItr++ = *(str+i);
			*retItr++ = *(str+i);
			*retItr++ = run;
			encodedLength+=2;
			run = 0;
		}
		if(*prev == *(str + i))
		{
			run++;
		}
		else if(run==0)
		{
			*retItr++ = *prev;
			encodedLength++;
		}
		else 
		{
			*retItr++ = *prev;
			*retItr++ = *prev;
			*retItr++ = run;
			run = 0;
			encodedLength+=3;
		}
		prev = str+i;
	}
	*retItr=0;
	ret = (u8*)alloc.UpdateSize(ret,encodedLength,fail);
	return ret;
}

u8* RLEDecode(u8* str,u32 length,u32 &decodedLength)
{
	u32 initialSize = 1024;
	u8 * retItr = (u8*)alloc.PushSize(initialSize);
	u8* ret = retItr;
	b32 fail = 0;
	u8 prev = *str;
	for(s32 i = 1 ; i <=length;i++)
	{
		if(*(str+i) == prev)
		{
			u8 run = *(str+i+1);
			while(run>0)
			{
				decodedLength++;
				if(decodedLength == initialSize)
				{
					initialSize *= 2;
					
					ret = (u8*)alloc.UpdateSize(ret,initialSize,fail);
				}
				run--;
				*retItr++ = prev;
			}

			decodedLength++;
			*retItr++ = prev;
			i+=2; // skip the length byte
		}
		else
		{
			*retItr++ = prev;
			decodedLength++;
		}
		prev = *(str+i);
	}
	*retItr = 0;
	ret = (u8*)alloc.UpdateSize(ret,decodedLength + 1,fail);

	return ret;
}
void TestRLE()
{
	u32 encodedLength = 0;
	u32 decodedLength = 0;
	s8* toEncode = "Hello WORLD WWWWBBBB";
	u8* encoded = RLEEncode((u8*)toEncode,strlen(toEncode),encodedLength);
	u8* decoded = RLEDecode(encoded,encodedLength,decodedLength);
	if(strcmp((s8*)decoded,toEncode) == 0)
	{
		printf("%s",encoded);
		scanf("");
	}
}
#endif
