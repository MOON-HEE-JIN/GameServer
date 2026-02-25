#pragma once
#include "Stub/StructDef.h"

class CEntity
{
protected:
	int m_nMoveIndex;
	st_Vector m_stPosition;
public:
	int GetMoveIndex() { return m_nMoveIndex; }
	st_Vector GetPosition() { return m_stPosition; }

	void SetMoveIndex(int index) { m_nMoveIndex = index; }
};