//
//  my_malloc.c
//  Lab1: Malloc
//

#include "my_malloc.h"
#include<stdio.h>
#include <stdint.h>
#include <unistd.h>

#define MAGICNUM  2021
#define MINIMUMSIZE 16

MyErrorNo my_errno=MYNOERROR;

FreeListNode Start;

FreeListNode free_list_begin();

uint32_t calculate_size(uint32_t sz);

void* find_chunk(uint32_t size);

void split_chunk(void* A, uint32_t size, uint32_t chunk_size);

void insert_node(FreeListNode L); //insert the node in ascending sequence

void remove_node(FreeListNode L);

int ifAdjacent(void* curr, void* next);

void merge(void* ptr1, void* ptr2);

void *my_malloc(uint32_t size)
{
    //total chunk size including header and padding
    uint32_t needed_size = calculate_size(size);
    void* A = find_chunk(needed_size);
    //if sbrk failed my_errno will be MYENOMEM
    if (my_errno == MYENOMEM)
        return NULL;
//    uint32_t freeChunkSize = *((int*)A - 1);
//    //check size, if bigger than size and padding, split
//    if ((freeChunkSize - MINIMUMSIZE) > chunk_size) {
//        A = split_chunk(A, chunk_size);
//    }
    //bookkeep
//    uint32_t* s = (uint32_t*)A - 2;
//    *s = chunk_size;
//    *(s + 1) = MAGICNUM;

    //put magic number
    uint32_t* magicNumber = (uint32_t*)((char*)A + 4);
    *magicNumber = MAGICNUM;
    
    //return the address after header
    A = (char*)A + 8;
    
    return A;
}
      

void my_free(void *ptr)
{
    if (ptr == NULL) {
        my_errno = MYBADFREEPTR;
        return;
    }
    
    uint32_t* num = (uint32_t*)((char*)ptr - 4);
    
    if (*num != MAGICNUM) {
        my_errno = MYBADFREEPTR;
        return;
    }
    
    FreeListNode freeNode;
    freeNode = (FreeListNode)((char*)ptr -  8);
    freeNode -> size = *num;
    freeNode -> flink = NULL;
    
    insert_node(freeNode);
    coalesce_free_list();
}

int ifAdjacent(void* curr, void* next) {
    uint32_t* size = (uint32_t*)((char*)curr + 4);
    
    if (((char*)curr + *size) == next)
        return 1;
    else
        return 0;
}

void merge(void* ptr1, void* ptr2) {
    uint32_t* size1 = (uint32_t*)((char*)ptr1 + 4);
    uint32_t* size2 = (uint32_t*)((char*)ptr2 + 4);
    uint32_t newSize = *size1 + *size2;

    *size1 = newSize;
    remove_node(ptr2);
    
}


void coalesce_free_list()
{
//    printf("\ncoalesce list entered\n");
    FreeListNode L = free_list_begin();
    while(L -> flink != NULL) {
        if(ifAdjacent(L, L->flink))
            merge(L, L-> flink);
        L = L -> flink;
    }
}

uint32_t calculate_size(uint32_t sz){
    uint32_t size = sz + CHUNKHEADERSIZE;
    while((size % 8) != 0)
        size += 1;
    return size;
}

void* find_chunk(uint32_t size){
    void* target = NULL;
    FreeListNode L = free_list_begin();
    uint32_t chunk_size;
    while(L != NULL){
//        printf("Free List size: %d\n" , L -> size);
        if (size <= L -> size){
            target = L;  // The address
            //delete from freelist
            chunk_size = *((char*)L + 4);
            remove_node(L);
            break;
        }
        L = L -> flink;
    }
    // No avaliable chunk, call sbrk, check sbrk return.
    if (target == NULL) {
        void* temp;
//        printf("no avaliable chunk, sbrk called\n");
        
        if (size <= 8192) {
            temp = sbrk(8192);
            chunk_size = 8192;
        } else {
            temp = sbrk(size);
            chunk_size = size;
        }
        //if sbrk failed
        if (!temp) {
            my_errno = MYENOMEM;
            return temp;
        }
//        int* size = (int*)temp;
//        FreeListNode newHeap;
//        newHeap = (FreeListNode)temp;
//        newHeap -> size = heapSize
//        newHeap -> flink = NULL:
//        insert_node(newHeap);
        target = temp;
    }
    
    uint32_t* finalSize = (uint32_t*)target;
    *finalSize = chunk_size; //book keeping
    // if chuck is too large, split the chunk
    if ((chunk_size - MINIMUMSIZE) > size)
        split_chunk(target, size, chunk_size);
    
    
    return target;
}

void insert_node(FreeListNode N){
//    printf("\ninsert node begin\n");
    FreeListNode L = free_list_begin();
    if (L == NULL) {               //if N is the first element
        Start = N;
        return;
    }
    
    if (L -> flink == NULL){      //only have one element
        if (L > N) {
            Start = N;
            N -> flink = L;
        } else {
            L -> flink = N;
        }
        return;
    }
    if (L > N) {
        Start = N;
        N -> flink = L;
        return;
    }
    
    while (L -> flink != NULL) {
        if (L -> flink > N) {
            N -> flink = L -> flink;
            L -> flink = N;
            return;
        }
        L = L -> flink;
    }
    // reached the end
    L -> flink = N;
//    printf("insert node end\n");
}

void remove_node(FreeListNode N){
    FreeListNode L = free_list_begin();
    if (L == N) { // N is the first one
        Start = L -> flink;
        return;
    }
    while (L -> flink != NULL) {
        if (L -> flink == N) {
            L -> flink = L -> flink -> flink;
            return;
        }
    }
//    fprintf(stderr, "remove_node failed\n");
}

void split_chunk(void* A, uint32_t size, uint32_t chunk_size){
    //split the chunk and put remainder into freelist.
    
    FreeListNode remainderNode;
    
    uint32_t remainderSize = chunk_size - size;
    
    uint32_t* finalSize = (uint32_t*)A;  //bookkeeping
    *finalSize = size;
    
    char* tmp = (char*)A;
    tmp += size;        // begin of remainder.
    //embed node to space
    remainderNode = (FreeListNode)tmp;
    remainderNode -> size = remainderSize;
    remainderNode -> flink = NULL;
    
    
    //put node into the free List
    insert_node(remainderNode);

}

FreeListNode free_list_begin()
{
    return Start;
}
