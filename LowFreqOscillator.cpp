#include "SynthOl.h"

#include <cmath>

namespace SynthOl
{

//-----------------------------------------------------
void LFOTransients::Init(LFOData * Data, const Synth & Synth)
{	
	m_Data = Data;
	SetOscillator(m_Data->m_WF, Synth);
}

//-----------------------------------------------------
void LFOTransients::SetOscillator(WaveType Wave, const Synth & Synth)
{
	const Waveform & Waveform = Synth.GetWaveForm(Wave);
	if(m_SrcWaveForm != &Waveform)
	{
		m_SrcWaveForm = &Waveform;  
		m_Cursor = 0.0f;
		m_Data->m_WF = Wave;
	}
}

//-----------------------------------------------------
float LFOTransients::GetValue(float NoteTime, bool ZeroCentered)
{
	if(NoteTime > m_Data->m_Delay)
	{
		float val = m_CurVal * m_Data->m_Magnitude;

		NoteTime -= m_Data->m_Delay;
		if(NoteTime < m_Data->m_Attack)
			val *= NoteTime / m_Data->m_Attack;

		return val * m_Data->m_BaseValue + (ZeroCentered ? 0.0f : m_Data->m_BaseValue);
	}

	if(m_Data->m_Delay > 0.0f)
	{
		float val = m_Data->m_BaseValue * (NoteTime/m_Data->m_Delay);
		return ZeroCentered ? m_Data->m_BaseValue - val : val;
	}

	return 0.0f;
}

//-----------------------------------------------------
void LFOTransients::Update(float FrameTime)
{
	unsigned int c = unsigned int(m_Cursor);
	m_CurVal = std::lerp(m_SrcWaveForm->m_Data[c], m_SrcWaveForm->m_Data[(c+1) % PlaybackFreq], m_Cursor - std::floor(m_Cursor));

/*
	if(m_CurVal < 0.0f) // ???
		m_CurVal *= -m_CurVal;
	else
		m_CurVal *= m_CurVal;
		*/

	m_Cursor += FrameTime * m_Data->m_Rate * (float)PlaybackFreq;
	while(unsigned int(m_Cursor) >= PlaybackFreq)
		m_Cursor -= PlaybackFreq;
}

//-----------------------------------------------------
void LFOTransients::NoteOn()
{
	if(m_Data->m_NoteSync)
		m_Cursor = 0;
}

};