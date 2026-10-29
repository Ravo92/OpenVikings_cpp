#include "C2E2StaticDataHumanMisc.h"
#include "C2EngineObjectData.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2StringIO.h"
#include "XPalette.h"
#include "XStringArray.h"
#include "XTextEncoding.h"

#include <cstring>

namespace NC2E2
{
	// Original function: FUN_004806c5; SE2DynamicDataHumanData::SetName.
	void DynamicHuman_SetName(void* human, const char* name)
	{
		auto* bytes = static_cast<char*>(human);
		// Remaster keeps the 100-byte name field and zero padding, cutting at UTF-8 boundaries.
		NXBasics::Text_Copy(bytes + NC2Layout::DynamicHumanOffset::Name, 100, name);
	}
}

namespace NC2LogicExternals
{
	// Original function: FUN_00480939; each call owns and releases its mission string array.
	void External_Human_SetName(std::uint32_t human, std::uint32_t name)
	{
		auto* strings = NC2Logic::StringIO_GetMissionStringArrayPtr();

		if (strings == nullptr)
		{

			return;
		}

		const auto* text = strings->GetStringPtr(name);

		if (text != nullptr)
			NC2E2::DynamicHuman_SetName(NC2E2::gDynamicDataHumans.entries + human * NC2Layout::RecordSize::DynamicHumanUnsigned, text);

		delete strings;
	}

	// Original function: FUN_00480975.
	void External_Human_SetRandomPalettePatch(std::uint32_t human, const char* palette)
	{
		const auto patch = NC2E2::StaticHumanMisc_RandomPalette_GetPatchId(palette);

		if (patch == -1)
		{

			return;
		}

		auto* data = NC2E2::gDynamicDataHumans.entries + human * NC2Layout::RecordSize::DynamicHumanUnsigned;
		NC2E2::StaticHumanMisc_RandomPalette_Execute(patch, reinterpret_cast<NXBasics::CPalette*>(data + NC2Layout::DynamicHumanOffset::BodyPalette), reinterpret_cast<NXBasics::CPalette*>(data + NC2Layout::DynamicHumanOffset::HeadPalette), false);
	}
}