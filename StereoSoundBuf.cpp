#include "SynthOl.h"

namespace SynthOl
{

//-----------------------------------------------------
void StereoSoundBuf::Allocate(long _Size)
{
	m_Left = new  float[_Size];
	m_Right = new  float[_Size];
	SoundBuf::Allocate(_Size);
}

//-----------------------------------------------------
void StereoSoundBuf::Clear(long _Index, long _Size)
{
	if(_Index + _Size > m_Size)
	{
		FloatClear(m_Left + _Index, m_Size - _Index);
		FloatClear(m_Left, _Size - (m_Size - _Index));
		FloatClear(m_Right + _Index, m_Size - _Index);
		FloatClear(m_Right, _Size - (m_Size - _Index));
	}
	else
	{
		FloatClear(m_Left + _Index, _Size);
		FloatClear(m_Right + _Index, _Size);
	}
}

};