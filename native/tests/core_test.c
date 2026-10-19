#include <sysdolphin/baselib/util.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/random.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
_Static_assert(sizeof(u32) == 4, "Game scalars must remain 32-bit");
_Static_assert(sizeof(bool) == 4, "CodeWarrior booleans are 32-bit");
_Static_assert(sizeof(uintptr_t) == sizeof(void*), "Heap addresses must retain every pointer bit");

/* Test dependencies: this test supplies an explicit arena and must never use
 * the game's general heap. A call into that path is a test failure. */
void* HSD_MemAlloc(u32 size) { (void)size; abort(); }
int HSD_GetHeap(void) { abort(); }
s32 OSCheckHeap(int heap) { (void)heap; abort(); }
void __assert(char* file, u32 line, char* condition) {
    fprintf(stderr, "%s:%u: %s\n", file, line, condition);
    abort();
}

int main(void) {
    CHECK(HSD_GetNbBits(0)==0);
    CHECK(HSD_GetNbBits(0x80000000U)==1);
    CHECK(HSD_GetNbBits(0xFFFFFFFFU)==32);
#if UINTPTR_MAX > UINT32_MAX
    CHECK(OSRoundUp32B((uintptr_t)0x100000001ULL) == 0x100000020ULL);
    CHECK(OSRoundDown32B((uintptr_t)0x10000001FULL) == 0x100000000ULL);
#else
    CHECK(OSRoundUp32B((uintptr_t)0x10000001UL) == 0x10000020UL);
    CHECK(OSRoundDown32B((uintptr_t)0x1000001FUL) == 0x10000000UL);
#endif
    const u32 expected[] = {41, 51235, 6334, 59268, 51937, 15724};
    *seed_ptr = 1;
    for (unsigned i = 0; i < sizeof(expected)/sizeof(*expected); ++i)
        CHECK((u32)HSD_Rand() == expected[i]);
    *seed_ptr = 1;
    CHECK(HSD_Randf() == 41.0f / 65536.0f);

    unsigned char* arena = malloc(256);
    CHECK(arena != NULL);
#if UINTPTR_MAX > UINT32_MAX
    CHECK((uintptr_t)arena > UINT32_MAX);
#endif
    HSD_ObjAllocData pool = {0};
    HSD_ObjSetHeap(255, arena + 1);
    HSD_ObjAllocInit(&pool, 17, 16);
    CHECK(pool.size == 32);
    CHECK(HSD_ObjAllocAddFree(&pool, 20) == 7);
    void* objects[7];
    for (int i = 0; i < 7; ++i) {
        objects[i] = HSD_ObjAlloc(&pool);
        CHECK(objects[i] != NULL);
        CHECK(((uintptr_t)objects[i] & 15) == 0);
        CHECK((unsigned char*)objects[i] >= arena + 1);
        CHECK((unsigned char*)objects[i] + 32 <= arena + 256);
        for (int j = 0; j < i; ++j) CHECK(objects[i] != objects[j]);
    }
    CHECK(HSD_ObjAlloc(&pool) == NULL);
    CHECK(pool.used == 7 && pool.free == 0 && pool.peak == 7);
    HSD_ObjFree(&pool, objects[3]);
    CHECK(HSD_ObjAlloc(&pool) == objects[3]);
    HSD_ObjFree(&pool, objects[3]);
    HSD_ObjAllocSetNumLimit(&pool, 6);
    HSD_ObjAllocEnableNumLimit(&pool);
    CHECK(HSD_ObjAlloc(&pool) == NULL);
    HSD_ObjAllocDisableNumLimit(&pool);
    CHECK(HSD_ObjAlloc(&pool) == objects[3]);
    /* Actual game callers request 12-byte Vec storage with 4-byte alignment.
     * Every free-list slot still needs an aligned host pointer. */
    enum { PTR_ALIGN = _Alignof(void*) };
    HSD_ObjAllocData vectors = {0};
    HSD_ObjSetHeap(255, arena + 1);
    HSD_ObjAllocInit(&vectors, 12, 4);
    CHECK(vectors.size == ((12 + PTR_ALIGN - 1) & ~(PTR_ALIGN - 1)) && vectors.align == PTR_ALIGN - 1);
    CHECK(HSD_ObjAllocAddFree(&vectors, 8) == 8);
    for (int i = 0; i < 8; ++i) {
        void* vector = HSD_ObjAlloc(&vectors);
        CHECK(((uintptr_t)vector & (PTR_ALIGN - 1)) == 0);
    }
    HSD_ObjSetHeap(255, arena + 1);
    HSD_IDInitAllocData(); HSD_IDSetup();
    CHECK(HSD_ObjAllocAddFree(HSD_IDGetAllocData(), 4) == 4);
    /* Equal low 32 bits must remain distinct, including updates and removal. */
#if UINTPTR_MAX > UINT32_MAX
    uintptr_t key1 = 0x100000001ULL, key2 = 0x200000001ULL;
#else
    uintptr_t key1 = 0x00010001UL, key2 = 0x00020001UL;
#endif
    int a=1, b=2, c=3, found;
    HSD_IDInsertToTable(NULL,key1,&a); HSD_IDInsertToTable(NULL,key2,&b);
    CHECK(HSD_IDGetData(key1,&found)==&a && found==1);
    CHECK(HSD_IDGetData(key2,&found)==&b && found==1);
    HSD_IDInsertToTable(NULL,key1,&c);
    CHECK(HSD_IDGetData(key1,NULL)==&c);
    HSD_IDRemoveByIDFromTable(NULL,key1);
    CHECK(HSD_IDGetData(key1,&found)==NULL && found==0);
    CHECK(HSD_IDGetData(key2,NULL)==&b);
    HSD_IDRemoveByIDFromTable(NULL,key2);
    free(arena);
    puts("PASS: native ABI, GameCube random sequence, 64-bit allocator exhaustion/reuse/limits");
    return 0;
}
