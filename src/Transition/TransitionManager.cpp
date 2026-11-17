#include "pch.h"

#include "Transition\\TransitionManager.h"

#include "Transition\\RenderGeometryBuilder.h"
#include "Transition\\TopologyCoordinator.h"
#include "Transition\\TransitionAnimator.h"
#include "Transition\\TransitionStatistics.h"
#include "Transition\\TransitionTrace.h"

namespace Transition {

TransitionManager::TransitionManager() {}

TransitionManager::~TransitionManager()
{
    EndTransition();
}

HRESULT
TransitionManager::BeginTransition(const Request &Request)
{
    if (m_Active)
    {
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    m_Request = Request;

    m_Active = true;

    return S_OK;
}

HRESULT
TransitionManager::CommitTopology(const std::function<HRESULT()> &Callback)
{
    if (!m_Active)
    {
        return E_UNEXPECTED;
    }

    if (!Callback)
    {
        return E_INVALIDARG;
    }

    return Callback();
}

HRESULT
TransitionManager::EndTransition()
{
    m_PanelVisuals.clear();

    m_CapturedDesktop.Clear();

    m_CurrentRenderGeometry.clear();

    m_TargetRenderGeometry.clear();

    m_CompositionHost.Shutdown();

    m_Overlay.Destroy();

    m_Active = false;

    return S_OK;
}

HRESULT
TransitionManager::ExecuteTransition(
    const TopologySnapshot &CurrentSnapshot,
    const TopologySnapshot &TargetSnapshot,
    const std::function<HRESULT()> &TopologyAction)
{
    ScopedTrace trace(TraceEvent::TransitionBegin, TraceEvent::TransitionEnd, L"TransitionManager::ExecuteTransition");

    const ULONGLONG startTick = GetTickCount64();

    auto cleanup = [&]() {
        m_PanelVisuals.clear();

        m_CapturedDesktop.Clear();

        m_CurrentRenderGeometry.clear();

        m_TargetRenderGeometry.clear();

        m_CompositionHost.Shutdown();

        m_Overlay.Destroy();

        m_Active = false;
    };

    HRESULT hr = BeginTransition(Request{});

    if (FAILED(hr))
    {
        cleanup();

        return hr;
    }

    do
    {
        RETURN_IF_FAILED(m_Overlay.Create());

        m_Overlay.Show();

        RETURN_IF_FAILED(m_CompositionHost.Initialize(m_Overlay.GetWindow()));

        RETURN_IF_FAILED(m_CaptureManager.Initialize(m_CompositionHost.GetD3DDevice()));

        RETURN_IF_FAILED(m_CaptureManager.Capture(m_CapturedDesktop));

        RETURN_IF_FAILED(m_RenderGeometryBuilder.Build(CurrentSnapshot, m_CapturedDesktop, m_CurrentRenderGeometry));

        RETURN_IF_FAILED(m_RenderGeometryBuilder.Build(TargetSnapshot, m_CapturedDesktop, m_TargetRenderGeometry));

        m_PanelVisuals.clear();

        for (const auto &panel : m_CapturedDesktop.Panels)
        {
            PanelVisual visual;

            RETURN_IF_FAILED(m_SurfaceFactory.CreatePanelVisual(m_CompositionHost, panel, visual));

            m_PanelVisuals.emplace_back(std::move(visual));
        }

        RETURN_IF_FAILED(m_CompositionHost.Commit());

        Scenario scenario;

        RETURN_IF_FAILED(m_ScenarioBuilder.BuildScenario(m_CurrentRenderGeometry, m_TargetRenderGeometry, scenario));

        TransitionAnimator animator(m_CompositionHost);

        TopologyCoordinator coordinator(m_Planner, animator, m_CaptureManager);

        RETURN_IF_FAILED(coordinator.Execute(scenario, m_PanelVisuals, TopologyAction));
    } while (false);

    const auto durationMs = static_cast<uint64_t>(GetTickCount64() - startTick);

    if (SUCCEEDED(hr))
    {
        TransitionStatistics::RecordTransition(durationMs);
    }

    cleanup();

    return hr;
}

} // namespace Transition