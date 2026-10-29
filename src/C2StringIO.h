#pragma once

namespace NXBasics
{
	class CStringArray;
}

namespace NC2Logic
{
	// Original function: FUN_0042bd6b
	NXBasics::CStringArray* StringIO_CreateOutOfIniFile(const char* fileName);
	NXBasics::CStringArray* StringIO_GetMissionStringArrayPtr(const char* mapRoot);
	NXBasics::CStringArray* StringIO_GetMissionStringArrayPtr();
}