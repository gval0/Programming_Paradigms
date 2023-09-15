#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation){
    // check everything first
    assert(v != NULL);
    assert(elemSize > 0);
    assert(initialAllocation >= 0);

    // keep everything in v
    v->size = elemSize;
    v->logical_size = 0;
    if(initialAllocation == 0)v->alloc_size = 4;
    else v->alloc_size = initialAllocation;
    v->fn = freeFn;
    v->buckets = malloc(v->size * v->alloc_size);
    assert(v->buckets != NULL);
}

void VectorDispose(vector *v){
    for(int i = 0; i < v->logical_size; i++){
        void* curElement = VectorNth(v, i);
        if(v->fn != NULL) v->fn(curElement);
    }
    free(v->buckets);
}

int VectorLength(const vector *v){
    return v->logical_size;
}

void *VectorNth(const vector *v, int position){
    assert(position >= 0);
    assert(position <= v->logical_size - 1);
    void* curElement = (char*)v->buckets + position * v->size;
    return curElement;
}

void VectorReplace(vector *v, const void *elemAddr, int position){
    void* curElement = VectorNth(v, position);
    if(v->fn != NULL) v->fn(curElement);
    memcpy(curElement, elemAddr, v->size);
}

void VectorInsert(vector *v, const void *elemAddr, int position){
    assert(position >= 0);
    assert(position <= v->logical_size);
    
    // see if buckets needs to grow
    if(v->logical_size == v->alloc_size){
        v->alloc_size *= 2;
        v->buckets = realloc(v->buckets, v->alloc_size * v->size);
        assert(v->buckets != NULL);
    }

    void* curElement = (char*)v->buckets + position * v->size;
    if(position != v->logical_size){
        void* relocationAddress = (char*)curElement + v->size;
        memmove(relocationAddress, curElement, (v->logical_size - position)* v->size);
    }
    memcpy(curElement, elemAddr, v->size);
    v->logical_size++;
}

void VectorAppend(vector *v, const void *elemAddr){
    VectorInsert(v, elemAddr, v->logical_size);  
}

void VectorDelete(vector *v, int position){
    assert(position >= 0);
    assert(position <= v->logical_size - 1);
    void* curElement = (char*)v->buckets + v->size * position;
    if(v->fn != NULL) v->fn(curElement);
    if(position < v->logical_size - 1){
        void* toCopy = (char*) curElement + v->size;
        memmove(curElement, toCopy, (v->logical_size - position)* v->size - v->size);
    }
    v->logical_size--;
}

void VectorSort(vector *v, VectorCompareFunction compare){
    assert(compare != NULL);
    qsort(v->buckets, v->logical_size, v->size, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData){
    assert(mapFn != NULL);
    for(int i = 0; i < v->logical_size; i++){
        void* curElement = VectorNth(v, i);
        mapFn(curElement, auxData);
    }
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted){
    assert(startIndex >= 0);
    assert(startIndex <= v->logical_size);
    assert(searchFn != NULL);

    if(!isSorted){
        for(int i = startIndex; i < v->logical_size; i++){
            void* curElement = VectorNth(v, i);
            if(searchFn(key, curElement) == 0) return i;
        }
    } else {
        void* curElement = bsearch(key, ((char*) v->buckets) + v->size * startIndex, v->logical_size - startIndex, v->size, searchFn);
        if(curElement != NULL){
            int res = ((char*)curElement - (char*)v->buckets)/ v->size;
            return res;
        }
    }
    return -1;
}