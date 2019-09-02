
#include "TypeDefinitions.h"
#include "Array.h"

struct huff_l
{
	s32 left;
	s32 right;
	huff_l *parent;
	u32 decoded;
	u32 combinedCount;
	u32 used;
};

struct h_encoding
{
	u32 encoding;
	u32 decoded;
	u32 encodedLength;
	u32 decodedLength;
	
};
u32 FindBestMatchToCombine(u32 &left ,u32 &right, huff_l * writeNew, u32 baseUsable,huff_l * basePtr)
{
	huff_l * bestLeftMAT_HISTOch;
	huff_l * nextUsableAlphabet = 0;
	huff_l * alphabet = basePtr + baseUsable;
	while(alphabet < writeNew)
	{
		if(alphabet->used)
			alphabet++;
		else 
		{
			if(alphabet>=writeNew)
				return 0;
			baseUsable = alphabet - basePtr;
			huff_l * leftPtr = alphabet;
			huff_l * rightPtr = alphabet+1;
			if(rightPtr>=writeNew)
			{
				return 0;
			}
			bestLeftMAT_HISTOch = rightPtr;
			alphabet+=2;
			while(alphabet < writeNew)
			{
				if(!alphabet->used && bestLeftMAT_HISTOch->combinedCount + leftPtr->combinedCount > alphabet->combinedCount + leftPtr->combinedCount)
						bestLeftMAT_HISTOch = alphabet;
				alphabet++;
			}
			left =  leftPtr - basePtr;
			right = bestLeftMAT_HISTOch- basePtr;
		}
	}
	return baseUsable;
}
#define Swap(buffer,i,j) {huff_l t =  buffer[i]; buffer[i] = buffer[j]; buffer[j] = t;}
struct histogram_record
{
	u32 dAT_HISTOa;
	u32 count;
};


#define AT_HISTO(i) ((huff_l*)(histogram[i]))
u8* Hencode(u8 * raw,u32 length,u32 & encodedLength)
{
	Array histogram = CArray(huff_l);
	u32 count = 0; 
	u32 histogramCount = 0;
	
	for(u32 i = 0 ; i < length; i++)
	{
		u32 j = 0;
		for(;j < histogram.count;j++)
		{
			u32 decoded = AT_HISTO(j)->decoded;
			if(decoded == (u32)raw[i])
				break;
		}
		if(j == histogram.count) 
		{	
			huff_l t = {0,0,0,*(raw + i),0,0};
			histogram.AddElement(&t,histogram.count);
		}
		count++;
		AT_HISTO(j)->combinedCount++;
	}	

	for(u32 i = 0 ; i < histogram.count; i++)
	{	
		for (u32 j = i+1 ;j< histogram.count; j++)
		{
			if(AT_HISTO(j)->combinedCount< AT_HISTO(i)->combinedCount)
				Swap(((huff_l*)histogram.mem),j,i);
		}
	}

	huff_l * root = 0;
	u32 usableAlphabet = 0;
	u8 alphabetCount = histogram.count;

	for(;;)
	{
		u32 right;
		u32 left;
		usableAlphabet = FindBestMatchToCombine(left,right,AT_HISTO(histogram.count),usableAlphabet ,AT_HISTO(0));
		
		huff_l* leftPtr = AT_HISTO(left);
		huff_l* rightPtr = AT_HISTO(right);
		leftPtr->used = true;
		rightPtr->used = true;
		huff_l toInsert = {left,right,0,0,rightPtr->combinedCount + leftPtr->combinedCount,0};
		histogram.AddElement(&toInsert,histogram.count);

		leftPtr->parent = AT_HISTO(histogram.count-1);
		rightPtr->parent = AT_HISTO(histogram.count-1);
		root = AT_HISTO(histogram.count-1);
		if(toInsert.combinedCount == length)
			break;
	}

	
	h_encoding * encodings = (h_encoding *)alloc.PushSize(alphabetCount*sizeof(h_encoding));
	for(u32 i = 0 ; i< alphabetCount;i++)
	{
		huff_l * currentNode = AT_HISTO(i);
		u8 encoding = 0;
		u8 currentBitCount = 0;
		while(currentNode->parent)
		{
			if(currentNode->parent->left == currentNode - AT_HISTO(0))
			{
				encoding = encoding | (1<<currentBitCount);
			}
			currentNode = currentNode->parent;
			currentBitCount++;
		}

		encodings[i].encoding = encoding;
		encodings[i].decoded = AT_HISTO(i)->decoded;
		encodings[i].encodedLength = currentBitCount;
		encodings[i].decodedLength = sizeof(encodings[i].decoded);
	}

	alloc.FreeBlock(encodings);
	return raw;
}

u8* Hdecode(u8* encoded,u32 length,u32 &decodedLength)
{
	return encoded;
}

void HuffmanTest()
{
	u8 str[] = "aaaaaabbb12348712380947147238907481237890471238904712304712380897df890s7sdf890asdufsjiuqwer8907wqe890r78901237489012347890cc";
	//u8 str[] = "aaaaaabbb";
	u32 encodedLength =0;
	Hencode(str,sizeof(str),encodedLength);
	
}