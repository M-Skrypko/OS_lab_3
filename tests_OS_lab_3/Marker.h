#ifndef MARKER_H
#define MARKER_H

#include <windows.h>

struct MarkerData {
    int id;
    int arraySize;
    int* arr;
    CRITICAL_SECTION* pCs;
    HANDLE hStartEvent;
    HANDLE hBlockEvent;
    HANDLE hResumeEvent;
    HANDLE hKillEvent;
    bool active;
    int markedCount;
};

class CriticalSectionGuard {
    CRITICAL_SECTION* cs_;
public:
    explicit CriticalSectionGuard(CRITICAL_SECTION* cs) : cs_(cs) {
        EnterCriticalSection(cs_);
    }
    ~CriticalSectionGuard() {
        LeaveCriticalSection(cs_);
    }
    CriticalSectionGuard(const CriticalSectionGuard&) = delete;
    CriticalSectionGuard& operator=(const CriticalSectionGuard&) = delete;
};

DWORD WINAPI MarkerThread(LPVOID lpParam);

#endif