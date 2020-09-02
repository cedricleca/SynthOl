#include "SynthOl.h"

namespace SynthOl
{
	Waveform WaveformSquare(bool Soft)
	{
		Waveform WF;
		const unsigned int Size = static_cast<unsigned int>(WF.m_Data.size());
		const unsigned int halfPeriod = Size / 2;
		const unsigned int QuatPeriod = Size / 4;
		for(unsigned int i = 0; i < Size; i++)
			WF.m_Data[i] = (i > QuatPeriod && i < QuatPeriod+halfPeriod) ? 1.0f : -1.0f;

		if(Soft)
		{
			WF.Soften(0, Size, 0.001f);
			WF.Normalize(0, Size, 1.0f);
		}

		return WF;
	}

	Waveform WaveformSaw(bool Soft)
	{
		Waveform WF;
		const unsigned int Size = static_cast<unsigned int>(WF.m_Data.size());
		const float halfPeriod = float(Size) / 2.0f;
		float c = 0.0f;
		for(unsigned int i = 0; i < Size; i++, c += 1.0f)
		{
			if(i < Size / 2)
				WF.m_Data[i] = ((2.0f * c / halfPeriod) - 1.0f) * 1.0f;
			else
				WF.m_Data[i] = (1.0f - 2.0f * ((c - halfPeriod) / halfPeriod)) * 1.0f;
		}

		if(Soft)
		{
			WF.Soften(0, Size, 0.001f);
			WF.Normalize(0, Size, 1.0f);
		}
	
		return WF;
	}

	Waveform WaveformRamp(bool Soft)
	{
	
		Waveform WF;
		const unsigned int Size = static_cast<unsigned int>(WF.m_Data.size());
		const float halfPeriod = float(Size) / 2.0f;
		float c = 0.0f;
		for(unsigned int i = 0; i < Size; i++, c += 1.0f)
			WF.m_Data[i] = ((c / halfPeriod) - 1.0f) * 1.0f;

		if(Soft)
		{
			WF.Soften(0, Size, 0.001f);
			WF.Normalize(0, Size, 1.0f);
		}
		return WF;
	}

	Waveform WaveformRand(bool Soft)
	{
		int g_x1 = 0x67452301;
		int g_x2 = 0xefcdab89;

		Waveform WF;
		for(unsigned int i = 0; i < WF.m_Data.size(); i++)
		{
			g_x1 ^= g_x2;
			WF.m_Data[i] = float(g_x2);
			g_x2 += g_x1;
		}
		return WF;
	}

//-----------------------------------------------------
void Waveform::Normalize(long _Index, long _Size, float _Coef)
{
	float max = 0.0f;

	for(int i = 0; i < _Size; i++, _Index++)
	{
		if(m_Data[_Index] > max) max = m_Data[_Index];
		if(-m_Data[_Index] > max) max = -m_Data[_Index];
	}

	*this *= 1.0f / max;
}

//-----------------------------------------------------
void Waveform::operator *= (float _Coef)
{
	for(unsigned int i = 0; i < m_Data.size(); i++)
		m_Data[i] *= _Coef;
}

//-----------------------------------------------------
void Waveform::Soften(long _Index, long _Size, float _Coef)
{	
	long i = 0;

	for(i = 0; i < _Size; i++, _Index++)
	{
		if(_Index > 0)
			m_Data[_Index] = m_Data[_Index-1] + (m_Data[_Index] - m_Data[_Index-1]) * _Coef;
		else
			m_Data[_Index] = m_Data[m_Data.size()-1] + (m_Data[_Index] - m_Data[m_Data.size()-1]) * _Coef;
	}
}

};