
#define PN_SERIALIZER_IMPLEMENTATION
#include "pn_serializer.h"
#define PN_HASHTABLE_IMPLEMENTATION
#include "pn_hashtable.h"

static void TestSerializer()
{
    PNSERIALIZER Data = { 0 };
    uint32_t k;

    char* lpstrString = NULL;
    struct tm Now = { .tm_mday=20, .tm_mon=1, .tm_year=121 };
    struct tm* pDate = NULL;
    double pNums[5] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
    double* pOutNums = NULL;

    // Serialize
    PnSerializationBegin(&Data);
    PnWriteBytes(&Data, pNums, sizeof(pNums));
    PnWriteString(&Data, "This Is A String", -1);       // If the string is null-terminated, length can be set to -1
    PnWriteDatetime(&Data, &Now);
    PnWriteInt16(&Data, 1200);
    PnWriteInt32(&Data, 24000);
    PnWriteInt64(&Data, 480000);
    PnWriteFloat32(&Data, 654.321f);
    PnWriteFloat64(&Data, 987.654F);
    PnSerializationEnd(&Data, "out.bin");               // Save the serialized data to binary file

    // Deserialize
    PnDeserializationBegin(&Data, "out.bin");           // Deserialize in the same order we serialized
    PnReadBytes(&Data, (void**)&pOutNums, NULL);
    printf("Deserialized-Bytes:   "); for (k = 0; k < 5; k++) { printf("%g ", pOutNums[k]); } printf("\n");
    PnReadString(&Data, &lpstrString, NULL);            // We can set arg 3 to NULL since the string will be null-terminated
    printf("Deserialized-String:  %s\n", lpstrString);
    PnReadDatetime(&Data, &pDate);
    printf("Deserialized-Date:    %s", asctime(pDate));
    printf("Deserialized-Int16:   %d\n", PnReadInt16(&Data));
    printf("Deserialized-Int32:   %d\n", PnReadInt32(&Data));
    printf("Deserialized-Int64:   %I64d\n", PnReadInt64(&Data));
    printf("Deserialized-Float32: %g\n", PnReadFloat32(&Data));
    printf("Deserialized-Float64: %g\n", PnReadFloat64(&Data));
    void* pMemBlocks[] = { lpstrString, pDate, pOutNums };
    PnDeserializationEnd(&Data, pMemBlocks, 3);         // Or call free yourself (not sure about this)

    printf("\n");
    return;
}

static void TestHashtable()
{
    PNHASHTABLE Table = PnHtCreate(PN_HT_NONE, 0.1F, NULL, 10);

    PnHtInsert(&Table, "Key-0", 5, "This is the Value-0", 19);
    PnHtInsert(&Table, "Key-1", 5, "This is the Value-1", 19);
    PnHtInsert(&Table, "Key-2", 5, "This is the Value-2", 19);
    PnHtInsert(&Table, "Key-3", 5, "This is the Value-3", 19);
    PnHtInsert(&Table, "Key-4", 5, "This is the Value-4", 19);
    PnHtInsert(&Table, "Key-5", 5, "This is the Value-5", 19);

    printf("Key=(Key-1), Value=(%s)\n", (char*)PnHtGet(&Table, "Key-1", 5));
    printf("Key=(Key-3), Value=(%s)\n", (char*)PnHtGet(&Table, "Key-3", 5));
    printf("Key=(Key-5), Value=(%s)\n", (char*)PnHtGet(&Table, "Key-5", 5));

    PnHtDestroy(&Table);

    printf("\n");
    return;
}

int main()
{
    TestSerializer();
    TestHashtable();

    return 0;
}
