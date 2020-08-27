#include "SynthOl.h"

//-----------------------------------------------------
void SO_AnalogSource::Init(SO_StereoSoundBuf * _OutBuf, SynthOl * _Synth, SO_AnalogSourceData * _Data, int _Channel)
{
	MemClear((unsigned char *)&m_Transients, sizeof(SO_Transients));

	m_Data = _Data;

	SO_SoundSource::Init(_OutBuf, _Synth, _Channel); 

	for(int i = 0; i < ANALOGSOURCE_OSCILLATOR_NR; i++)
	{
		m_LowFreqOscillatorTab[i*SO_LowFreqOsc_Max+SO_LowFreqOsc_Volume ].Init(_Synth, &m_Data->m_LowFreqOscillatorTab[i*SO_LowFreqOsc_Max+SO_LowFreqOsc_Volume ]);
		m_LowFreqOscillatorTab[i*SO_LowFreqOsc_Max+SO_LowFreqOsc_Morph  ].Init(_Synth, &m_Data->m_LowFreqOscillatorTab[i*SO_LowFreqOsc_Max+SO_LowFreqOsc_Morph  ]);
		m_LowFreqOscillatorTab[i*SO_LowFreqOsc_Max+SO_LowFreqOsc_Distort].Init(_Synth, &m_Data->m_LowFreqOscillatorTab[i*SO_LowFreqOsc_Max+SO_LowFreqOsc_Distort]);
		m_LowFreqOscillatorTab[i*SO_LowFreqOsc_Max+SO_LowFreqOsc_Tune   ].Init(_Synth, &m_Data->m_LowFreqOscillatorTab[i*SO_LowFreqOsc_Max+SO_LowFreqOsc_Tune   ]);

		SetOscillator((SO_WaveForms)m_Data->m_OscillatorTab[i].m_WF0, (SO_WaveForms)m_Data->m_OscillatorTab[i].m_WF1, i);
	}

	m_Data->m_PolyphonyMode = SO_PolyphonyMode_Arpeggio;
	m_Data->m_PortamentoTime = 0.5f;
}

//-----------------------------------------------------
void SO_AnalogSource::NoteOn(int _KeyId, float _Velocity)
{	
	for(int k = 0; k < ANALOGSOURCE_POLYPHONY_NOTE_NR; k++)
	{
		if(m_Transients.m_NoteTab[k].m_Code == _KeyId && m_Transients.m_NoteTab[k].m_NoteOn)
			return;
	}

	int Idx = 0;

	if(m_Data->m_PolyphonyMode == SO_PolyphonyMode_Portamento)
	{
		if(m_Transients.m_NoteTab[0].m_NoteOn)
		{
			m_Transients.m_PortamentoCurFreq = SO_GetNoteFreq(m_Transients.m_NoteTab[0].m_Code);
			m_Transients.m_PortamentoStep = (SO_GetNoteFreq(_KeyId) - m_Transients.m_PortamentoCurFreq) / m_Data->m_PortamentoTime;
			m_Transients.m_NoteTab[0].NoteOff();
		}
		else
		{
			m_Transients.m_PortamentoCurFreq = SO_GetNoteFreq(_KeyId);
			m_Transients.m_PortamentoStep = 0.0f;
		}
		goto Out;
	}
	else
	{
		for(int k = 0; k < ANALOGSOURCE_POLYPHONY_NOTE_NR; k++)
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

	for(int j = 0; j < ANALOGSOURCE_LOWFREQ_OSCILLATOR_NR; j++)
		m_LowFreqOscillatorTab[j].NoteOn();
}

//-----------------------------------------------------
void SO_AnalogSource::NoteOff(int _KeyId)
{
	for(int k = 0; k < ANALOGSOURCE_POLYPHONY_NOTE_NR; k++)
	{
		if(m_Transients.m_NoteTab[k].m_Code == _KeyId)
		{
			m_Transients.m_NoteTab[k].NoteOff();
			return;
		}
	}
}

//-----------------------------------------------------
float SO_AnalogSource::GetADSRValue(SO_Note * _Note, float _Time)
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
void SO_AnalogSource::SetOscillator(SO_WaveForms _Wave, SO_WaveForms _MorphWave, int _Index)
{
	SO_Waveform * Wave = m_Synth->GetWaveForm(_Wave);
	SO_Waveform * MorphWave = m_Synth->GetWaveForm(_MorphWave);

	for(int k = 0; k < ANALOGSOURCE_POLYPHONY_NOTE_NR; k++)
	{
		if(m_Transients.m_OscillatorTab[_Index].m_SrcWaveForm != Wave)
			m_Transients.m_InterpTab[k][_Index].m_Cursor = 0.0f;

		if(m_Transients.m_OscillatorTab[_Index].m_SrcMorphWaveForm != MorphWave)
			m_Transients.m_InterpTab[k][_Index].m_Cursor = 0.0f;
	}

	m_Transients.m_OscillatorTab[_Index].m_SrcWaveForm = Wave; 
	m_Transients.m_OscillatorTab[_Index].m_SrcMorphWaveForm = MorphWave; 
	m_Data->m_OscillatorTab[_Index].m_WF0 = _Wave;
	m_Data->m_OscillatorTab[_Index].m_WF1 = _MorphWave;
}

//-----------------------------------------------------
void SO_AnalogSource::Render(long _SampleNr)
{
	long wc = m_OutBuf->m_WriteCursor;
	long wcSav = wc;

	float time = (float)_SampleNr / SO_PLAYBACK_FREQ;

	// update Low freq oscillators
	for(int j = 0; j < ANALOGSOURCE_LOWFREQ_OSCILLATOR_NR; j++)
		m_LowFreqOscillatorTab[j].Update(time);
	
	int nbActiveNotes = 0;
	for(int k = 0; k < ANALOGSOURCE_POLYPHONY_NOTE_NR; k++)
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

	int PolyNoteNr = (m_Data->m_PolyphonyMode == SO_PolyphonyMode_Poly ? ANALOGSOURCE_POLYPHONY_NOTE_NR : 1);
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
		case SO_PolyphonyMode_Arpeggio:		noteIdx = m_Transients.m_ArpeggioIdx>>1;	break;
		case SO_PolyphonyMode_Portamento:	noteIdx = 0;								break;
		}

		float NoteFreq = SO_GetNoteFreq(m_Transients.m_NoteTab[noteIdx].m_Code);
		
		// manage the portamento
		if(m_Data->m_PolyphonyMode == SO_PolyphonyMode_Portamento)
		{
			float Newfreq = m_Transients.m_PortamentoCurFreq + m_Transients.m_PortamentoStep * time;
			if(m_Transients.m_PortamentoCurFreq > NoteFreq)
				m_Transients.m_PortamentoCurFreq = (Newfreq < NoteFreq ? NoteFreq : Newfreq);
			else if(m_Transients.m_PortamentoCurFreq < NoteFreq)
				m_Transients.m_PortamentoCurFreq = (Newfreq > NoteFreq ? NoteFreq : Newfreq);

			NoteFreq = m_Transients.m_PortamentoCurFreq;
		}

		// update Oscillators
		for(int j = 0; j < ANALOGSOURCE_OSCILLATOR_NR; j++)
		{
			m_Transients.m_OscillatorTab[j].m_Volume		= m_LowFreqOscillatorTab[j*SO_LowFreqOsc_Max+SO_LowFreqOsc_Volume ].GetValue(m_Transients.m_NoteTab[k].m_Time);
			m_Transients.m_OscillatorTab[j].m_Morph			= m_LowFreqOscillatorTab[j*SO_LowFreqOsc_Max+SO_LowFreqOsc_Morph  ].GetValue(m_Transients.m_NoteTab[k].m_Time);
			m_Transients.m_OscillatorTab[j].m_DistortGain	= m_LowFreqOscillatorTab[j*SO_LowFreqOsc_Max+SO_LowFreqOsc_Distort].GetValue(m_Transients.m_NoteTab[k].m_Time);
			m_Transients.m_OscillatorTab[j].m_StepShift		= m_LowFreqOscillatorTab[j*SO_LowFreqOsc_Max+SO_LowFreqOsc_Tune   ].GetValue(m_Transients.m_NoteTab[k].m_Time, true);

			float Freq = NoteFreq;
			for(int i = 0; i < m_Data->m_OscillatorTab[j].m_OctaveOffset; i++)	Freq *= 2.0f;
			for(int i = 0; i > m_Data->m_OscillatorTab[j].m_OctaveOffset; i--)	Freq *= 0.5f;
			
			// update step value
			if(m_Transients.m_OscillatorTab[j].m_SrcWaveForm == m_Synth->GetWaveForm(SO_Wave_Rand))
				m_Transients.m_OscillatorTab[j].m_Step = 1.0f / (36.0f*m_Transients.m_OscillatorTab[j].m_StepShift+1.0f);
			else
				m_Transients.m_OscillatorTab[j].m_Step = Freq + m_Transients.m_OscillatorTab[j].m_StepShift;

			if(m_Transients.m_OscillatorTab[j].m_Step < 0.0f)
				m_Transients.m_OscillatorTab[j].m_Step = 0.0f;

			if(m_Transients.m_OscillatorTab[j].m_Volume < 0.0f)
				m_Transients.m_OscillatorTab[j].m_Volume = 0.0f;

			FloatClamp01(m_Transients.m_OscillatorTab[j].m_Morph);

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
			for(int j = 0; j < ANALOGSOURCE_OSCILLATOR_NR; j++)
			{
				long cursor = float2int(m_Transients.m_InterpTab[k][j].m_Cursor);

				float val = (1.0f - m_Transients.m_InterpTab[k][j].m_Morph) * m_Transients.m_OscillatorTab[j].m_SrcWaveForm->m_Wave[cursor];
				val += m_Transients.m_InterpTab[k][j].m_Morph * m_Transients.m_OscillatorTab[j].m_SrcMorphWaveForm->m_Wave[cursor];
				val = SO_Distortion(m_Transients.m_InterpTab[k][j].m_DistortGain, val);
				val *= m_Transients.m_InterpTab[k][j].m_Volume;

				switch(m_Data->m_OscillatorTab[j].m_ModulationType)
				{
				case SO_ModulationType_Mix:		Output += val;	break;
				case SO_ModulationType_Mul:		Output *= val;	break;
				case SO_ModulationType_Ring:	Output *= 1.0f - 0.5f*(val+m_Transients.m_InterpTab[k][j].m_Volume);	break;
				}

				// interp
				m_Transients.m_InterpTab[k][j].m_Volume			+= m_Transients.m_OscillatorTab[j].m_VolumeInterp;
				m_Transients.m_InterpTab[k][j].m_Morph			+= m_Transients.m_OscillatorTab[j].m_MorphInterp;
				m_Transients.m_InterpTab[k][j].m_DistortGain	+= m_Transients.m_OscillatorTab[j].m_DistortGainInterp;

				// avance le curseur de lecture de l'oscillateur
				m_Transients.m_InterpTab[k][j].m_Cursor += m_Transients.m_OscillatorTab[j].m_Step;		
				while(float2int(m_Transients.m_InterpTab[k][j].m_Cursor) >= m_Transients.m_OscillatorTab[j].m_SrcWaveForm->m_Size)
					m_Transients.m_InterpTab[k][j].m_Cursor -= m_Transients.m_OscillatorTab[j].m_SrcWaveForm->m_Size;
			}

			m_OutBuf->m_Left[wc] +=  Output * m_Data->m_LeftVolume;
			m_OutBuf->m_Right[wc] += Output * m_Data->m_RightVolume;

			// avance le curseur temporaire d'écriture
			wc++;
			if(wc >= m_OutBuf->m_Size)
				wc = 0;
		}
	}
}
