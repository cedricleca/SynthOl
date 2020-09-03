#include "SynthOl.h"
#include <algorithm>

namespace SynthOl
{

//-----------------------------------------------------
AnalogSource::AnalogSource(StereoSoundBuf * Dest, int Channel, AnalogSourceData * Data) : 
	SoundSource(Dest, Channel),
	m_Data(Data)
{
	m_Data->m_PolyphonyMode = PolyphonyMode::Poly;
	m_Data->m_PortamentoTime = 0.5f;
}

void AnalogSource::OnBound(Synth * Synth)
{ 
	SoundSource::OnBound(Synth); 

	for(int i = 0; i < AnalogsourceOscillatorNr; i++)
	{
		for(int j = 0; j < int(LFODest::Max); j++)
			m_OscillatorTab[i].m_LFOTab[j].Init(&m_Data->m_OscillatorTab[i].m_LFOTab[j], *m_Synth);

		SetOscillator((WaveType)m_Data->m_OscillatorTab[i].m_WF0, (WaveType)m_Data->m_OscillatorTab[i].m_WF1, i);
	}
}

//-----------------------------------------------------
void AnalogSource::NoteOn(int _KeyId, float _Velocity)
{	
	for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
	{
		if(m_NoteTab[k].m_Code == _KeyId && m_NoteTab[k].m_NoteOn)
			return;
	}

	int Idx = 0;

	if(m_Data->m_PolyphonyMode == PolyphonyMode::Portamento)
	{
		if(m_NoteTab[0].m_NoteOn)
		{
			m_PortamentoCurFreq = GetNoteFreq(m_NoteTab[0].m_Code);
			m_PortamentoStep = (GetNoteFreq(_KeyId) - m_PortamentoCurFreq) / m_Data->m_PortamentoTime;
			m_NoteTab[0].NoteOff();
		}
		else
		{
			m_PortamentoCurFreq = GetNoteFreq(_KeyId);
			m_PortamentoStep = 0.0f;
		}
		goto Out;
	}
	else
	{
		for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
		{
			if(!m_NoteTab[k].m_NoteOn)
			{
				Idx = k;
				goto Out;
			}
		}
	}

Out:
	m_NoteTab[Idx].NoteOn(_KeyId, _Velocity);

	for(int i = 0; i < AnalogsourceOscillatorNr; i++)
		for(int j = 0; j < int(LFODest::Max); j++)
			m_OscillatorTab[i].m_LFOTab[j].NoteOn();
}

//-----------------------------------------------------
void AnalogSource::NoteOff(int _KeyId)
{
	for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
	{
		if(m_NoteTab[k].m_Code == _KeyId)
		{
			m_NoteTab[k].NoteOff();
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
void AnalogSource::SetOscillator(WaveType Wave, WaveType MorphWave, int OscIndex)
{
	const Waveform & BaseWaveForm = m_Synth->GetWaveForm(Wave);
	const Waveform & MorphWaveForm = m_Synth->GetWaveForm(MorphWave);

	for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
	{
		if(m_OscillatorTab[OscIndex].m_SrcWaveForm != &BaseWaveForm)
			m_NoteTab[k].m_InterpTab[OscIndex].m_Cursor = 0.0f;

		if(m_OscillatorTab[OscIndex].m_SrcMorphWaveForm != &MorphWaveForm)
			m_NoteTab[k].m_InterpTab[OscIndex].m_Cursor = 0.0f;
	}

	m_OscillatorTab[OscIndex].m_SrcWaveForm = &BaseWaveForm; 
	m_OscillatorTab[OscIndex].m_SrcMorphWaveForm = &MorphWaveForm; 
	m_Data->m_OscillatorTab[OscIndex].m_WF0 = Wave;
	m_Data->m_OscillatorTab[OscIndex].m_WF1 = MorphWave;
}

//-----------------------------------------------------
void AnalogSource::Render(long SampleNr)
{
	long wc = m_Dest->m_WriteCursor;
	long wcSav = wc;

	float time = (float)SampleNr / PlaybackFreq;

	// update Low freq oscillators
	for(int i = 0; i < AnalogsourceOscillatorNr; i++)
		for(int j = 0; j < int(LFODest::Max); j++)
			m_OscillatorTab[i].m_LFOTab[j].Update(time);
	
	int nbActiveNotes = 0;
	for(int k = 0; k < AnalogsourcePolyphonyNoteNr; k++)
	{
		// arpeggio : count active notes
		if(m_NoteTab[k].m_NoteOn)
			nbActiveNotes++;
	}
	// next arpeggio step
	if(nbActiveNotes > 0)
	{
		m_ArpeggioIdx++;
		m_ArpeggioIdx %= (nbActiveNotes<<1);
	}

	int PolyNoteNr = (m_Data->m_PolyphonyMode == PolyphonyMode::Poly ? AnalogsourcePolyphonyNoteNr : 1);
	for(int k = 0; k < PolyNoteNr; k++)
	{
		auto & Note = m_NoteTab[k];
		Note.m_Time += time;
	
		// Get ADSR and Velocity
		float VolumeMultiplier = GetADSRValue(&Note, time) * Note.m_Velocity * 1.5f;
		if(VolumeMultiplier == 0.0f)
			continue;

		// select the active note depending on arpeggio mode
		float NoteFreq;
		switch(m_Data->m_PolyphonyMode)
		{
		case 
			PolyphonyMode::Arpeggio:	
			NoteFreq = GetNoteFreq(m_NoteTab[m_ArpeggioIdx>>1].m_Code);
			break;

		case PolyphonyMode::Portamento:	
			NoteFreq = GetNoteFreq(m_NoteTab[0].m_Code);
			{
				float Newfreq = m_PortamentoCurFreq + m_PortamentoStep * time;
				if(m_PortamentoCurFreq > NoteFreq)
					m_PortamentoCurFreq = (Newfreq < NoteFreq ? NoteFreq : Newfreq);
				else if(m_PortamentoCurFreq < NoteFreq)
					m_PortamentoCurFreq = (Newfreq > NoteFreq ? NoteFreq : Newfreq);

			}
			NoteFreq = m_PortamentoCurFreq;
			break;

		default:
			NoteFreq = GetNoteFreq(m_NoteTab[k].m_Code);
			break;
		}

		// update Oscillators
		for(int j = 0; j < AnalogsourceOscillatorNr; j++)
		{
			auto & Oscillator = m_OscillatorTab[j];
			Oscillator.m_Volume			= m_OscillatorTab[j].m_LFOTab[int(LFODest::Volume )].GetValue(Note.m_Time);
			Oscillator.m_Morph			= m_OscillatorTab[j].m_LFOTab[int(LFODest::Morph  )].GetValue(Note.m_Time);
			Oscillator.m_DistortGain	= m_OscillatorTab[j].m_LFOTab[int(LFODest::Distort)].GetValue(Note.m_Time);
			Oscillator.m_StepShift		= m_OscillatorTab[j].m_LFOTab[int(LFODest::Tune   )].GetValue(Note.m_Time, true);

			for(int i = 0; i < m_Data->m_OscillatorTab[j].m_OctaveOffset; i++)	NoteFreq *= 2.0f;
			for(int i = 0; i > m_Data->m_OscillatorTab[j].m_OctaveOffset; i--)	NoteFreq *= 0.5f;
			
			// update step value
			if(Oscillator.m_SrcWaveForm == &m_Synth->GetWaveForm(WaveType::Rand))
				Oscillator.m_Step = 1.0f / (36.0f*Oscillator.m_StepShift+1.0f);
			else
				Oscillator.m_Step = NoteFreq + Oscillator.m_StepShift;

			Oscillator.m_Step = std::min(Oscillator.m_Step, 0.f);
			Oscillator.m_Morph = std::clamp(Oscillator.m_Morph, 0.f, 1.f);
			Oscillator.m_Volume = std::min(Oscillator.m_Volume, 0.f);
			Oscillator.m_Volume *= VolumeMultiplier;

			// calc interp value
			Oscillator.m_VolumeInterp = (Oscillator.m_Volume - Note.m_InterpTab[j].m_Volume) / SampleNr;
			Oscillator.m_MorphInterp = (Oscillator.m_Morph - Note.m_InterpTab[j].m_Morph) / SampleNr;
			Oscillator.m_DistortGainInterp = (Oscillator.m_DistortGain - Note.m_InterpTab[j].m_DistortGain) / SampleNr;
		}

		wc = wcSav;

		// compute samples
		for(long i = 0; i < SampleNr; i++)
		{
			float Output = 0.0f;
			for(int j = 0; j < AnalogsourceOscillatorNr; j++)
			{
				const auto & Oscillator = m_OscillatorTab[j];
				auto & NoteInterp = Note.m_InterpTab[j];
				long cursor = int(NoteInterp.m_Cursor);

				float val = (1.0f - NoteInterp.m_Morph) * Oscillator.m_SrcWaveForm->m_Data[cursor];
				val += NoteInterp.m_Morph * Oscillator.m_SrcMorphWaveForm->m_Data[cursor];
				val = Distortion(NoteInterp.m_DistortGain, val);
				val *= NoteInterp.m_Volume;

				switch(m_Data->m_OscillatorTab[j].m_ModulationType)
				{
				case ModulationType::Mix:	Output += val;	break;
				case ModulationType::Mul:	Output *= val;	break;
				case ModulationType::Ring:	Output *= 1.0f - 0.5f*(val+NoteInterp.m_Volume);	break;
				}

				// interp
				NoteInterp.m_Volume			+= Oscillator.m_VolumeInterp;
				NoteInterp.m_Morph			+= Oscillator.m_MorphInterp;
				NoteInterp.m_DistortGain	+= Oscillator.m_DistortGainInterp;

				// avance le curseur de lecture de l'oscillateur
				NoteInterp.m_Cursor += Oscillator.m_Step;		
				while(unsigned int(NoteInterp.m_Cursor) >= PlaybackFreq)
					NoteInterp.m_Cursor -= PlaybackFreq;
			}

			m_Dest->m_Data[wc] = { Output * m_Data->m_LeftVolume, Output * m_Data->m_RightVolume };
			wc = (wc + 1) % PlaybackFreq;
		}
	}
}

};