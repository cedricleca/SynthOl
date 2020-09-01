#include "SynthOl.h"

namespace SynthOl
{

//-----------------------------------------------------
void LFO::Init(Synth * _Synth, LFOData * _Data)
{	
	std::memset(this, 0, sizeof(LFO));

	m_Data = _Data;
	m_Synth = _Synth;
	SetOscillator((WaveType)m_Data->m_WF);
}

//-----------------------------------------------------
void LFO::SetOscillator(WaveType Wave)
{
	const Waveform & Waveform = m_Synth->GetWaveForm(Wave);
	if(m_SrcWaveForm != &Waveform)
	{
		m_SrcWaveForm = &Waveform;  
		m_Cursor = 0.0f;
		m_Data->m_WF = Wave;
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
	unsigned int c = int(m_Cursor);
	float val = m_SrcWaveForm->m_Data[c];
	float val2;

	if(c + 1 < m_SrcWaveForm->m_Data.size())
		val2 = m_SrcWaveForm->m_Data[c+1];
	else
		val2 = m_SrcWaveForm->m_Data[0];

	m_CurVal = (m_Cursor - (float)c) * val2 + (1.0f - m_Cursor + (float)c) * val;

	if(m_CurVal < 0.0f)
		m_CurVal *= -m_CurVal;
	else
		m_CurVal *= m_CurVal;

	m_Cursor += _FrameTime * m_Data->m_Rate * ((float)m_SrcWaveForm->m_Data.size());
	while(unsigned int(m_Cursor) >= m_SrcWaveForm->m_Data.size())
		m_Cursor -= m_SrcWaveForm->m_Data.size();
}

//-----------------------------------------------------
void LFO::NoteOn()
{
	if(m_Data->m_NoteSync)
		m_Cursor = 0;
}

};