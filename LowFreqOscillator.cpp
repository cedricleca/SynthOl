#include "SynthOl.h"

namespace SynthOl
{

//-----------------------------------------------------
void LFO::Init(Synth * _Synth, LFOData * _Data)
{	
	MemClear((unsigned char *)this, sizeof(LFO));

	m_Data = _Data;
	m_Synth = _Synth;
	SetOscillator((Wave)m_Data->m_WF);
}

//-----------------------------------------------------
void LFO::SetOscillator(Wave _Wave)
{
	Waveform * Wave = m_Synth->GetWaveForm(_Wave);
	if(m_SrcWaveForm != Wave)
	{
		m_SrcWaveForm = Wave;  
		m_Cursor = 0.0f;
		m_Data->m_WF = _Wave;
	}
}

//-----------------------------------------------------
float LFO::GetValue(float _NoteTime, bool _ZeroCentered)
{
	if(_NoteTime > m_Data->m_Delay)
	{
		float val = m_CurVal;

		val *= m_Data->m_Magnitude;
		_NoteTime -= m_Data->m_Delay;

		if(_NoteTime < m_Data->m_Attack)
			val *= _NoteTime / m_Data->m_Attack;

		return val*m_Data->m_BaseValue + (_ZeroCentered ? 0.0f : m_Data->m_BaseValue);
	}

	if(m_Data->m_Delay > 0.0f)
	{
		float val = m_Data->m_BaseValue * (_NoteTime/m_Data->m_Delay);

		if(_ZeroCentered)
			return m_Data->m_BaseValue - val;
		else
			return val;
	}

	return 0.0f;
}

//-----------------------------------------------------
void LFO::Update(float _FrameTime)
{
	long c = int(m_Cursor);
	float val = m_SrcWaveForm->m_Wave[c];
	float val2;

	if(c + 1 < m_SrcWaveForm->m_Size)
		val2 = m_SrcWaveForm->m_Wave[c+1];
	else
		val2 = m_SrcWaveForm->m_Wave[0];

	m_CurVal = (m_Cursor - (float)c) * val2 + (1.0f - m_Cursor + (float)c) * val;

	if(m_CurVal < 0.0f)
		m_CurVal *= -m_CurVal;
	else
		m_CurVal *= m_CurVal;

	m_Cursor += _FrameTime * m_Data->m_Rate * ((float)m_SrcWaveForm->m_Size);
	while(int(m_Cursor) >= m_SrcWaveForm->m_Size)
		m_Cursor -= m_SrcWaveForm->m_Size;
}

//-----------------------------------------------------
void LFO::NoteOn()
{
	if(m_Data->m_NoteSync)
		m_Cursor = 0;
}

};