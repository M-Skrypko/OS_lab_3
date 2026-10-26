#include "Marker.h"
#include <cstdlib>
#include <iostream>

DWORD WINAPI MarkerThread(LPVOID lpParam) {
    auto* data = reinterpret_cast<MarkerData*>(lpParam);

    try {
        WaitForSingleObject(data->hStartEvent, INFINITE);
        std::srand(data->id);

        while (true) {
            int index = std::rand() % data->arraySize;
            bool marked = false;

            {
                CriticalSectionGuard lock(data->pCs);
                if (data->arr[index] == 0) {
                    LeaveCriticalSection(data->pCs);
                    Sleep(5);
                    EnterCriticalSection(data->pCs);
                    if (data->arr[index] == 0) {
                        data->arr[index] = data->id;
                        data->markedCount++;
                        marked = true;
                    }
                }
            }

            if (marked) {
                Sleep(5);
                continue;
            }

            {
                CriticalSectionGuard lock(data->pCs);
                std::cout << "[Поток marker " << data->id << "] "
                    << "Помечено элементов: " << data->markedCount << ", "
                    << "Невозможно пометить индекс: " << index << std::endl;
            }

            SetEvent(data->hBlockEvent);

            HANDLE waitEvents[] = { data->hResumeEvent, data->hKillEvent };
            DWORD waitResult = WaitForMultipleObjects(2, waitEvents, FALSE, INFINITE);

            if (waitResult == WAIT_OBJECT_0 + 1) {
                CriticalSectionGuard lock(data->pCs);
                for (int i = 0; i < data->arraySize; ++i) {
                    if (data->arr[i] == data->id) {
                        data->arr[i] = 0;
                    }
                }
                break;
            }
            else if (waitResult == WAIT_OBJECT_0) {
                continue;
            }
            else {
                break;
            }
        }
    }
    catch (...) {
    }

    return 0;
}