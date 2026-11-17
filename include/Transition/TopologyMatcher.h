#pragma once

#include "Transition/TopologySnapshot.h"

namespace Transition {

const DisplayRegion *
FindRegion(const TopologySnapshot &Snapshot, const std::wstring &Id);

}