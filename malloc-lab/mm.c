/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team hyowon",
    /* First member's full name */
    "riceburger",
    /* First member's email address */
    "yummy",
    /* Second member's full name (leave blank if none) */
    "chicken pizza",
    /* Second member's email address (leave blank if none) */
    "let's go"};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
static char* start = NULL;
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
//이게 이게 워드 사이즈? 인듯함.
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

void updateheader(int* header, int size, int isAllocated);
int isallocated(int* header);
void updateheader(int* header, int size, int isAllocated);
int* getnextheader(int* header);
int* getfrontheader(int* header);
void* extendheap(int size);
void* mergeFreeBlock(int* header);
int max(int a, int b);
size_t getsize(int* header);
/*
 * mm_init - initialize the malloc package.
 */
//내 완벽한 계산에 따르면 이게 되야 하는데 ㅋㅋㅋ
int mm_init(void)
{
    //알단 4kb 크기로 만드는게 나을듯??
    //이게 1페이지 크기니까 일단 이렇게 하고, 나중에 늘리는 방향으로 해보자
    //ㄴㄴ 아니다 일단 크기가 정해져 있네
    //헤더도 일단 만들어줘야 할듯함.
    //일단 헤더는 8바이트로 하고, 크기랑 비할당 중이니까 flog 비트는 0으로 하고,
    //이거 저거 해보면 될 거 같음.
    mem_init();
    start = (char*)mem_sbrk(0);
    //8바이트 정렬이 되어 있지 않은 경우
    //payload의 주소가 정렬 단위에 맞춰야 하기 때문에 padding을 항상 넣어줘야 함. ㅇㅋ?
    mem_sbrk(16);
    //이건 패딩인데 한번  빼야하나?
    //프롤로그 헤더
    updateheader((int*)(start + 4), 8, 1);
    //프롤로그 푸터
    updateheader((int*)(start + 8), 8, 1);
    //에필로그 블럭 만들어주기
    updateheader((int*)(start + 12), 0, 1);
    start += 4;
    
    //힙 영역 늘려주기 4kb
    //페이지 단위가 4kb라서 일단 처음엔 저렇게 늘려줌.
    extendheap(1 << 12);
    //에필로그 블럭 크기 제한 거 일단 가용 상태로 만듦
    return 0;
}
//first fit? best-fit?
//뭐로 하는게 좋을까
//일단 first fit
int* find_fit(size_t size)
{
    int* curheader = (int*)start;
    while (1)
    {
        int block_size = getsize(curheader);
        int isallocate = isallocated(curheader);
        if (isallocate == 1)
        {
            if (block_size == 0)
                return NULL;
            curheader = getnextheader(curheader);
            continue;
        }

        if (block_size >= size)
            return curheader;
        else
        {
            curheader = getnextheader(curheader);
            continue;
        }
    }
    return NULL;
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
//이거도 수정이 필요해보임
void *mm_malloc(size_t size)
{;
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
    //     return NULL;
    // else
    // {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }

    //헤더 푸터 크기 + 요청한 크기를 정렬에 맞게 조정하기
    size_t newsize = ALIGN(size + 8);
    int* fit = find_fit(newsize);
    // 공간이 없을 때,
    if (fit == NULL)
    {
        //공간 새로 할당 받기
        fit = extendheap(newsize);
    }
    //요청한 사이즈와 받은 사이즈가 같은면 해당 블럭의 헤더와 푸터를 수정해서, 할당중으로 바꾼다.
    int origin_size = getsize(fit);
    if (origin_size == newsize)
    {
        updateheader(fit, origin_size, 1);
        char* footer = (char*)getnextheader(fit);
        footer -= 4;
        updateheader((int*)footer, origin_size, 1);
        char* ptr = (char*)fit;
        return (void*)(ptr + 4);
    }
    //만약 요청한 사이즈와 사이즈가 같지 않다면
    //요청한 만큼만 공간을 쓰고, 나머지 부분에 새로운 가용 블록을 만들어준다.
    else
    {
        //요청한 만큼만 공간 쓰기
        updateheader(fit, newsize, 1);
        char* footer = (char*)getnextheader(fit);
        footer -= 4;
        updateheader((int*)footer, newsize, 1);
        //남은 부분
        char* h = (char*)getnextheader(fit);
        updateheader((int*)h, origin_size - newsize, 0);
        char* f = (char*)getnextheader((int*)h);
        f -= 4;
        updateheader((int*)f, origin_size - newsize, 0);
        char* ptr = (char*)fit;
        return (void*)(ptr + 4);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    char* header = (char*)ptr;
    header -= 4;
    updateheader((int*)header, getsize((int*)header), 0);
    char* footer = (char*)getnextheader((int*)header);
    footer -= 4;
    updateheader((int*)header, getsize((int*)header), 0);
    //병합
    mergeFreeBlock((int*)header);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
*/
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

int* getheader(void* payload)
{
    char* a = (char*)payload;
    int* header = (int*)(a - 4);
    return header;
}

int* getnextheader(int* header)
{
    char* h = (char*)header;
    int size = getsize(header);
    h += size;
    return (int*)h;
}

int* getfrontheader(int* header)
{
    char* frontheader = (char*)header;
    size_t size = getsize((int*)(frontheader - 4));

    return (int*)(frontheader - size);
}

int isallocated(int* header)
{
    int isallocated = *header & 0x1;
    return isallocated;
}

size_t getsize(int* header)
{
    size_t size = (*header) & ~0x7;
    return size;
}

void updateheader(int* header, int size, int isallocated)
{
    *header = size | isallocated;
}

void* extendheap(int word)
{
    //todo : 일단 mem_sbrk로 힙을 늘린 후, 에필로그 블럭을 뒤로 옮기기
    //원래 있던 에필로그 블럭을 없애줌.
    char* header = mem_sbrk(word);
    if (header == (void*)-1)
    {
        return;
    }
    header -= 4;
    //가용 상태의 블럭을 만들어줌
    updateheader((int*)header, word, 0);
    char* footer = (char*)getnextheader((int*)header);
    updateheader((int*)(footer - 4), word, 0);

    updateheader((int*)footer, 0, 1);
    //todo : 가용 상태의 블럭을 합쳐주기!
    return mergeFreeBlock((int*)header);
}

int max(int a, int b)
{
    if (a > b)
        return b;
    return a;
}

void* mergeFreeBlock(int* header)
{
    int* frontheader = getfrontheader(header);
    int* backheader = getnextheader(header);

    int frontAllocate = isallocated(frontheader);
    int backAllocate = isallocated(backheader);
    //case 1
    //양 쪽 다 가용 블록일 때
    if (backAllocate == 0 && frontAllocate == 0)
    {
        //todo : 양 쪽 전부 연결 해줌
        // 앞 헤더의 값을 세 가용 블럭의 크기의 합으로 바꿔줌
        //푸터도 바꿔줘야 함
        int newsize = getsize(frontheader) + getsize(header) + getsize(backheader);
        updateheader(frontheader, newsize, 0);
        char* footer = (char*)getnextheader(frontheader);
        footer -= 4;
        updateheader((int*)footer, newsize, 0);
        return frontheader;
    }
    //case 2
    //한쪽만 가용 블록일 때
    else if (backAllocate == 1 && frontAllocate == 0)
    {
        int newsize = getsize(frontheader) + getsize(header);
        updateheader(frontheader, newsize, 0);
        char* footer = (char*)getnextheader(frontheader);
        footer -= 4;
        updateheader((int*)footer, newsize, 0);
        return frontheader;
    }

    else if (backAllocate == 0 && frontAllocate == 1)
    {
        int newsize = getsize(header) + getsize(backheader);
        updateheader(header, newsize, 0);
        char* footer = (char*)getnextheader(header);
        footer -= 4;
        updateheader((int*)footer, newsize, 0);
        return backheader;
    }
    return header;
}