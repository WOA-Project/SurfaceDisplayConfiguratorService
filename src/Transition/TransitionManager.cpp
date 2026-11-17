#include "pch.h"

#include "Transition\TransitionManager.h"

#include "Transition\AnimationClock.h"
#include "Transition\TransitionStatistics.h"
#include "Transition\TransitionTrace.h"
#include "Transition\TopologySnapshot.h"

#include <utility>

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

        m_CompositionHost.Shutdown();

        m_Overlay.Destroy();

        m_Active = false;
    };

    const HRESULT beginHr = BeginTransition(Request{});

    if (FAILED(beginHr))
    {
        cleanup();
        return beginHr;
    }

    HRESULT hr = S_OK;

    do
    {
        hr = m_Overlay.Create();
        if (FAILED(hr))
        {
            break;
        }

        m_Overlay.Show();

        hr = m_CompositionHost.Initialize(m_Overlay.GetWindow());

        if (FAILED(hr))
        {
            break;
        }

        hr = m_CaptureManager.Initialize(m_CompositionHost.GetD3DDevice());

        if (FAILED(hr))
        {
            break;
        }

        hr = m_CaptureManager.Capture(m_CapturedDesktop);

        if (FAILED(hr))
        {
            break;
        }

        m_PanelVisuals.clear();

        for (const auto &panel : m_CapturedDesktop.Panels)
        {
            wchar_t buffer[256];

            swprintf_s(
                buffer,
                L"Captured panel: %ldx%ld texture=%p\n",
                (panel.DesktopRect.right - panel.DesktopRect.left),
                (panel.DesktopRect.bottom - panel.DesktopRect.top),
                panel.Texture.Get());

            wprintf(buffer);

            PanelVisual visual;

            hr = m_SurfaceFactory.CreatePanelVisual(m_CompositionHost, panel, visual);

            if (FAILED(hr))
            {
                break;
            }

            m_PanelVisuals.emplace_back(std::move(visual));
        }

        if (FAILED(hr))
        {
            break;
        }

        hr = m_CompositionHost.Commit();

        if (FAILED(hr))
        {
            break;
        }

        Scenario scenario;

        hr = m_ScenarioBuilder.BuildScenario(CurrentSnapshot, TargetSnapshot, scenario);

        if (FAILED(hr))
        {
            break;
        }

        TransitionAnimator animator(m_CompositionHost);

        TopologyCoordinator coordinator(m_Planner, animator, m_CaptureManager);

        hr = coordinator.Execute(scenario, m_PanelVisuals, TopologyAction);

        if (FAILED(hr))
        {
            break;
        }
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
