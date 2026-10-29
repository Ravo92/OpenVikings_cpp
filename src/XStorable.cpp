#include "XFile.h"
#include "XStorable.h"

namespace NXBasics
{
	// Original function: FUN_004106c2
	bool CStorable::Storable_Save(const char* fileName, bool openOption)
	{
		CFile file(fileName, false);

		if (!file.OpenForWriting(openOption))
		{
			return false;
		}

		Storable_Save(static_cast<void*>(&file));

		return true;
	}
}