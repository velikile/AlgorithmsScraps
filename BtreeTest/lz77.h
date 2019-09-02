
#include "TypeDefinitions.h"
#define ALLOC_GLOB 1<<24
#include "Allocator.h"
#include <string.h>

#define max(a,b) (a)>(b) ? (a) : (b)
#define min(a,b) (a)<(b) ? (a) : (b)

#define MAX_ENCODED_LEGNTH 255
#define MAX_BACK_DISTANCE 255

struct sliding_window
{
	u32 start;
	u32 end;
};

struct match_handle
{
	u8 back;
	u8 length;
	u8 next;

};
u8* LZ77Encode(u8* input,u32 length,u32 &encodedLength)
{
	assert(input);
	sliding_window sw = {0,min(length,12)};
	
	u8* inputItr = input + sw.start;

	match_handle matches[4196];
	u32 matchCount = 0;
	u32 biggestMatchLength = 0;
	while(sw.start <= length)
	{
		u8 * startOfSlidingWindow = input + sw.start;
		u8 * startOfSlidingWindowItr = startOfSlidingWindow;
		inputItr = startOfSlidingWindow -1;

		match_handle matchHandle = {0,0,*startOfSlidingWindow};

		u8 * inputItrStart = inputItr;
		
		while(inputItr < input + length && inputItr >= input)
		{	
			s32 matchLength = 0;
			//find best match
			while(*startOfSlidingWindowItr &&
				  *startOfSlidingWindowItr == *inputItr &&
				   startOfSlidingWindow - inputItr < MAX_BACK_DISTANCE &&
				   matchLength < MAX_ENCODED_LEGNTH)
			{
				startOfSlidingWindowItr++;
				inputItr++;				
				matchLength++;
			}
			biggestMatchLength  = max(biggestMatchLength,matchLength);
			if(matchLength > matchHandle.length)
			{
				matchHandle.back = startOfSlidingWindowItr - inputItr;
				matchHandle.length = matchLength;
				matchHandle.next = *startOfSlidingWindowItr;
			}
			startOfSlidingWindowItr = startOfSlidingWindow;
			if(startOfSlidingWindow - inputItr < MAX_BACK_DISTANCE)
				inputItr = --inputItrStart;
			else 
				break;
		}
		sw.start += matchHandle.length +1;
		sw.end += matchHandle.length +1;
		matches[matchCount++] = matchHandle;	
	}
	u32 currentMemSize = 1024;
	u8* ret = (u8*)alloc.PushSize(currentMemSize);
	for(s32 i = 0; i < matchCount;i++)
	{		
			if(encodedLength + sizeof(match_handle) >= currentMemSize)
			{
				currentMemSize*=2;
				u32 fail =  false;
				ret = (u8*)alloc.UpdateSize(ret,currentMemSize,fail);
			}
			ret[encodedLength++] = matches[i].back;
			ret[encodedLength++] = matches[i].length;		
			ret[encodedLength++] = matches[i].next;	
			
	}
	return ret; 
}
u8 * LZ77Decode(u8* input,u32 length,u32 &decodedLength)
{
	assert(input);
	u8 * ret = (u8*)alloc.PushSize(length*10);
	for(s32 i = 0 ; i<length;i++)
	{
		u8 goBack = input[i++];
		u8 matchCount = input[i++];
		u8 nextCharacter = input[i];
		s32 copiedCount = 0;
		s32 decodedLengthStart = decodedLength - goBack;
		if(decodedLengthStart < 0) decodedLengthStart = 0;
		while(copiedCount < matchCount)
		{
			ret[decodedLength++] = ret[decodedLengthStart + copiedCount++];
		}
		ret[decodedLength] = nextCharacter;
		if(nextCharacter)
			decodedLength++;
		else break;
	}
	return ret;
}
void TestLZ77()
{
	s8 * raw = "1111111111111111122222222222222222222222222111111111111111111111111111";
	u32 rawLength = strlen(raw),encodedLength = 0,decodedLength = 0;
	u8 * res = LZ77Encode((u8*)raw,rawLength,encodedLength);
	u8 * decoded = LZ77Decode(res,encodedLength,decodedLength);

	printf("%s",raw);
	printf("\n\n\n\n\n");
	printf("%s",decoded);

	if(strcmp(raw,(s8*)decoded)==0)
	{
		printf("ok");
	}
}