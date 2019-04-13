#pragma once
#include "../Interface/ILive++.h"

namespace Cry
{
	namespace Lpp
	{

		class CProgressMeter : public ICompileListener
		{
			enum class ECompileState
			{
				None,
				Compiling,
				Success,
				Error,
			};

		public:
			CProgressMeter();
			~CProgressMeter();

			virtual void OnCompileStarted() override;
			virtual void OnCompileSuccess() override;
			virtual void OnCompileError() override;

			void Update();
		private:
			ECompileState m_compileState = ECompileState::None;
		};
	}
}