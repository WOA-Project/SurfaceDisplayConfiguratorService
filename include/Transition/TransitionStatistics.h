#pragma once

#include <atomic>
#include <cstdint>

namespace Transition {

struct StatisticsSnapshot
{
    uint64_t TransitionCount = 0;

    uint64_t CaptureCount = 0;

    uint64_t RebuildCount = 0;

    uint64_t AccessLostCount = 0;

    uint64_t DeviceRemovedCount = 0;

    uint64_t TopologyChangeCount = 0;

    uint64_t TotalCaptureMs = 0;

    uint64_t TotalTransitionMs = 0;

    uint64_t TotalRebuildMs = 0;
};

class TransitionStatistics
{
public:
    static void RecordTransition(uint64_t DurationMs);

    static void RecordCapture(uint64_t DurationMs);

    static void RecordRebuild(uint64_t DurationMs);

    static void RecordAccessLost();

    static void RecordDeviceRemoved();

    static void RecordTopologyChange();

    static StatisticsSnapshot Snapshot();

private:
    static std::atomic<uint64_t> s_TransitionCount;

    static std::atomic<uint64_t> s_CaptureCount;

    static std::atomic<uint64_t> s_RebuildCount;

    static std::atomic<uint64_t> s_AccessLostCount;

    static std::atomic<uint64_t> s_DeviceRemovedCount;

    static std::atomic<uint64_t> s_TopologyChangeCount;

    static std::atomic<uint64_t> s_TotalCaptureMs;

    static std::atomic<uint64_t> s_TotalTransitionMs;

    static std::atomic<uint64_t> s_TotalRebuildMs;
};

} // namespace Transition