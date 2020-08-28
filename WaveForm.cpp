#include "SynthOl.h"

namespace SynthOl
{

//-----------------------------------------------------
void Waveform::Allocate(long _Size)
{
	m_Wave = new  float[_Size];
	SoundBuf::Allocate(_Size);
}

//-----------------------------------------------------
void Waveform::Clear(long _Index, long _Size)
{
	if(_Index + _Size > m_Size)
	{
		FloatClear(m_Wave + _Index, m_Size - _Index);
		FloatClear(m_Wave, _Size - (m_Size - _Index));
	}
	else
		FloatClear(m_Wave + _Index, _Size);
}

//-----------------------------------------------------
void Waveform::Copy(Waveform * _Src, long _SrcIndex, long _DstIndex, long _Size)
{
	for(long i = 0; i < _Size; i++)
		m_Wave[_DstIndex++] = _Src->m_Wave[_SrcIndex++];
}

//-----------------------------------------------------
void Waveform::GenerateWave(Wave _Type, long _Index, long _Period, long _Size, float _Magnitude)
{
	long cursor = 0;
	long halfPeriod = _Period / 2;
	long QuatPeriod = _Period / 4;
	long period16 = _Period / 16;
	
	float g_fScale = 2.0f / 0xffffffff;
	int g_x1 = 0x67452301;
	int g_x2 = 0xefcdab89;

	if(_Type == Wave::Rand)
		_Magnitude *= g_fScale;

	for(long i = 0; i < _Size; i++, cursor++, _Index++)
	{
		switch(_Type)
		{
		case Wave::Square:
		case Wave::Square_Soft:
			m_Wave[_Index] = (cursor > QuatPeriod && cursor < QuatPeriod+halfPeriod) ? _Magnitude : -_Magnitude;
			break;

		case Wave::Saw:
		case Wave::Saw_Soft:
			{
				float c = (float)cursor;
				float d = (float)halfPeriod;

				if(cursor < halfPeriod)
					m_Wave[_Index] = ((2.0f * c/d) - 1.0f) * _Magnitude;
				else
					m_Wave[_Index] = (1.0f - 2.0f * ((c - d)/d)) * _Magnitude;
			}
			break;

		case Wave::RampUp:
		case Wave::RampUp_Soft:
			{
				float c = (float)cursor;
				float d = (float)halfPeriod;

				m_Wave[_Index] = ((c/d) - 1.0f) * _Magnitude;
			}
			break;

/*
		case Wave_Pulse:
			if(_Index < _Size/10)
				m_Wave[_Index] = _Magnitude;
			else if(_Index >= halfPeriod && _Index < halfPeriod+_Size/10)
				m_Wave[_Index] = -_Magnitude;
			else
				m_Wave[_Index] = m_Wave[_Index-1] * 0.999f;
			break;
*/

		case Wave::Rand:
			g_x1 ^= g_x2;
			m_Wave[_Index] = g_x2 * _Magnitude;
			g_x2 += g_x1;
			break;
		}

		if(cursor > _Period)
			cursor = -1;
	}
}

//-----------------------------------------------------
void Waveform::Normalize(long _Index, long _Size, float _Coef)
{
	float max = 0.0f;

	for(int i = 0; i < _Size; i++, _Index++)
	{
		if(m_Wave[_Index] > max) max = m_Wave[_Index];
		if(-m_Wave[_Index] > max) max = -m_Wave[_Index];
	}

	*this *= 1.0f / max;
}

//-----------------------------------------------------
void Waveform::operator *= (float _Coef)
{
	for(int i = 0; i < m_Size; i++)
		m_Wave[i] *= _Coef;
}

//-----------------------------------------------------
void Waveform::Soften(long _Index, long _Size, float _Coef)
{	
	long i = 0;

	for(i = 0; i < _Size; i++, _Index++)
	{
		if(_Index > 0)
			m_Wave[_Index] = m_Wave[_Index-1] + (m_Wave[_Index] - m_Wave[_Index-1]) * _Coef;
		else
			m_Wave[_Index] = m_Wave[m_Size-1] + (m_Wave[_Index] - m_Wave[m_Size-1]) * _Coef;
	}
}

};