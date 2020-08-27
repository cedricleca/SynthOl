#include "SynthOl.h"

//-----------------------------------------------------
void SO_LowFreqOscillator::Init(SynthOl * _Synth, SO_LowFreqOscillatorData * _Data)
{	
	MemClear((unsigned char *)this, sizeof(SO_LowFreqOscillator));

	m_Data = _Data;
	m_Synth = _Synth;
	SetOscillator((SO_WaveForms)m_Data->m_WF);
}

//-----------------------------------------------------
void SO_LowFreqOscillator::SetOscillator(SO_WaveForms _Wave)
{
	SO_Waveform * Wave = m_Synth->GetWaveForm(_Wave);
	if(m_SrcWaveForm != Wave)
	{
		m_SrcWaveForm = Wave;  
		m_Cursor = 0.0f;
		m_Data->m_WF = _Wave;
	}
}

//-----------------------------------------------------
float SO_LowFreqOscillator::GetValue(float _NoteTime, bool _ZeroCentered)
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
void SO_LowFreqOscillator::Update(float _FrameTime)
{
	long c = float2int(m_Cursor);
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
	while(float2int(m_Cursor) >= m_SrcWaveForm->m_Size)
		m_Cursor -= m_SrcWaveForm->m_Size;
}

//-----------------------------------------------------
void SO_LowFreqOscillator::NoteOn()
{
	if(m_Data->m_NoteSync)
		m_Cursor = 0;
}
