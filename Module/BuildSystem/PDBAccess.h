#pragma once
#include <CryCore/Platform/CryWindows.h>
#include <dia2.h>
#include <atlbase.h>


namespace Cry
{
	namespace Lpp
	{
		struct SModuleData;

		//Wrapper to keep COM interfaces out of the other headers
		struct SPDBData
		{
			CComPtr<IDiaDataSource> pDataSource;
		};
		using TPDBDataPtr = std::unique_ptr<SPDBData>;

		//Class for interaction with microsoft pdb files.
		class CPDBAccess
		{
		public:
			CPDBAccess() = default;
			~CPDBAccess();

			bool Init();
			TPDBDataPtr LoadPDBData(fs::path pdbPath) const;

			bool GetAndFillCompilandPaths(SModuleData* pModuleData) const;
		private:
			CComPtr<IClassFactory> m_pClassFactory;
		};
	}
}