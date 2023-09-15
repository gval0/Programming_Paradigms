#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn){
			// do all the asserts
			assert(elemSize > 0);
			assert(numBuckets > 0);
			assert(hashfn != NULL);
			assert(comparefn != NULL);

			//assigm everything
			h->hashfn = hashfn;
			h->comparefn = comparefn;
			h->freefn = freefn;
			h->size = elemSize;
			h->bucketsN = numBuckets;
			h->elementsN = 0;

			h->buckets = (vector**)malloc(sizeof(vector*) * numBuckets);
			assert(h->buckets != NULL);
			
			for(int i = 0; i < numBuckets; i++){
				vector** curElement = h->buckets + i;
				*curElement = malloc(sizeof(vector));
				assert(curElement != NULL);
				VectorNew(*curElement, h->size, h->freefn, 4);
			}
		}

void HashSetDispose(hashset *h){
	
	for(int i = 0; i < h->bucketsN; i++){
		vector** curBucket = h->buckets + i;
		VectorDispose(*curBucket);
        free(*curBucket);
	}
	free(h->buckets);
}

int HashSetCount(const hashset *h){
	return h->elementsN;
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData){
	assert(mapfn != NULL);
	for(int i = 0 ; i < h->bucketsN; i++){
		vector** curBucket = h->buckets + i;
		VectorMap(*curBucket, mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr){
	assert(elemAddr != NULL);
	int bucket = h->hashfn(elemAddr, h->bucketsN);
	assert(bucket >= 0 && bucket <= h->bucketsN - 1);
	
	vector** curBucket = h->buckets + bucket;
	int hasOccured = VectorSearch(*curBucket, elemAddr, h->comparefn, 0, false);

	if(hasOccured != -1){
		VectorReplace(*curBucket, elemAddr, hasOccured);
	} else {
		VectorAppend(*curBucket, elemAddr);
		h->elementsN++;
	}
}

void *HashSetLookup(const hashset *h, const void *elemAddr){
	assert(elemAddr != NULL);
	int bucket = h->hashfn(elemAddr, h->bucketsN);
	assert(bucket >= 0 && bucket <= h->bucketsN - 1);

	vector** curBucket = h->buckets + bucket;
	int hasOccured = VectorSearch(*curBucket, elemAddr, h->comparefn, 0, false);
	if(hasOccured != -1){
		return VectorNth(*curBucket, hasOccured);	
	}
	return NULL; 
}
