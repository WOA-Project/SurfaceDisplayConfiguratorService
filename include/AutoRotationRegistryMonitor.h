#pragma once

#include "RegistryMonitor.h"

class AutoRotationRegistryMonitor : public RegistryMonitor
{
public:
    HRESULT Initialize();

protected:
    VOID OnRegistryChanged() override;
};