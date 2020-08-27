

#ifndef _SYNTHOL_
#define _SYNTHOL_

#ifndef NULL
#define NULL 0
#endif

#ifndef DWORD
typedef unsigned long       DWORD;
#endif

#include "MIDIListener.h"

enum SO_LowFreqOscillators
{
	SO_LowFreqOsc_Tune,
	SO_LowFreqOsc_Morph,
	SO_LowFreqOsc_Distort,
	SO_LowFreqOsc_Volume,
	SO_LowFreqOsc_Max
};

#ifndef ANALOGSOURCE_OSCILLATOR_NR
#define ANALOGSOURCE_OSCILLATOR_NR		2
#endif

#ifndef ANALOGSOURCE_LOWFREQ_OSCILLATOR_NR
#define ANALOGSOURCE_LOWFREQ_OSCILLATOR_NR	(ANALOGSOURCE_OSCILLATOR_NR*SO_LowFreqOsc_Max)
#endif

#ifndef SO_SOURCE_NR
#define SO_SOURCE_NR					16
#endif

#ifndef SO_PLAYBACK_FREQ
#define SO_PLAYBACK_FREQ				44100
#endif

#ifndef ANALOGSOURCE_POLYPHONY_NOTE_NR
#define ANALOGSOURCE_POLYPHONY_NOTE_NR	3
#endif

#define SO_PRIMARY_BUFFER_SIZE  (SO_PLAYBACK_FREQ*2)

enum SO_WaveForms
{
	SO_Wave_Square,
	SO_Wave_Saw,
	SO_Wave_RampUp,
	SO_Wave_Rand,
	SO_Wave_Square_Soft,
	SO_Wave_Saw_Soft,
	SO_Wave_RampUp_Soft,
	SO_Wave_Max,
};

enum SO_SourceType
{
	SO_SourceType_Sample,
	SO_SourceType_Analog,
	SO_SourceType_EchoFilter,
};

enum SO_ModulationType
{
	SO_ModulationType_Mix,
	SO_ModulationType_Mul,
	SO_ModulationType_Ring,
	SO_ModulationType_Max,
};

enum SO_TrackCommand
{
	SO_TrackCommand_NoteOn,
	SO_TrackCommand_NoteOff,
	SO_TrackCommand_Loop,
	SO_TrackCommand_Wait,
};

enum SO_PolyphonyMode
{
	SO_PolyphonyMode_Poly,
	SO_PolyphonyMode_Arpeggio,
	SO_PolyphonyMode_Portamento,
};

extern float SO_OctaveFreq[];
class SynthOl;

// round a float to int using the actual rounding mode 
// round a float to int using the actual rounding mode 
inline int float2int(float x) 
{
	/*
    int i;
    __asm {
        fld x
        fistp i
    }
	*/
    return int(x);
}

void FloatClear(float * Dest, long len);
void MemClear(unsigned char * Dest, long len);
float SO_Distortion(float _Gain, float _Sample);
void FloatClamp01(float & _Val);
float SO_GetNoteFreq(int _NoteCode);

//_________________________________________________
class SO_SoundBuf
{
public:
	long	m_Size;
	long	m_WriteCursor;

	virtual void Allocate(long _Size)
	{
		m_Size = _Size;
		m_WriteCursor = 0;
		Clear(0, _Size);
	}

	virtual void Clear(long _Index, long _Size) {}
};

//_________________________________________________
class SO_StereoSoundBuf : public SO_SoundBuf 
{
public:
	float * m_Left;
	float * m_Right;

	void Allocate(long _Size);
	void Clear(long _Index, long _Size);
};

//_________________________________________________
class SO_Waveform : public SO_SoundBuf 
{
public:
	float * m_Wave;

	void Allocate(long _Size);
	void GenerateWave(SO_WaveForms _Type, long _Index, long _Period, long _Size, float _Magnitude);
	void Soften(long _Index, long _Size, float _Coef);
	void Normalize(long _Index, long _Size, float _Coef);
	void Clear(long _Index, long _Size);
	void Copy(SO_Waveform * _Src, long _SrcIndex, long _DstIndex, long _Size);
	void operator *= (float _Coef);
};

class SO_SoundSource;

//_________________________________________________
class SO_SoundSource
{
protected:
	SO_StereoSoundBuf	* m_OutBuf;
	SynthOl				* m_Synth;
	int					m_Channel;

public:
	virtual void Init(SO_StereoSoundBuf * _OutBuf, SynthOl * _Synth, int _Channel)
	{
		m_OutBuf = _OutBuf; 
		m_Channel = _Channel;
		m_Synth = _Synth;
	}

	virtual void NoteOn(int _Channel, int _KeyId, float _Velocity)
	{
		if(_Channel==m_Channel)
			NoteOn(_KeyId, _Velocity);
	}
	virtual void NoteOff(int _Channel, int _KeyId)
	{
		if(_Channel==m_Channel)
			NoteOff(_KeyId);
	}
	virtual void NoteOn(int _KeyId, float _Velocity){}
	virtual void NoteOff(int _KeyId){}
	virtual void Render(long _SampleNr){}
	virtual SO_StereoSoundBuf * GetOutBuf(){ return m_OutBuf; }
};

//_________________________________________________
class SO_FilterSource:public SO_SoundSource
{
protected:
	SO_StereoSoundBuf		m_SrcWaveForm;

public:
	long			m_Cursor;

	virtual void Init(SO_StereoSoundBuf * _OutBuf, SynthOl * _Synth, int _Channel);
	virtual void Render(long _SampleNr){}
	virtual SO_StereoSoundBuf * GetSrcWaveForm();
};

//_________________________________________________
class SO_EchoFilterSource:public SO_FilterSource
{
	SO_StereoSoundBuf		m_DelayWaveForm;
	float			m_S0;
	float			m_S1;

public:
	long	m_DelayLen;
	float	m_Feedback;
	long	m_ResoDelayLen;
	float	m_ResoFeedback;
	long	m_ResoSteps;

	virtual void Init(long _DelayLen, SO_StereoSoundBuf * _OutBuf, SynthOl * _Synth, int _Channel);
	virtual void Render(long _SampleNr);
};

//_________________________________________________
class SO_SampleSource:public SO_SoundSource
{
	SO_Waveform		* m_SrcWaveForm;

public:
	bool			m_bLoop;
	float			m_Volume;
	float			m_Step;
	float			m_Cursor;

	void Init(SO_Waveform * _SrcWaveForm, SO_StereoSoundBuf * _OutBuf, SynthOl * _Synth, int _Channel){	m_SrcWaveForm = _SrcWaveForm; m_Step = 1.0f; SO_SoundSource::Init(_OutBuf, _Synth, _Channel); }
	void NoteOn(int _KeyId, float _Velocity){	m_Cursor = 0; }
	void Render(long _SampleNr);
};

//_________________________________________________
class SO_Note
{
public:
	float	m_Time;
	float	m_SustainTime;
	bool	m_NoteOn;
	float	m_Velocity;
	int		m_Code;
	bool	m_Died;

	void NoteOn(int _KeyId, float _Velocity)
	{
		if(m_NoteOn)
			return;

		m_Time = 0.0f; 
		m_SustainTime = 0.0f; 
		m_NoteOn = true; 
		m_Code = _KeyId;
		m_Velocity = _Velocity;
		m_Died = false;
	}
	void NoteOff(){ m_NoteOn = false; }
};

//_________________________________________________
struct SO_LowFreqOscillatorData
{
	float				m_Delay;
	float				m_Attack;
	float				m_Magnitude;
	float				m_Rate;
	float				m_BaseValue;
	char				m_WF;
	char				m_NoteSync;
};

//_________________________________________________
class SO_LowFreqOscillator
{
	SO_Waveform			* m_SrcWaveForm;
	float				m_Cursor;
	float				m_CurVal;
	SynthOl				* m_Synth;

public:
	SO_LowFreqOscillatorData	* m_Data;

	void Init(SynthOl * _Synth, SO_LowFreqOscillatorData	* _Data);
	void Update(float _FrameTime);
	float GetValue(float _NoteTime, bool _ZeroCentered=false);
	void SetOscillator(SO_WaveForms _Wave);
	void NoteOn();
};


//_________________________________________________
struct SO_OscillatorData
{
	char				m_OctaveOffset;
	char				m_NoteOffset;	
	char				m_WF0;
	char				m_WF1;
	SO_ModulationType	m_ModulationType;
};

//_________________________________________________
struct SO_AnalogSourceData
{
	SO_OscillatorData			m_OscillatorTab[ANALOGSOURCE_OSCILLATOR_NR];
	SO_LowFreqOscillatorData	m_LowFreqOscillatorTab[ANALOGSOURCE_LOWFREQ_OSCILLATOR_NR];
	float						m_ADSR_Attack;
	float						m_ADSR_Decay;
	float						m_ADSR_Sustain;
	float						m_ADSR_Release;
	float						m_LeftVolume;
	float						m_RightVolume;
	float						m_PortamentoTime;
	char						m_PolyphonyMode;
};

//_________________________________________________
class SO_AnalogSource:public SO_SoundSource
{
	//_________________________________________________
	struct SO_OscillatorInterp
	{
		float		m_Volume;
		float		m_DistortGain;
		float		m_Morph;
		float		m_Cursor;
	};

	//_________________________________________________
	struct SO_OscillatorTransients
	{
		SO_Waveform			* m_SrcWaveForm;
		SO_Waveform			* m_SrcMorphWaveForm;
		float				m_VolumeInterp;
		float				m_DistortGainInterp;
		float				m_MorphInterp;
		float				m_Volume;
		float				m_Step;
		float				m_StepShift;
		float				m_Morph;
		float				m_DistortGain;
	};

	//_________________________________________________
	struct SO_Transients
	{
		SO_OscillatorTransients	m_OscillatorTab[ANALOGSOURCE_OSCILLATOR_NR];
		SO_Note					m_NoteTab[ANALOGSOURCE_POLYPHONY_NOTE_NR];
		SO_OscillatorInterp		m_InterpTab[ANALOGSOURCE_POLYPHONY_NOTE_NR][ANALOGSOURCE_OSCILLATOR_NR];
		int						m_ArpeggioIdx;
		float					m_PortamentoCurFreq;
		float					m_PortamentoStep;
	};

public:
	SO_AnalogSourceData		* m_Data;
	SO_LowFreqOscillator	m_LowFreqOscillatorTab[ANALOGSOURCE_LOWFREQ_OSCILLATOR_NR];
	SO_Transients			m_Transients;

	void Init(SO_StereoSoundBuf * _OutBuf, SynthOl * _Synth, SO_AnalogSourceData * _Data, int _Channel);
	void NoteOn(int _KeyId, float _Velocity);
	void NoteOff(int _KeyId);
	void Render(long _SampleNr);
	void SetOscillator(SO_WaveForms _Wave, SO_WaveForms _MorphWave, int _Index);
	float GetADSRValue(SO_Note * _Note, float _Time);
};

//_________________________________________________
class SynthOl : public MIDIListener
{
	SO_Waveform					m_WaveTab[SO_Wave_Max];
	SO_SoundSource				*m_SourceTab[SO_SOURCE_NR];
	int							m_SourceAllocIndex;

public:
	SO_StereoSoundBuf			m_OutBuf;
	bool						m_InitDone;

	SynthOl();
	void Init();
	void Render(DWORD SamplesToRender);

	void NoteOn(int _Channel, int _KeyId, float _Velocity);
	void NoteOff(int _Channel, int _KeyId);

	SO_SoundSource * AddSource(SO_SourceType _SourceType);
	SO_Waveform * GetWaveForm(SO_WaveForms _WaveForm) {	return &m_WaveTab[_WaveForm];	}
	void ClearOutBuffers(DWORD SamplesToRender);
	void PopOutputVal(float & _Left, float & _Right);
};

#endif