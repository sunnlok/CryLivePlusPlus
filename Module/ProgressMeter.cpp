#include "StdAfx.h"
#include "ProgressMeter.h"
#include "Live++.h"


using namespace Cry::Lpp;

CProgressMeter::CProgressMeter()
{
	if (auto pLPP = CLivePlusPlus::Get())
		pLPP->RegisterCompileListener(this);
}

CProgressMeter::~CProgressMeter()
{
	if (auto pLPP = CLivePlusPlus::Get())
		pLPP->RemoveCompileListener(this);
}

void CProgressMeter::OnCompileStarted()
{
	m_compileState = ECompileState::Compiling;
}

void CProgressMeter::OnCompileSuccess()
{
	m_compileState = ECompileState::Success;
}

void CProgressMeter::OnCompileError()
{
	m_compileState = ECompileState::Error;
}

void CProgressMeter::Update()
{
	if (m_compileState == ECompileState::None)
		return;

	



}