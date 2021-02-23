#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <memory.h>

#define PN_HT_INITIAL_SIZE 64


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef _MSC_VER
  #define PN_HT_ALWAYS_INLINE __declspec(always_inline)
#elif defined(__GNUC__)
  #define PN_HT_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
  #define PN_HT_ALWAYS_INLINE
#endif // _MSC_VER

#ifdef PN_HASHTABLE_IMPLEMENTATION
  #define PNHASHTABLE_API extern
#else
  #define PNHASHTABLE_API
#endif // PN_HASHTABLE_IMPLEMENTATION

#ifndef PN_STRCMP
  #define PN_STRCMP(s0, s1, n)   (strcmp((s0), (s1), n) == 0)
#endif

#ifndef PN_STRNCMP
  #define PN_STRNCMP(s0, s1, n)  (strncmp((s0), (s1), n) == 0)
#endif

typedef uint32_t(pfn_Hasher)(const void* Key, size_t kSize);

typedef enum
{
    PN_HT_NONE         = 0x00,
    PN_HT_COPY_KV    	= 0x01,
    PN_HT_NO_RESIZE    = 0x02,
} PN_HT_FLAGS;

typedef struct __sPNBUCKET  PNBUCKET;
typedef struct __sPNBUCKET* PNBUCKETPTR;

typedef struct
{
    pfn_Hasher*   Hasher;
    size_t        Count;
    size_t        Cap;
    PNBUCKETPTR*  pBuckets;
    uint32_t      Flags;
    uint32_t      Collisions;
    double        MLF; /* max load factor */
    double        CLF; /* current load factor */
} PNHASHTABLE,  *PNHASHTABLEPTR,
  PNHASHMAP,    *PNHASHMAPPTR,
  PNDICTIONARY, *PNDICTIONARYPTR;


PNHASHTABLE_API PNHASHTABLE   PnHtCreate(uint32_t Flags, double MLF, pfn_Hasher* Hash, int Cap);
PNHASHTABLE_API void          PnHtDestroy(PNHASHTABLEPTR pTable);
PNHASHTABLE_API void          PnHtInsert(PNHASHTABLEPTR pTable, const void* Key, size_t kSize, const void* Value, size_t vSize);
PNHASHTABLE_API void*         PnHtGet(PNHASHTABLEPTR pTable, const void* Key, size_t kSize);
PNHASHTABLE_API int           PnHtRemove(PNHASHTABLEPTR pTable, const void* Key, size_t kSize);
PNHASHTABLE_API int           PnHtContains(PNHASHTABLEPTR pTable, const void* Key, size_t kSize);
PNHASHTABLE_API void          PnHtResize(PNHASHTABLEPTR pTable, size_t NewSize);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef PN_HASHTABLE_IMPLEMENTATION

static PN_HT_ALWAYS_INLINE inline uint32_t _pn_default_hash_func0(const void* Key, size_t kSize)
{
    static uint32_t Seed = /* 0x485E99EB3ULL; //*/ 0xC70F6907UL;
    const char* data = (const char*)Key;
    size_t Hash = 0;

    size_t k;
    for (k = 0; k < kSize; k++)
        Hash = Hash * Seed + data[k];

    return Hash;
}

static PN_HT_ALWAYS_INLINE inline uint32_t _pn_default_hash_func1(const void* Key, size_t kSize)
{
    /* djb2 string hashing algorithm */
	/* sstp://www.cse.yorku.ca/~oz/hash.ssml */
	uint32_t Hash = 0x1505UL;
	const char* data = (char*)Key;

	size_t k;
	for (k = 0; k < kSize; k++)
		Hash = ((Hash << 5) + Hash) ^ data[k]; // (Hash << 5) + Hash = Hash * 33

	return Hash;
}

/* Hash table pBuckets */
struct __sPNBUCKET
{
    void*       Key;
    void*       Value;
    uint32_t    kSize;
    uint32_t    vSize;
    PNBUCKETPTR pNext;
};

/* HASH TABLE BUCKET FUNCTIONS */
static PNBUCKETPTR _PnBkt_Create(uint32_t Flags, const void* Key, size_t kSize, const void* Value, size_t vSize)
{
    PNBUCKETPTR b = (PNBUCKETPTR)malloc(sizeof(PNBUCKET));
    if (b == NULL)
        return NULL;

    /* Set Key */
    b->kSize = kSize;
	b->vSize = vSize;

    if (Flags & PN_HT_COPY_KV)
    {
		b->Key = malloc(b->kSize);
		b->Value = malloc(b->vSize);

		if (b->Key == NULL || b->Value == NULL)
        {
            free(b);
            return NULL;
        }
        memcpy(b->Key, Key, b->kSize);
        memcpy(b->Value, Value, b->vSize);
	}
    else
	{
		b->Key = (void*)Key;
		b->Value = (void*)Value;
	}

    b->pNext = NULL;
    return b;
}

static void _PnBkt_Destroy(PNBUCKETPTR pBkt, uint32_t Flags)
{
    if (pBkt == NULL) return;

    if (Flags & PN_HT_COPY_KV)
    {
        free(pBkt->Key);
        free(pBkt->Value);
    }

    pBkt->kSize = 0u;
    pBkt->vSize = 0u;
    free(pBkt);
	pBkt = NULL;

    return;
}

static int _PnBkt_KeyCmp(PNBUCKETPTR pBkt0, PNBUCKETPTR pBkt1)
{
    if (pBkt0->kSize != pBkt1->kSize)
        return 0;

    char* Key0 = (char*)pBkt0->Key;
    char* Key1 = (char*)pBkt1->Key;

    return PN_STRNCMP(Key0, Key1, pBkt0->kSize);
}

static void _PnBkt_SetValue(PNBUCKETPTR pBkt, uint32_t Flags, const void* Value, uint32_t vSize)
{
    if (Flags & PN_HT_COPY_KV)
    {
        if (pBkt->Value) free(pBkt->Value);
        pBkt->Value = malloc(pBkt->vSize);

        if (pBkt->Value == NULL)
			return;
        memcpy(pBkt->Value, Value, vSize);
    }
    else
		pBkt->Value = (void*)Value;

    pBkt->vSize = vSize;
    return;
}


/* HASH TABLE FUNCTIONS */
PNHASHTABLE PnHtCreate(uint32_t Flags, double MLF, pfn_Hasher* Hash, int Cap)
{
    PNHASHTABLE Table;
    size_t k;

    Table.Flags = Flags;
    Table.Hasher = Hash != NULL ? Hash : &_pn_default_hash_func1;
    Table.Count = 0;
    Table.Cap = Cap <= 0 ? PN_HT_INITIAL_SIZE : Cap;
    Table.Collisions = 0;
    Table.MLF = MLF;
    Table.CLF = 0.0d;
    Table.pBuckets = (PNBUCKETPTR*)malloc(sizeof(PNBUCKETPTR) * Table.Cap);
    for (k = 0; k < Table.Cap; k++)
        Table.pBuckets[k] = NULL;

    return Table;
}

void PnHtDestroy(PNHASHTABLEPTR pTable)
{
    size_t k;
    for (k = 0; k < pTable->Cap; k++)
    {
        _PnBkt_Destroy(pTable->pBuckets[k], pTable->Flags);
        pTable->pBuckets[k] = NULL;
    }

    free(pTable->pBuckets);
	pTable->pBuckets = NULL;
    pTable->Flags = 0;

    pTable->Hasher = NULL;
    pTable->Count = 0;
    pTable->Collisions = 0;
    pTable->Cap = 0;
    pTable->MLF = 0.0d;
    pTable->CLF = 0.0d;

    return;
}

void PnHtInsert(PNHASHTABLEPTR pTable, const void* Key, size_t kSize, const void* Value, size_t vSize)
{
    PNBUCKETPTR pBucket = _PnBkt_Create(pTable->Flags, Key, kSize, Value, vSize);
    uint32_t Index = pTable->Hasher(Key, kSize) % pTable->Cap;
    PNBUCKETPTR tmp = pTable->pBuckets[Index];
    int KeyIsSame = 0;

    if (tmp == NULL)
    {
        pTable->pBuckets[Index] = pBucket;
        pTable->Count++;
        return;
    }

    while (tmp->pNext != NULL)
    {
        if (_PnBkt_KeyCmp(tmp, pBucket))
        {
            KeyIsSame = 1;
            break;
        }
        else tmp = tmp->pNext;
    }

    if (KeyIsSame)
    {
        _PnBkt_SetValue(tmp, pTable->Flags, pBucket->Value, pBucket->vSize);
        _PnBkt_Destroy(pBucket, pTable->Flags);
    }
    else
    {
        tmp->pNext = pBucket;
        pTable->Collisions += 1;
        pTable->Count++;
        pTable->CLF = (double)pTable->Collisions / (double)pTable->Cap;
    }

    if (pTable->CLF > pTable->MLF)
        printf("DEBUG: CLF: %g, MLF: %g, %u\n", pTable->CLF, pTable->MLF, pTable->Flags & PN_HT_NO_RESIZE);

    if(!(pTable->Flags & PN_HT_NO_RESIZE) && (pTable->CLF > pTable->MLF))
        PnHtResize(pTable, pTable->Cap * 2u);

    return;
}

void* PnHtGet(PNHASHTABLEPTR pTable, const void* Key, size_t kSize)
{
    uint32_t Index = pTable->Hasher(Key, kSize) % pTable->Cap;
    PNBUCKETPTR pBucket = pTable->pBuckets[Index];

    PNBUCKET tmp;
    tmp.Key = (void*)Key;
    tmp.kSize = kSize;

    while (pBucket != NULL)
    {
        if (_PnBkt_KeyCmp(pBucket, &tmp))
            return pBucket->Value;
        else
            pBucket = pBucket->pNext;
    }

    return NULL;
}

int PnHtRemove(PNHASHTABLEPTR pTable, const void* Key, size_t kSize)
{
    uint32_t Index = pTable->Hasher(Key, kSize) % pTable->Cap;
    PNBUCKETPTR pBucket = pTable->pBuckets[Index];
    PNBUCKETPTR pPrev = NULL;

    PNBUCKET tmp;
    tmp.Key = (void*)Key;
    tmp.kSize = kSize;

    while(pBucket != NULL)
    {
        if(_PnBkt_KeyCmp(pBucket, &tmp))
        {
            if(pPrev == NULL)
                pTable->pBuckets[Index] = pBucket->pNext;
            else
                pPrev->pNext = pBucket->pNext;

            pTable->Count--;

            if(pPrev != NULL)
              pTable->Collisions--;

            _PnBkt_Destroy(pBucket, pTable->Flags);
            return 1;
        }
        else
        {
            pPrev = pBucket;
            pBucket = pBucket->pNext;
        }
    }

    return 0;
}

int PnHtContains(PNHASHTABLEPTR pTable, const void* Key, size_t kSize)
{
    uint32_t Index = pTable->Hasher(Key, kSize) % pTable->Cap;
    PNBUCKETPTR pBucket = pTable->pBuckets[Index];

    PNBUCKET tmp;
    tmp.Key = (void*)Key;
    tmp.kSize = kSize;

    while (pBucket != NULL)
    {
        if (_PnBkt_KeyCmp(pBucket, &tmp))
            return 1;
        else
            pBucket = pBucket->pNext;
    }

    return 0;
}

void PnHtResize(PNHASHTABLEPTR pTable, size_t NewSize)
{
    PNHASHTABLE newtable = PnHtCreate(pTable->Flags, pTable->MLF, pTable->Hasher, NewSize);

    size_t k;
    PNBUCKETPTR pBucket = NULL;
    PNBUCKETPTR pNext = NULL;
    for(k = 0; k < pTable->Cap; k++)
    {
        pBucket = pTable->pBuckets[k];
        while(pBucket != NULL)
        {
            pNext = pBucket->pNext;
            PnHtInsert(&newtable, pBucket->Key, pBucket->kSize, pBucket->Value, pBucket->vSize);
            pBucket = pNext;
        }
        pTable->pBuckets[k] = NULL;
    }

    PnHtDestroy(pTable);

    pTable->Flags = newtable.Flags;
    pTable->Hasher = newtable.Hasher;
    pTable->Count = newtable.Count;
    pTable->Cap = newtable.Cap;
    pTable->Collisions = newtable.Collisions;
    pTable->MLF = newtable.MLF;
    pTable->CLF = newtable.CLF;
    pTable->pBuckets = newtable.pBuckets;

    return;
}

#endif // PN_HASHTABLE_IMPLEMENTATION


#endif // _HASHTABLE_H_
