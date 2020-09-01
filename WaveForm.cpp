#include "SynthOl.h"

namespace SynthOl
{
	void Waveform::WaveformSquare(unsigned int Size, float Magnitude, bool Soft)
	{
		const unsigned int halfPeriod = Size / 2;
		const unsigned int QuatPeriod = Size / 4;
	
		for(unsigned int i = 0; i < Size; i++)
			m_Data[i] = (i > QuatPeriod && i < QuatPeriod+halfPeriod) ? Magnitude : -Magnitude;

		if(Soft)
		{
			Soften(0, PlaybackFreq, 0.001f);
			Normalize(0, PlaybackFreq, 1.0f);
		}
	}

	void Waveform::WaveformSaw(unsigned int Size, float Magnitude, bool Soft)
	{
		const float halfPeriod = float(Size) / 2.0f;
	
		float c = 0.0f;
		for(unsigned int i = 0; i < Size; i++, c += 1.0f)
		{
			if(i < Size / 2)
				m_Data[i] = ((2.0f * c / halfPeriod) - 1.0f) * Magnitude;
			else
				m_Data[i] = (1.0f - 2.0f * ((c - halfPeriod) / halfPeriod)) * Magnitude;
		}

		if(Soft)
		{
			Soften(0, PlaybackFreq, 0.001f);
			Normalize(0, PlaybackFreq, 1.0f);
		}
	}

	void Waveform::WaveformRamp(unsigned int Size, float Magnitude, bool Soft)
	{
		const float halfPeriod = float(Size) / 2.0f;
	
		float c = 0.0f;
		for(unsigned int i = 0; i < Size; i++, c += 1.0f)
			m_Data[i] = ((c / halfPeriod) - 1.0f) * Magnitude;

		if(Soft)
		{
			Soften(0, PlaybackFreq, 0.001f);
			Normalize(0, PlaybackFreq, 1.0f);
		}
	}

	void Waveform::WaveformRand(unsigned int Size, float Magnitude, bool Soft)
	{
		int g_x1 = 0x67452301;
		int g_x2 = 0xefcdab89;
		Magnitude *= 2.0f / 0xffffffff;

		for(unsigned int i = 0; i < Size; i++)
		{
			g_x1 ^= g_x2;
			m_Data[i] = g_x2 * Magnitude;
			g_x2 += g_x1;
		}
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