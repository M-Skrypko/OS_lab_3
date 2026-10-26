#include "pch.h"
#include "Marker.h"
#include <vector>
#include <cstdlib>

TEST(Lab3Tests, ArrayInitializationTest) {
    int arraySize = 10;
    int* arr = new int[arraySize]();
    for (int i = 0; i < arraySize; ++i) {
        EXPECT_EQ(arr[i], 0);
    }
    delete[] arr;
}

TEST(Lab3Tests, MarkerIndexModuloTest) {
    int arraySize = 5;
    int id = 1;
    std::srand(id);
    for (int i = 0; i < 100; ++i) {
        int randVal = std::rand();
        int index = randVal % arraySize;
        EXPECT_GE(index, 0);
        EXPECT_LT(index, arraySize);
    }
}

TEST(Lab3Tests, MarkerThreadLifecycleTest) {
    const int arraySize = 5;
    int* arr = new int[arraySize]();
    CRITICAL_SECTION cs;
    InitializeCriticalSection(&cs);

    HANDLE hStartEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    HANDLE hBlockEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    HANDLE hResumeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    HANDLE hKillEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    MarkerData data{
        1, arraySize, arr, &cs,
        hStartEvent, hBlockEvent, hResumeEvent, hKillEvent,
        true, 0
    };

    HANDLE hThread = CreateThread(NULL, 0, MarkerThread, &data, 0, NULL);

    Sleep(50);

    SetEvent(hKillEvent);
    DWORD waitResult = WaitForSingleObject(hThread, 1000);
    EXPECT_EQ(waitResult, WAIT_OBJECT_0);

    CloseHandle(hThread);
    CloseHandle(hStartEvent);
    CloseHandle(hBlockEvent);
    CloseHandle(hResumeEvent);
    CloseHandle(hKillEvent);
    DeleteCriticalSection(&cs);
    delete[] arr;
}