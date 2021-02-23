#ifndef _PN_SERIALIZER_H_
#define _PN_SERIALIZER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define PN_SERIALIZER_BUFSIZE        512

/**
 * I have no idea why I like the Windows type-naming convention so much
 * (if you don't like it, you change it)
**/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PN_SERIALIZER_IMPLEMENTATION
  #define PNSERIALIZER_API extern
#else
  #define PNSERIALIZER_API
#endif // PN_SERIALIZER_IMPLEMENTATION


typedef struct __sPNSERIALIZER
{
    char*     pBuffer;   /* Data to be serialized to the file */
    char*     pPos;      /* Current position of the pointer   */
    uint32_t  Size;      /* Total size of "pData" in bytes    */
    uint32_t  _Cap;      /* Total available memory in "pData" */
    FILE*     pFile;     /* Output of ~ & Input of de~        */
} PNSERIALIZER;

typedef PNSERIALIZER*          PNSERIALIZERPTR;
typedef const PNSERIALIZER*    PNSERIALIZERCPTR;


PNSERIALIZER_API void PnSerializationBegin(PNSERIALIZERPTR pData);
PNSERIALIZER_API int  PnSerializationEnd(PNSERIALIZERPTR pData, const char* lpstrFilename);
PNSERIALIZER_API int  PnDeserializationBegin(PNSERIALIZERPTR pData, const char* lpstrFilename);
PNSERIALIZER_API int  PnDeserializationEnd(PNSERIALIZERPTR pData, void** pMemBlocks, int32_t nMemBlocks);

PNSERIALIZER_API void PnWriteBytes(PNSERIALIZERPTR pData, const void* pBytes, int32_t nSize);
PNSERIALIZER_API void PnWriteString(PNSERIALIZERPTR pData, const char* lpstrString, int32_t nLength);
PNSERIALIZER_API void PnWriteDatetime(PNSERIALIZERPTR pData, const struct tm* pDatetime);
PNSERIALIZER_API void PnWriteInt16(PNSERIALIZERPTR pData, int16_t Value);
PNSERIALIZER_API void PnWriteInt32(PNSERIALIZERPTR pData, int32_t Value);
PNSERIALIZER_API void PnWriteInt64(PNSERIALIZERPTR pData, int64_t Value);
PNSERIALIZER_API void PnWriteFloat32(PNSERIALIZERPTR pData, float Value);
PNSERIALIZER_API void PnWriteFloat64(PNSERIALIZERPTR pData, double Value);

PNSERIALIZER_API void    PnReadBytes(PNSERIALIZERPTR pData, void** pBytes, uint32_t* pSize);
PNSERIALIZER_API void    PnReadString(PNSERIALIZERPTR pData, char** lpstrString, int32_t* pLength);
PNSERIALIZER_API void    PnReadDatetime(PNSERIALIZERPTR pData, struct tm** pDatetime);
PNSERIALIZER_API int16_t PnReadInt16(PNSERIALIZERPTR pData);
PNSERIALIZER_API int32_t PnReadInt32(PNSERIALIZERPTR pData);
PNSERIALIZER_API int64_t PnReadInt64(PNSERIALIZERPTR pData);
PNSERIALIZER_API float   PnReadFloat32(PNSERIALIZERPTR pData);
PNSERIALIZER_API double  PnReadFloat64(PNSERIALIZERPTR pData);


#ifdef __cplusplus
}
#endif


#ifdef PN_SERIALIZER_IMPLEMENTATION

void PnSerializationBegin(PNSERIALIZERPTR pData)
{
    pData->pBuffer = malloc(PN_SERIALIZER_BUFSIZE);
    pData->pPos  = pData->pBuffer;
    pData->pFile = NULL;
    pData->Size  = 0UL;
    pData->_Cap  = PN_SERIALIZER_BUFSIZE;

    return;
}

int PnSerializationEnd(PNSERIALIZERPTR pData, const char* lpstrFilename)
{
    int8_t Result = 0;
    if (!pData || !pData->pBuffer) return Result;

    pData->pFile = fopen(lpstrFilename, "wb");

    if (pData->pFile == NULL)
        Result = 0;
    else
    {
        /* First, write the size of the serialized data (useful when deserializing) */
        const uint32_t FullSize = pData->Size + sizeof(uint32_t);
        fwrite(&FullSize, sizeof(uint32_t), 1U, pData->pFile);

        fwrite(pData->pBuffer, pData->Size, sizeof(char), pData->pFile);
        fclose(pData->pFile);
        Result = 1;
    }

    free(pData->pBuffer);
    pData->pBuffer = NULL;
    pData->pFile = NULL;
    pData->Size  = 0UL;
    pData->_Cap  = 0L;

    return Result;
}

int PnDeserializationBegin(PNSERIALIZERPTR pData, const char* lpstrFilename)
{
    int8_t Result = 0;
    if (!pData || !lpstrFilename) return Result;

    pData->pBuffer = NULL;
    pData->pPos  = NULL;
    pData->Size  = 0UL;
    pData->_Cap  = 0UL;
    pData->pFile = fopen(lpstrFilename, "rb");

    if (pData->pFile == NULL)
        Result = 0;
    else
    {
        fread(&pData->Size, sizeof(uint32_t), 1UL, pData->pFile);

        if (pData->Size == 0UL)
            Result = 0;
        else
        {
            pData->_Cap = pData->Size;
            pData->pBuffer = malloc(pData->Size);

            fread(pData->pBuffer, sizeof(char), pData->Size, pData->pFile);
            pData->pPos = pData->pBuffer;
            fclose(pData->pFile);
            Result = 1;
        }
    }

    return Result;
}

// NOTE: Not sure if this passing an array of pointers to delete them is going to be a problem
int PnDeserializationEnd(PNSERIALIZERPTR pData, void** pMemBlocks, int32_t nMemBlocks)
{
    if (!pData || !pData->pBuffer) return 0;

    free(pData->pBuffer);
    pData->pBuffer = NULL;
    pData->pPos = NULL;
    pData->pFile = NULL;
    pData->Size  = 0UL;
    pData->_Cap = 0UL;

    if (pMemBlocks != NULL && nMemBlocks > 0)
    {
        int32_t k;
        for (k = 0; k < nMemBlocks; k++)
            if (pMemBlocks[k] != NULL)
                free(pMemBlocks[k]);
    }

    return 1;
}


// NOTE: To prevent errors and crashes, increase the value of PN_SERIALIZER_BUFSIZE
//       Calling PnWriteBytes with nSize too large, may cause the program to write to
//       unallocated memory
void PnWriteBytes(PNSERIALIZERPTR pData, const void* pBytes, int32_t nSize)
{
    PnWriteInt32(pData, nSize);

    char* _p = (char*)pBytes;
    uint32_t k;

    for (k = 0; k < nSize; k++)
        pData->pPos[k] = _p[k];

    pData->pPos += nSize;
    pData->Size += nSize;

    if (pData->Size >= pData->_Cap)
    {
        pData->_Cap += PN_SERIALIZER_BUFSIZE;
        pData->pBuffer = realloc(pData->pBuffer, pData->_Cap);
    }

    return;
}

void PnWriteString(PNSERIALIZERPTR pData, const char* lpstrString, int32_t nLength)
{
    if (nLength == -1L) nLength = strlen(lpstrString);
    PnWriteInt16(pData, (int16_t)nLength); /* Useful for deserializing */

    pData->pPos = strncpy(pData->pPos, lpstrString, nLength);
    pData->pPos += nLength;
    pData->Size += nLength;

    return;
}

void PnWriteDatetime(PNSERIALIZERPTR pData, const struct tm* pDatetime)
{
    time_t Time = (pDatetime == NULL) ? time(NULL) : mktime((struct tm*)pDatetime);
    /* On 32-bit machines, "time_t" is defined as "int" (a 32-bit/4-byte type) while
       on 64-bit machines, it is defined as "long long" (a 64-bit/8-byte type)
    */
#ifdef _USE_32BIT_TIME_T
    PnWriteInt32(pData, (int32_t)Time);
#else
    PnWriteInt64(pData, (int64_t)Time);
#endif // _USE_32BIT_TIME_T

    return;
}

void PnWriteInt16(PNSERIALIZERPTR pData, int16_t Value)
{
    union { int16_t I16; char I8[2]; } __u;
    __u.I16 = Value;

    pData->pPos[0] = __u.I8[0];
    pData->pPos[1] = __u.I8[1];

    pData->pPos += sizeof(int16_t);
    pData->Size += sizeof(int16_t);

    if (pData->Size >= pData->_Cap)
    {
        pData->_Cap += PN_SERIALIZER_BUFSIZE;
        pData->pBuffer = realloc(pData->pBuffer, pData->_Cap);
    }

    return;
}

void PnWriteInt32(PNSERIALIZERPTR pData, int32_t Value)
{
    union { int32_t I32; char I8[4]; } __u;
    __u.I32 = Value;

    uint32_t k;
    for (k = 0; k < 4; k++)
        pData->pPos[k] = __u.I8[k];

    pData->pPos += sizeof(int32_t);
    pData->Size += sizeof(int32_t);

    if (pData->Size >= pData->_Cap)
    {
        pData->_Cap += PN_SERIALIZER_BUFSIZE;
        pData->pBuffer = realloc(pData->pBuffer, pData->_Cap);
    }

    return;
}

void PnWriteInt64(PNSERIALIZERPTR pData, int64_t Value)
{
    union { int64_t I64; int32_t I32[2]; } __u;
    __u.I64 = Value;

    PnWriteInt32(pData, __u.I32[0]);
    PnWriteInt32(pData, __u.I32[1]);

    return;
}

// TODO: [WriteFloat32(...)] does not call realloc(...) on pData->pBuffer
void PnWriteFloat32(PNSERIALIZERPTR pData, float Value)
{
    static const size_t Float32Size = sizeof(float);

    union
    {
        float F32;
        char FloatBytes[Float32Size];
    } __u;
    __u.F32 = Value;

    uint32_t k;
    for (k = 0; k < Float32Size; k++)
        pData->pPos[k] = __u.FloatBytes[k];

    pData->pPos += Float32Size;
    pData->Size += Float32Size;

    return;
}

// TODO: [WriteFloat64(...)] does not call realloc(...) on pData->pBuffer
void PnWriteFloat64(PNSERIALIZERPTR pData, double Value)
{
    static const size_t Float64Size = sizeof(double);

    union
    {
        double F64;
        char FloatBytes[Float64Size];
    } __u;
    __u.F64 = Value;

    uint32_t k;
    for (k = 0; k < Float64Size; k++)
        pData->pPos[k] = __u.FloatBytes[k];

    pData->pPos += Float64Size;
    pData->Size += Float64Size;

    return;
}


void PnReadBytes(PNSERIALIZERPTR pData, void** pBytes, uint32_t* pSize)
{
    int32_t nSize = PnReadInt32(pData);

    *pBytes = malloc(nSize);
    *pBytes = memcpy(*pBytes, pData->pPos, nSize);
    if (pSize != NULL) *pSize = nSize;

    pData->pPos += nSize;
    return;
}

void PnReadString(PNSERIALIZERPTR pData, char** lpstrString, int32_t* pLength)
{
    int32_t nLength = PnReadInt16(pData);

    *lpstrString = malloc(nLength + 1);
    *lpstrString = strncpy(*lpstrString, pData->pPos, nLength);
    (*lpstrString)[nLength] = 0;
    if (pLength != NULL) *pLength = nLength;

    pData->pPos += nLength;
    return;
}

void PnReadDatetime(PNSERIALIZERPTR pData, struct tm** pDatetime)
{
    time_t Time = 0;
#ifdef _USE_32BIT_TIME_T
    Time = PnReadInt32(pData);
#else
    Time = PnReadInt64(pData);
#endif // _USE_32BIT_TIME_T

    *pDatetime = localtime(&Time);
    return;
}

int16_t PnReadInt16(PNSERIALIZERPTR pData)
{
    union { int16_t I16; char I8[2]; } __u;

    __u.I8[0] = pData->pPos[0];
    __u.I8[1] = pData->pPos[1];
    pData->pPos += sizeof(int16_t);

    return __u.I16;
}

int32_t PnReadInt32(PNSERIALIZERPTR pData)
{
    union { int32_t I32; char I8[4]; } __u;

    __u.I8[0] = pData->pPos[0];
    __u.I8[1] = pData->pPos[1];
    __u.I8[2] = pData->pPos[2];
    __u.I8[3] = pData->pPos[3];
    pData->pPos += sizeof(int32_t);

    return __u.I32;
}

int64_t PnReadInt64(PNSERIALIZERPTR pData)
{
    union { int64_t I64; int32_t I32[2]; } __u;

    __u.I32[0] = PnReadInt32(pData);
    __u.I32[1] = PnReadInt32(pData);

    return __u.I64;
}

float PnReadFloat32(PNSERIALIZERPTR pData)
{
    static const size_t Float32Size = sizeof(float);

    union
    {
        float F32;
        char FloatBytes[Float32Size];
    } __u;

    uint32_t k;
    for (k = 0; k < Float32Size; k++)
        __u.FloatBytes[k] = pData->pPos[k];

    pData->pPos += Float32Size;

    return __u.F32;
}

double PnReadFloat64(PNSERIALIZERPTR pData)
{
    static const size_t Float64Size = sizeof(double);

    union
    {
        double F64;
        char FloatBytes[Float64Size];
    } __u;

    uint32_t k;
    for (k = 0; k < Float64Size; k++)
        __u.FloatBytes[k] = pData->pPos[k];

    pData->pPos += Float64Size;

    return __u.F64;
}

#endif // PN_SERIALIZER_IMPLEMENTATION

#endif // _PN_SERIALIZER_H_
