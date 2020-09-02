#include "SynthOl.h"
#include <algorithm>

namespace SynthOl
{

//-----------------------------------------------------
AnalogSource::AnalogSource(StereoSoundBuf * Dest, Synth * Synth, int Channel, AnalogSourceData * Data) : 
	SoundSource(Dest, Synth, Channel),
	m_Data(Data)
{
	for(int i = 0; i < AnalogsourceOscillatorNr; i++)
	{
		for(int j = 0; j < int(LFODest::Max); j++)
			m_LFOTab[i][j].Init(Synth, &m_Data->m_LFOTab[i][j]);

		SetOscillator((WaveType)m_Data->m_OscillatorTab[i].m_WF0, (WaveType)m_Data->m_OscillatorTab[i].m_WF1, i);
	}

	m_Data->m_PolyphonyMode = PolyphonyMode::Poly;
	m_Data->m_PortamentoTime = 0.5f;
}

//-----------------------------------------------------
void AnalogSource::NoteOn(int _KeyId, float _Velocity)
{	
	for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
	{
		if(m_Transients.m_NoteTab[k].m_Code == _KeyId && m_Transients.m_NoteTab[k].m_NoteOn)
			return;
	}

	int Idx = 0;

	if(m_Data->m_PolyphonyMode == PolyphonyMode::Portamento)
	{
		if(m_Transients.m_NoteTab[0].m_NoteOn)
		{
			m_Transients.m_PortamentoCurFreq = GetNoteFreq(m_Transients.m_NoteTab[0].m_Code);
			m_Transients.m_PortamentoStep = (GetNoteFreq(_KeyId) - m_Transients.m_PortamentoCurFreq) / m_Data->m_PortamentoTime;
			m_Transients.m_NoteTab[0].NoteOff();
		}
		else
		{
			m_Transients.m_PortamentoCurFreq = GetNoteFreq(_KeyId);
			m_Transients.m_PortamentoStep = 0.0f;
		}
		goto Out;
	}
	else
	{
		for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
		{
			if(!m_Transients.m_NoteTab[k].m_NoteOn)
			{
				Idx = k;
				goto Out;
			}
		}
	}

Out:
	m_Transients.m_NoteTab[Idx].NoteOn(_KeyId, _Velocity);

	for(int i = 0; i < AnalogsourceOscillatorNr; i++)
		for(int j = 0; j < int(LFODest::Max); j++)
			m_LFOTab[i][j].NoteOn();
}

//-----------------------------------------------------
void AnalogSource::NoteOff(int _KeyId)
{
	for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
	{
		if(m_Transients.m_NoteTab[k].m_Code == _KeyId)
		{
			m_Transients.m_NoteTab[k].NoteOff();
			return;
		}
	}
}

//-----------------------------------------------------
float AnalogSource::GetADSRValue(Note * _Note, float _Time)
{
	if(_Note->m_NoteOn)
	{
		if(_Note->m_Time > m_Data->m_ADSR_Attack + m_Data->m_ADSR_Decay)
		{
			_Note->m_SustainTime += _Time;
			return m_Data->m_ADSR_Sustain;
		}
		else
		{
			if(_Note->m_Time > m_Data->m_ADSR_Attack && m_Data->m_ADSR_Decay > 0.0f)
				return 1.0f + ((_Note->m_Time - m_Data->m_ADSR_Attack) / m_Data->m_ADSR_Decay) * (m_Data->m_ADSR_Sustain - 1.0f);
			else if(m_Data->m_ADSR_Attack > 0.0f)
				return (_Note->m_Time / m_Data->m_ADSR_Attack);
			else
				return 0.0f;
		}
	}
	else
	{
		if(_Note->m_Time - _Note->m_SustainTime < m_Data->m_ADSR_Release && m_Data->m_ADSR_Release > 0.0f)
			return (1.0f - ((_Note->m_Time - _Note->m_SustainTime) / m_Data->m_ADSR_Release)) * m_Data->m_ADSR_Sustain;
		else
		{
			_Note->m_Died = true;
			return 0.0f;
		}
	}
}

//-----------------------------------------------------
void AnalogSource::SetOscillator(WaveType Wave, WaveType MorphWave, int Index)
{
	const Waveform & BaseWaveForm = m_Synth->GetWaveForm(Wave);
	const Waveform & MorphWaveForm = m_Synth->GetWaveForm(MorphWave);

	for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
	{
		if(m_Transients.m_OscillatorTab[Index].m_SrcWaveForm != &BaseWaveForm)
			m_Transients.m_InterpTab[k][Index].m_Cursor = 0.0f;

		if(m_Transients.m_OscillatorTab[Index].m_SrcMorphWaveForm != &MorphWaveForm)
			m_Transients.m_InterpTab[k][Index].m_Cursor = 0.0f;
	}

	m_Transients.m_OscillatorTab[Index].m_SrcWaveForm = &BaseWaveForm; 
	m_Transients.m_OscillatorTab[Index].m_SrcMorphWaveForm = &MorphWaveForm; 
	m_Data->m_OscillatorTab[Index].m_WF0 = Wave;
	m_Data->m_OscillatorTab[Index].m_WF1 = MorphWave;
}

//-----------------------------------------------------
void AnalogSource::Render(long _SampleNr)
{
	long wc = m_Dest->m_WriteCursor;
	long wcSav = wc;

	float time = (float)_SampleNr / PlaybackFreq;

	// update Low freq oscillators
	for(int i = 0; i < AnalogsourceOscillatorNr; i++)
		for(int j = 0; j < int(LFODest::Max); j++)
			m_LFOTab[i][j].Update(time);
	
	int nbActiveNotes = 0;
	for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
	{
		// arpeggio : count active notes
		if(m_Transients.m_NoteTab[k].m_NoteOn)
			nbActiveNotes++;
	}
	// next arpeggio step
	if(nbActiveNotes > 0)
	{
		m_Transients.m_ArpeggioIdx++;
		m_Transients.m_ArpeggioIdx %= (nbActiveNotes<<1);
	}

	int PolyNoteNr = (m_Data->m_PolyphonyMode == PolyphonyMode::Poly ? AnalogsourcePolyphonyNoteNr : 1);
	for(int k = 0; k < PolyNoteNr; k++)
	{
		m_Transients.m_NoteTab[k].m_Time += time;
	
		// Get ADSR and Velocity
		float VolumeMultiplier = GetADSRValue(&m_Transients.m_NoteTab[k], time) * m_Transients.m_NoteTab[k].m_Velocity * 1.5f;
		if(VolumeMultiplier == 0.0f)
			continue;

		// select the active note depending on arpeggio mode
		int noteIdx = k;
		switch(m_Data->m_PolyphonyMode)
		{
		case PolyphonyMode::Arpeggio:	noteIdx = m_Transients.m_ArpeggioIdx>>1;	break;
		case PolyphonyMode::Portamento:	noteIdx = 0;								break;
		}

		float NoteFreq = GetNoteFreq(m_Transients.m_NoteTab[noteIdx].m_Code);
		
		// manage the portamento
		if(m_Data->m_PolyphonyMode == PolyphonyMode::Portamento)
		{
			float Newfreq = m_Transients.m_PortamentoCurFreq + m_Transients.m_PortamentoStep * time;
			if(m_Transients.m_PortamentoCurFreq > NoteFreq)
				m_Transients.m_PortamentoCurFreq = (Newfreq < NoteFreq ? NoteFreq : Newfreq);
			else if(m_Transients.m_PortamentoCurFreq < NoteFreq)
				m_Transients.m_PortamentoCurFreq = (Newfreq > NoteFreq ? NoteFreq : Newfreq);

			NoteFreq = m_Transients.m_PortamentoCurFreq;
		}

		// update Oscillators
		for(int j = 0; j < AnalogsourceOscillatorNr; j++)
		{
			m_Transients.m_OscillatorTab[j].m_Volume		= m_LFOTab[j][int(LFODest::Volume )].GetValue(m_Transients.m_NoteTab[k].m_Time);
			m_Transients.m_OscillatorTab[j].m_Morph			= m_LFOTab[j][int(LFODest::Morph  )].GetValue(m_Transients.m_NoteTab[k].m_Time);
			m_Transients.m_OscillatorTab[j].m_DistortGain	= m_LFOTab[j][int(LFODest::Distort)].GetValue(m_Transients.m_NoteTab[k].m_Time);
			m_Transients.m_OscillatorTab[j].m_StepShift		= m_LFOTab[j][int(LFODest::Tune   )].GetValue(m_Transients.m_NoteTab[k].m_Time, true);

			float Freq = NoteFreq;
			for(int i = 0; i < m_Data->m_OscillatorTab[j].m_OctaveOffset; i++)	Freq *= 2.0f;
			for(int i = 0; i > m_Data->m_OscillatorTab[j].m_OctaveOffset; i--)	Freq *= 0.5f;
			
			// update step value
			if(m_Transients.m_OscillatorTab[j].m_SrcWaveForm == &m_Synth->GetWaveForm(WaveType::Rand))
				m_Transients.m_OscillatorTab[j].m_Step = 1.0f / (36.0f*m_Transients.m_OscillatorTab[j].m_StepShift+1.0f);
			else
				m_Transients.m_OscillatorTab[j].m_Step = Freq + m_Transients.m_OscillatorTab[j].m_StepShift;

			if(m_Transients.m_OscillatorTab[j].m_Step < 0.0f)
				m_Transients.m_OscillatorTab[j].m_Step = 0.0f;

			if(m_Transients.m_OscillatorTab[j].m_Volume < 0.0f)
				m_Transients.m_OscillatorTab[j].m_Volume = 0.0f;

			m_Transients.m_OscillatorTab[j].m_Morph = std::clamp(m_Transients.m_OscillatorTab[j].m_Morph, 0.f, 1.f);

			m_Transients.m_OscillatorTab[j].m_Volume *= VolumeMultiplier;

			// calc interp value
			m_Transients.m_OscillatorTab[j].m_VolumeInterp = (m_Transients.m_OscillatorTab[j].m_Volume - m_Transients.m_InterpTab[k][j].m_Volume) / _SampleNr;
			m_Transients.m_OscillatorTab[j].m_MorphInterp = (m_Transients.m_OscillatorTab[j].m_Morph - m_Transients.m_InterpTab[k][j].m_Morph) / _SampleNr;
			m_Transients.m_OscillatorTab[j].m_DistortGainInterp = (m_Transients.m_OscillatorTab[j].m_DistortGain - m_Transients.m_InterpTab[k][j].m_DistortGain) / _SampleNr;
		}

		wc = wcSav;

		// compute samples
		for(long i = 0; i < _SampleNr; i++)
		{
			float Output = 0.0f;
			for(int j = 0; j < AnalogsourceOscillatorNr; j++)
			{
				long cursor = int(m_Transients.m_InterpTab[k][j].m_Cursor);

				float val = (1.0f - m_Transients.m_InterpTab[k][j].m_Morph) * m_Transients.m_OscillatorTab[j].m_SrcWaveForm->m_Data[cursor];
				val += m_Transients.m_InterpTab[k][j].m_Morph * m_Transients.m_OscillatorTab[j].m_SrcMorphWaveForm->m_Data[cursor];
				val = Distortion(m_Transients.m_InterpTab[k][j].m_DistortGain, val);
				val *= m_Transients.m_InterpTab[k][j].m_Volume;

				switch(m_Data->m_OscillatorTab[j].m_ModulationType)
				{
				case ModulationType::Mix:		Output += val;	break;
				case ModulationType::Mul:		Output *= val;	break;
				case ModulationType::Ring:	Output *= 1.0f - 0.5f*(val+m_Transients.m_InterpTab[k][j].m_Volume);	break;
				}

				// interp
				m_Transients.m_InterpTab[k][j].m_Volume			+= m_Transients.m_OscillatorTab[j].m_VolumeInterp;
				m_Transients.m_InterpTab[k][j].m_Morph			+= m_Transients.m_OscillatorTab[j].m_MorphInterp;
				m_Transients.m_InterpTab[k][j].m_DistortGain	+= m_Transients.m_OscillatorTab[j].m_DistortGainInterp;

				// avance le curseur de lecture de l'oscillateur
				m_Transients.m_InterpTab[k][j].m_Cursor += m_Transients.m_OscillatorTab[j].m_Step;		
				while(unsigned int(m_Transients.m_InterpTab[k][j].m_Cursor) >= m_Transients.m_OscillatorTab[j].m_SrcWaveForm->m_Data.size())
					m_Transients.m_InterpTab[k][j].m_Cursor -= m_Transients.m_OscillatorTab[j].m_SrcWaveForm->m_Data.size();
			}

			m_Dest->m_Data[wc] = { Output * m_Data->m_LeftVolume, Output * m_Data->m_RightVolume };
			wc = (wc + 1) % m_Dest->m_Data.size();
		}
	}
}

};