#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "Allocator.h"
#include "rle.h"
#include "lz77.h"
#include "huffman.h"
#include <string.h>
#define CHILDREN_COUNT 5
#define VALUE_COUNT CHILDREN_COUNT -1

enum valueType
{
	CHAR,U_CHAR,SHORT,U_SHORT,INT,U_INT,FLOAT,DOUBLE,LONGLONG,U_LONGLONG 
};

int ceil(float b)
{
	return (b - (int)b) > 0 ? (int)(b+1) : (int)b;	
}
int floor(float b)
{
	return b<0 ? (int)(b-1) :(int)b;
}

struct value // this need to be cast to the correct type 
{
	union 
	{
		char cx;
		unsigned char ucx;
		short sx;
		unsigned short usx;
		int ix;
		unsigned int uix;
		float fx;
		double dx;
		long long llx;
		unsigned long long ullx;
	};
	valueType vt;

	value(){};
	value(char c)
	{
		cx = c;
		vt = CHAR;
	}
	value(unsigned char uc)
	{
		ucx = uc;
		vt = U_CHAR;
	}
	value(short s)
	{
		sx = s;
		vt = SHORT;
	}
	value(unsigned short us)
	{
		usx =us;
		vt = U_SHORT;
	}
	value(int i)
	{
		ix = i;
		vt = INT;
	}
	value(unsigned int ui)
	{
		uix = ui;
		vt = U_INT;
	}
	value(float f)
	{
		fx = f;
		vt = FLOAT;
	}
	value(double d)
	{
		dx = d;
		vt = DOUBLE;
	}
	value(long long ll)
	{
		llx = ll;
		vt = LONGLONG;
	}
	value(unsigned long long ull)
	{
		ullx = ull;
		vt = U_LONGLONG;
	}
	
	void operator = (value a)
	{
		// fully mutable ??? 
		vt = a.vt;
		switch(a.vt)
		{
		#define fillValue(type,v)case type: v = a.v ;break
			fillValue(CHAR,cx);
			fillValue(U_CHAR,ucx);
			fillValue(SHORT,sx);
			fillValue(U_SHORT,usx);	
			fillValue(INT,ix);
			fillValue(U_INT,uix);
			fillValue(FLOAT,fx);
			fillValue(DOUBLE,dx);
			fillValue(LONGLONG,llx);
			fillValue(U_LONGLONG,ullx);
		}	
		#undef fillValue
	}
#define cmp(type,v,op)case type: return v op a.v
#define CMP(op)bool operator op (value &a)\
{\
	switch(a.vt)\
	{\
		cmp(CHAR,cx,op);\
		cmp(U_CHAR,ucx,op);\
		cmp(SHORT,sx,op);\
		cmp(U_SHORT,usx,op);\
		cmp(INT,ix,op);\
		cmp(U_INT,uix,op);\
		cmp(FLOAT,fx,op);\
		cmp(DOUBLE,dx,op);\
		cmp(LONGLONG,llx,op);\
		cmp(U_LONGLONG,ullx,op);\
}\
return false;}
CMP(>)
CMP(>=)
CMP(<)
CMP(<=)
CMP(==)
};
#undef cmp
#undef CMP

struct btree
{	// children is always one more than the values count;
	btree * parent;
	btree * children;
	int childrenCount;
	value * values;
	int valuesCount;
	btree()
	{
		parent = 0;	
		children = 0;
		values   = (value*) calloc(VALUE_COUNT,sizeof(value));
		childrenCount = 0;
		valuesCount = 0;
	}
};
struct tree_index_pair
{
	btree * tree;
	int index;
	int parentChildIndex;
};
tree_index_pair FindMax(btree * t)
{
	tree_index_pair ret = {0};

	while(t->children)
	{
		t = t->children + (t->childrenCount-1);
	}
	ret.index = t->valuesCount-1;
	ret.tree = t ;
	return ret;
}
void PrintBtree(btree*t);
int FindInsertionIndex(value * arr,value v,int count);
int InsertToValueSortedArray(value * arr,int &count,value v);
int InsertToBtreeBucket(btree *t,value a);

void AssertBtreeAllocation(btree* t)
{
	assert(t && t->values);
}

int FindChildrenIndex(btree* tree, value a)
{
	if(tree->valuesCount==0)
		return 0;
	else 
	{
		int startRange= 0, endRange = tree->valuesCount-1;
		while(endRange - startRange > 0)
		{
			if(tree->values[(startRange + endRange)/2] > a)
				endRange = (startRange + endRange)/2;
			else 
				startRange = (startRange + endRange)/2;
		}
		return startRange;
	}
}

value PopMedian(value * arr,int &count)
{
	assert(count > 0);
	value median = arr[count/2];
	memcpy(arr+(count/2),arr+(count/2+1),(count/2-1)*sizeof(value));
	count--;
	return median;
}
void SpliceChildrenAddNewNode(btree* t , int index,btree * oldNode,btree*newNode)
{	
	for(int i = t->childrenCount; i > index ; i--)
	{
		t->children[i] = t->children[i-1];
		for(int j = 0 ; j < t->children[i].childrenCount;j++)
		{
			t->children[i].children[j].parent = &t->children[i];
		}
		t->children[i].parent = t;
	}
	for(int i = 0 ;i< oldNode->childrenCount;i++)
	{
		oldNode->children[i].parent = &t->children[index];
	}
	for(int i = 0 ;i< newNode->childrenCount;i++)
	{
		newNode->children[i].parent = &t->children[index+1];
	}
	t->children[index]   = *oldNode;
	t->children[index+1] = *newNode;

	t->childrenCount++;
}
 btree * SearchLocationForInsertion(btree * tree,value a)
{
	while(tree->children)
	{		
		int index = FindInsertionIndex(tree->values,a,tree->valuesCount);
		assert(index <= tree->childrenCount);
		tree = &tree->children[index];
	}
	return tree;
}
tree_index_pair SearchLocationForDeletion(btree * tree,value a)
{
	tree_index_pair ret = {0};
	while(tree)
	{		
		int index = FindInsertionIndex(tree->values,a,tree->valuesCount);
		if(index < tree->valuesCount && tree->values[index] == a)
		{
			ret.tree = tree;
			ret.index = index; 
			return ret;
		}
		if(tree->children && tree->childrenCount>0)
		{	
			tree = &tree->children[index];
			ret.parentChildIndex = index;
		}
		else
		{
			ret.tree = tree = 0;
			ret.index = -1;
			ret.parentChildIndex = -1;
		}
	}
	return ret;
}
 void InitValues(btree* tree)
 {
	 if(!tree->values)
	 {
		tree->values = (value*)calloc(VALUE_COUNT,sizeof(value));
	 }
 }

 void InitChildren(btree* &tree)
 {
	 if(!tree->children)
	 {
		tree->children = (btree*)calloc(CHILDREN_COUNT,sizeof(btree));
		assert(tree->children);
		for(int i = 0 ; i < CHILDREN_COUNT;i++)
		{
			 tree->children[i] = btree();
			 tree->children[i].parent = tree;
		}
	 }
 }
 void TestBrokenRelationship(btree* tree, int d = 0)
 {
	 if(!tree) return;
	 else
		 for(int i = 0 ; i < tree->childrenCount;i++)
		 {
			 if(tree->children[i].parent!=tree)
			 {
				 printf("error : %d , %d \n",d,i);
				 PrintBtree(&tree->children[i]);
			 }
		 }
 }
 void RemoveFromArray(value * tArr,int index ,int arraySize)
 {
	 assert(index < arraySize && index >= 0);
	 memcpy(tArr+index,tArr+index+1,(arraySize - index -1)*sizeof(value));
 }


btree * BtreeDelete(btree*& tree,value a)
{
	tree_index_pair delLoc = SearchLocationForDeletion(tree,a);
	if(!delLoc.tree)
		return tree;
	if(!delLoc.tree->children)// a leaf
	{
		if(delLoc.tree->valuesCount > (CHILDREN_COUNT-1)/2)
		{
			if(delLoc.index != delLoc.tree->valuesCount-1)
				memcpy(delLoc.tree->values + delLoc.index,delLoc.tree->values + delLoc.index + 1 ,sizeof(value) * (delLoc.tree->valuesCount - delLoc.index - 1));
			 delLoc.tree->valuesCount--;
		}
		else
		{
			value tArr[2 * CHILDREN_COUNT];
			int neighbourIndex = delLoc.parentChildIndex; 
			if(neighbourIndex == delLoc.tree->parent->childrenCount-1)
				neighbourIndex--;
			else if (neighbourIndex == 0)
				neighbourIndex++;
			else
				neighbourIndex--;

			int combinedValueCount = delLoc.tree->valuesCount + delLoc.tree->parent->children[neighbourIndex].valuesCount + 1;
			int parentElementToBorrowIndex =  (delLoc.parentChildIndex + neighbourIndex)/2;
			if(neighbourIndex > delLoc.parentChildIndex) // the neighbour elements are greater then the leaf to delete from;
			{
				memcpy(tArr,
					delLoc.tree->values,
					delLoc.tree->valuesCount*sizeof(value));
				memcpy(tArr + tree->valuesCount,
					   &delLoc.tree->parent->values[delLoc.parentChildIndex],
					   sizeof(value));
				memcpy(tArr + tree->valuesCount +1,
						delLoc.tree->parent->children[delLoc.parentChildIndex+1].values,
						delLoc.tree->parent->children[delLoc.parentChildIndex+1].valuesCount *sizeof(value));
			}
			else
			{
				memcpy(tArr,
						delLoc.tree->parent->children[neighbourIndex].values,
						delLoc.tree->parent->children[neighbourIndex].valuesCount *sizeof(value));
				memcpy(tArr + delLoc.tree->parent->children[neighbourIndex].valuesCount,
						&delLoc.tree->parent->values[parentElementToBorrowIndex],
						sizeof(value));
				memcpy(tArr + delLoc.tree->parent->children[neighbourIndex].valuesCount +1,delLoc.tree->values,delLoc.tree->valuesCount*sizeof(value));
				delLoc.index += delLoc.tree->parent->children[neighbourIndex].valuesCount +1;
			}
			
			if(combinedValueCount > CHILDREN_COUNT - 1)
			{
				memcpy(delLoc.tree->parent->values + delLoc.parentChildIndex + 1 ,tArr + combinedValueCount/2,sizeof(value));
				memcpy(delLoc.tree->values,tArr,(combinedValueCount/2 -1)* sizeof(value));
				memcpy(delLoc.tree->parent->children[delLoc.parentChildIndex].values,tArr + combinedValueCount/2+1,(combinedValueCount/2 -1)* sizeof(value));
			}
			else if(combinedValueCount == CHILDREN_COUNT-1)
			{
				if(delLoc.tree->parent->valuesCount > 1)
				{
					// no underflow
					RemoveFromArray(tArr,delLoc.index,2 * CHILDREN_COUNT);
					delLoc.tree->parent->valuesCount--;
					delLoc.tree->parent->childrenCount--;
					memcpy(delLoc.tree->parent->children[neighbourIndex].values,tArr,sizeof(value)*(combinedValueCount -1));
					delLoc.tree->parent->children[neighbourIndex].valuesCount = combinedValueCount -1;
				}

				else
				{
					RemoveFromArray(tArr,delLoc.index,combinedValueCount);
					delLoc.tree->parent->childrenCount = 0;
					delLoc.tree->parent->valuesCount = combinedValueCount -1;
					memcpy(delLoc.tree->parent->values,tArr,sizeof(value)*(combinedValueCount-1));
					free(delLoc.tree->parent->children);
					delLoc.tree->parent->children = 0;
				}
			}
		}
	}
	else
	{
		int leftChildValueCount = delLoc.tree->children[delLoc.index].valuesCount;
		tree_index_pair maxL = FindMax(&delLoc.tree->children[delLoc.index]);
		value toDelete = maxL.tree->values[maxL.index];
		//replace the max element with the delLoc position
		delLoc.tree->values[delLoc.index] = maxL.tree->values[maxL.index];
		//delete the element from the maxL updated
		return BtreeDelete(maxL.tree,maxL.tree->values[maxL.index]);
	}
	return tree;
}
btree * BtreeInsert(btree* tree, value a)
{	
	if(!tree)
		return 0;
	
	btree * node = SearchLocationForInsertion(tree,a);
	while(node)
	{
		int insertionIndex = InsertToBtreeBucket(node,a);
		if(node->valuesCount <  VALUE_COUNT)
			break;
		
		int medianIndex = (node->valuesCount-1)/2;
		value toInsertToParent = node->values[medianIndex];
		btree *newNode = new btree(); //right node;
		AssertBtreeAllocation(newNode);
		memcpy(newNode->values,&node->values[medianIndex+1],((node->valuesCount) - (medianIndex +1)) * sizeof(value));
		newNode->valuesCount = ((node->valuesCount) - (medianIndex +1));
		int nodeChildrenCount = node->childrenCount;
		TestBrokenRelationship(tree);
		node->valuesCount = medianIndex;
		if(node->childrenCount>0)
			node->childrenCount = medianIndex+1;
		if(node->parent)
			newNode->parent = node->parent;
		else
		{
			node->parent = new btree();
			AssertBtreeAllocation(node->parent);
			newNode->parent = node->parent;
			InitChildren(node->parent);
			node->parent->childrenCount = 1;
			tree = node->parent;
		}
		if(nodeChildrenCount > medianIndex)
		{	
			InitChildren(newNode);
			newNode->childrenCount = nodeChildrenCount - (medianIndex + 1);
			for(int i = 0; i < newNode->childrenCount; i++)
			{
				newNode->children[i]= node->children[i +(medianIndex + 1)];
				for(int j = 0 ;j <newNode->children[i].childrenCount;j++)
					newNode->children[i].children[j].parent = &newNode->children[i];		
			}
		}
		int index = FindInsertionIndex(node->parent->values,toInsertToParent,node->parent->valuesCount);
		SpliceChildrenAddNewNode(node->parent,index,node,newNode);
		a = toInsertToParent;
		node = node->parent;
	}
	return tree;
}
int FindInsertionIndex(value * arr,value v,int count)
{
	if(count==0) return 0;
	int left = 0, 
		right = count-1;
	while (right - left > 1)
	{
		int cmpIndex = (left + right)/2;
		if(arr[cmpIndex] < v)
			left = cmpIndex;
		else 
			right = cmpIndex;
	}
	if(arr[right] < v)
		return right+1;
	else if (arr[left] >= v)
		return left;
	else if ((arr[right] > v && arr[left]  < v )|| 
			  arr[right] == v)
		return right;

	return right;
}
int InsertToValueSortedArray(value * arr,int &count,value v)
{
	if(count == 0)
	{
		arr[0] = v;
		count++;
		return 0; 
	}
	else
	{
		int insertionIndex = FindInsertionIndex(arr,v,count);
		if(insertionIndex == 0)
		{
			memcpy(arr+1,arr,count*sizeof(value));
			arr[0] = v;
		}
		else if(insertionIndex == count)
		{
			arr[count] = v;		
		}
		else 
		{
			memcpy(arr+insertionIndex+1,arr+insertionIndex,(count-insertionIndex)*sizeof(value));
			arr[insertionIndex] = v;
		}

		count++;
		return insertionIndex;
	}
}
int InsertToBtreeBucket(btree *t,value a)
{
	if(!t)
		return -1;
	
	return InsertToValueSortedArray(t->values,t->valuesCount,a);
}
void testArrayInsert()
{
#define TEST_COUNT 100000
	srand(51);
	value * arr = new value[1];
	int count = 0;
	int top = 1;
	for(int i = 0 ; i< TEST_COUNT; i++)
	{
		value v(rand()%TEST_COUNT);

		InsertToValueSortedArray(arr,count,v);
		if(top == count)
		{
			top *=2;
			arr  = (value*) realloc(arr,top * sizeof(value));
		}
	}
	int numbersMissing = 0; 
	for(int i = 1 ; i< TEST_COUNT-1; i++)
	{
		int diff = arr[i].ix - arr[i-1].ix;  
		if(diff > 1)
		{
			numbersMissing += diff;
		}
	}
	printf("%d missing numbers\n",numbersMissing);
}
void PrintBtree(btree* t)
{
	if(t)
	{
		printf("{(");
		for(int i = 0; i<t->valuesCount;i++)		
		{
			if(i < t->valuesCount-1)
				printf("%d,",t->values[i].sx);
			else 
				printf("%d",t->values[i].sx);
		}	
		printf(")");
		for(int j = 0; j<t->childrenCount;j++)
		{
			PrintBtree(&t->children[j]);
		}
		printf("}");
	
	}
}
void BtreeDestroy(btree * t)
{
	if(!t)
		return;
	if(t->children)
	{
		for(int i = 0 ; i < t->childrenCount ; i++)
		{
			BtreeDestroy(&t->children[i]);
		}
		free(t->children);
		t->children = 0;
	}
	free(t->values);
	t->values = 0;
	t->childrenCount = 0;
	t->valuesCount = 0;
}
void testBtree()
{
#define TEST_COUNT 40
	srand(51);
	int count = 0;
	int top = 1;
	btree *t = new btree();
	for(int i = 0 ; i< TEST_COUNT; i++)
	{	
		value v(rand()%6);
		printf("%d \n",v.sx);

		t = BtreeInsert(t,v);
		printf("\n");
		//TestBrokenRelationship(t);
		PrintBtree(t);
	}
	printf("\n");
	PrintBtree(t); printf("\n");
#define DelPrint(v) BtreeDelete(t,(v)); PrintBtree(t);printf("\n");

	DelPrint(4);
	DelPrint(5);
	DelPrint(0);
	//BtreeDelete(t,0);
	//PrintBtree(t);printf("\n");
	BtreeDestroy(t);
}

int main(int argc, char * argv[])
{		
	//allocatorTest();
	//TestRLE();
	//TestLZ77();
	HuffmanTest();
//	testBtree();
	return 0;
}