#include "Marker.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <exception>

int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    int* arr = nullptr;
    CRITICAL_SECTION cs;
    HANDLE hStartEvent = NULL;
    std::vector<MarkerData> markers;
    std::vector<HANDLE> hThreads;

    try {
        InitializeCriticalSection(&cs);

        int arraySize = 0;
        std::cout << "Введите размер массива: ";
        if (!(std::cin >> arraySize) || arraySize <= 0) {
            throw std::invalid_argument("Некорректный размер массива.");
        }

        arr = new int[arraySize]();

        int numMarkers = 0;
        std::cout << "Введите количество потоков marker: ";
        if (!(std::cin >> numMarkers) || numMarkers <= 0) {
            throw std::invalid_argument("Некорректное количество потоков.");
        }

        markers.resize(numMarkers);
        hThreads.resize(numMarkers);

        hStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (hStartEvent == NULL) {
            throw std::runtime_error("Ошибка создания события старта.");
        }

        for (int i = 0; i < numMarkers; ++i) {
            markers[i] = {
                i + 1, arraySize, arr, &cs, hStartEvent,
                CreateEvent(NULL, FALSE, FALSE, NULL),
                CreateEvent(NULL, FALSE, FALSE, NULL),
                CreateEvent(NULL, FALSE, FALSE, NULL),
                true, 0
            };

            if (!markers[i].hBlockEvent || !markers[i].hResumeEvent || !markers[i].hKillEvent) {
                throw std::runtime_error("Ошибка создания событий для потоков marker.");
            }

            hThreads[i] = CreateThread(NULL, 0, MarkerThread, &markers[i], 0, NULL);
            if (hThreads[i] == NULL) {
                throw std::runtime_error("Ошибка создания потока marker.");
            }
        }

        SetEvent(hStartEvent);

        int activeMarkersCount = numMarkers;

        while (activeMarkersCount > 0) {
            std::vector<HANDLE> activeBlockEvents;
            for (const auto& marker : markers) {
                if (marker.active) {
                    activeBlockEvents.push_back(marker.hBlockEvent);
                }
            }

            if (activeBlockEvents.empty()) {
                break;
            }

            WaitForMultipleObjects(static_cast<DWORD>(activeBlockEvents.size()),
                activeBlockEvents.data(), TRUE, INFINITE);

            {
                CriticalSectionGuard lock(&cs);
                std::cout << "\n=== Текущее содержимое массива ==-\n";
                for (int i = 0; i < arraySize; ++i) {
                    std::cout << arr[i] << " ";
                }
                std::cout << "\n----------------------------------\n\n";
            }

            int targetId = 0;
            std::cout << "Введите порядковый номер потока marker для завершения (существующие активные): ";
            while (true) {
                if (!(std::cin >> targetId)) {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                    std::cout << "Ошибка ввода. Повторите попытку: ";
                    continue;
                }

                auto it = std::find_if(markers.begin(), markers.end(), [targetId](const MarkerData& m) {
                    return m.id == targetId && m.active;
                    });

                if (it != markers.end()) {
                    break;
                }
                std::cout << "Поток с таким номером не найден или уже завершен. Введите снова: ";
            }

            auto targetIt = std::find_if(markers.begin(), markers.end(), [targetId](const MarkerData& m) {
                return m.id == targetId;
                });
            int targetIndex = static_cast<int>(std::distance(markers.begin(), targetIt));

            SetEvent(markers[targetIndex].hKillEvent);
            WaitForSingleObject(hThreads[targetIndex], INFINITE);
            markers[targetIndex].active = false;
            activeMarkersCount--;

            {
                CriticalSectionGuard lock(&cs);
                std::cout << "\n=== Содержимое массива после завершения потока " << targetId << " ==-\n";
                for (int i = 0; i < arraySize; ++i) {
                    std::cout << arr[i] << " ";
                }
                std::cout << "\n----------------------------------------------------\n\n";
            }

            for (auto& marker : markers) {
                if (marker.active) {
                    SetEvent(marker.hResumeEvent);
                }
            }
        }

        std::cout << "Все потоки marker завершили работу. Главный поток завершается." << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Исключение: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Произошла неизвестная критическая ошибка." << std::endl;
    }

    if (hStartEvent != NULL) {
        CloseHandle(hStartEvent);
    }
    for (const auto& marker : markers) {
        if (marker.hBlockEvent) CloseHandle(marker.hBlockEvent);
        if (marker.hResumeEvent) CloseHandle(marker.hResumeEvent);
        if (marker.hKillEvent) CloseHandle(marker.hKillEvent);
    }
    for (auto hThread : hThreads) {
        if (hThread != NULL) CloseHandle(hThread);
    }

    DeleteCriticalSection(&cs);
    delete[] arr;

    return 0;
}