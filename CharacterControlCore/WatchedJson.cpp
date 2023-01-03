#include "PCH.h"
#include "WatchedJson.h"

bool CharacterControlCore::WatchedJson::UpdateInternal()
{
	try
	{
		std::ifstream characterControlMasterFileStream( this->GetPath() );
		characterControlMasterFileStream >> *GetMutable();
		characterControlMasterFileStream.close();
		return true;
	}
	catch ( ... )
	{
	}

	return false;
}
