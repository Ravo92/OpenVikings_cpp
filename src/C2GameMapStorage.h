#pragma once

#include "C2GameMapAccess.h"
#include <cstddef>

// Native runtime map owner; serialized map spans remain fixed-width.
alignas(NC2Logic::SLogicMapStorage) inline std::byte gGameMapStaticStorage[sizeof(NC2Logic::SLogicMapStorage)]{ };
