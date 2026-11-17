#include "pch.h"

#include "Transition\TransitionStatistics.h"

namespace Transition {

std::atomic<uint64_t> TransitionStatistics::s_TransitionCount{};

std::atomic<uint64_t> TransitionStatistics::s_CaptureCount{};

std::atomic<uint64_t> TransitionStatistics::s_RebuildCount{};

std::atomic<uint64_t> TransitionStatistics::s_AccessLostCount{};

std::atomic<uint64_t> TransitionStatistics::s_DeviceRemovedCount{};

std::atomic<uint64_t> TransitionStatistics::s_TopologyChangeCount{};

std::atomic<uint64_t> TransitionStatistics::s_TotalCaptureMs{};

std::atomic<uint64_t> TransitionStatistics::s_TotalTransitionMs{};

std::atomic<uint64_t> TransitionStatistics::s_TotalRebuildMs{};

void
TransitionStatistics::RecordTransition(uint64_t DurationMs)
{
    ++s_TransitionCount;

    s_TotalTransitionMs.fetch_add(DurationMs, std::memory_order_relaxed);
}

void
TransitionStatistics::RecordCapture(uint64_t DurationMs)
{
    ++s_CaptureCount;

    s_TotalCaptureMs.fetch_add(DurationMs, std::memory_order_relaxed);
}

void
TransitionStatistics::RecordRebuild(uint64_t DurationMs)
{
    ++s_RebuildCount;

    s_TotalRebuildMs.fetch_add(DurationMs, std::memory_order_relaxed);
}

void
TransitionStatistics::RecordAccessLost()
{
    ++s_AccessLostCount;
}

void
TransitionStatistics::RecordDeviceRemoved()
{
    ++s_DeviceRemovedCount;
}

void
TransitionStatistics::RecordTopologyChange()
{
    ++s_TopologyChangeCount;
}

StatisticsSnapshot
TransitionStatistics::Snapshot()
{
    StatisticsSnapshot snapshot;

    snapshot.TransitionCount = s_TransitionCount.load(std::memory_order_relaxed);

    snapshot.CaptureCount = s_CaptureCount.load(std::memory_order_relaxed);

    snapshot.RebuildCount = s_RebuildCount.load(std::memory_order_relaxed);

    snapshot.AccessLostCount = s_AccessLostCount.load(std::memory_order_relaxed);

    snapshot.DeviceRemovedCount = s_DeviceRemovedCount.load(std::memory_order_relaxed);

    snapshot.TopologyChangeCount = s_TopologyChangeCount.load(std::memory_order_relaxed);

    snapshot.TotalCaptureMs = s_TotalCaptureMs.load(std::memory_order_relaxed);

    snapshot.TotalTransitionMs = s_TotalTransitionMs.load(std::memory_order_relaxed);

    snapshot.TotalRebuildMs = s_TotalRebuildMs.load(std::memory_order_relaxed);

    return snapshot;
}

} // namespace Transition
