
#pragma once

#include "MIDIListener.h"

namespace SynthOl
{
	enum class LFODest : char
	{
		Tune,
		Morph,
		Distort,
		Volume,
		Max
	};

	#define ANALOGSOURCE_OSCILLATOR_NR		2
	#define ANALOGSOURCE_LOWFREQ_OSCILLATOR_NR	(ANALOGSOURCE_OSCILLATOR_NR*int(LFODest::Max))
	#define SOURCE_NR					16
	#define PLAYBACK_FREQ				44100
	#define ANALOGSOURCE_POLYPHONY_NOTE_NR	3
	#define PRIMARY_BUFFER_SIZE  (PLAYBACK_FREQ*2)

	enum class Wave : char
	{
		Square,
		Saw,
		RampUp,
		Rand,
		Square_Soft,
		Saw_Soft,
		RampUp_Soft,
		Max,
	};

	enum class SourceType
	{
		Sample,
		Analog,
		EchoFilter,
	};

	enum class ModulationType
	{
		Mix,
		Mul,
		Ring,
		Max,
	};

	enum class PolyphonyMode : char
	{
		Poly,
		Arpeggio,
		Portamento,
	};

	extern float OctaveFreq[];
	class Synth;

	void FloatClear(float * Dest, long len);
	void MemClear(unsigned char * Dest, long len);
	float Distortion(float _Gain, float _Sample);
	void FloatClamp01(float & _Val);
	float GetNoteFreq(int _NoteCode);

	//_________________________________________________
	class SoundBuf
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
	class StereoSoundBuf : public SoundBuf 
	{
	public:
		float * m_Left;
		float * m_Right;

		void Allocate(long _Size);
		void Clear(long _Index, long _Size);
	};

	//_________________________________________________
	class Waveform : public SoundBuf 
	{
	public:
		float * m_Wave;

		void Allocate(long _Size);
		void GenerateWave(Wave _Type, long _Index, long _Period, long _Size, float _Magnitude);
		void Soften(long _Index, long _Size, float _Coef);
		void Normalize(long _Index, long _Size, float _Coef);
		void Clear(long _Index, long _Size);
		void Copy(Waveform * _Src, long _SrcIndex, long _DstIndex, long _Size);
		void operator *= (float _Coef);
	};

	//_________________________________________________
	class SoundSource
	{
	protected:
		StereoSoundBuf	* m_OutBuf;
		Synth				* m_Synth;
		int					m_Channel;

	public:
		virtual void Init(StereoSoundBuf * _OutBuf, Synth * _Synth, int _Channel)
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
		virtual StereoSoundBuf * GetOutBuf(){ return m_OutBuf; }
	};

	//_________________________________________________
	class FilterSource:public SoundSource
	{
	protected:
		StereoSoundBuf		m_SrcWaveForm;

	public:
		long			m_Cursor;

		virtual void Init(StereoSoundBuf * _OutBuf, Synth * _Synth, int _Channel);
		virtual void Render(long _SampleNr){}
		virtual StereoSoundBuf * GetSrcWaveForm();
	};

	//_________________________________________________
	class EchoFilterSource:public FilterSource
	{
		StereoSoundBuf		m_DelayWaveForm;
		float			m_S0;
		float			m_S1;

	public:
		long	m_DelayLen;
		float	m_Feedback;
		long	m_ResoDelayLen;
		float	m_ResoFeedback;
		long	m_ResoSteps;

		virtual void Init(long _DelayLen, StereoSoundBuf * _OutBuf, Synth * _Synth, int _Channel);
		virtual void Render(long _SampleNr);
	};

	//_________________________________________________
	class SampleSource:public SoundSource
	{
		Waveform		* m_SrcWaveForm;

	public:
		bool			m_bLoop;
		float			m_Volume;
		float			m_Step;
		float			m_Cursor;

		void Init(Waveform * _SrcWaveForm, StereoSoundBuf * _OutBuf, Synth * _Synth, int _Channel){	m_SrcWaveForm = _SrcWaveForm; m_Step = 1.0f; SoundSource::Init(_OutBuf, _Synth, _Channel); }
		void NoteOn(int _KeyId, float _Velocity){	m_Cursor = 0; }
		void Render(long _SampleNr);
	};

	//_________________________________________________
	class Note
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
			if(!m_NoteOn)
			{
				m_Time = 0.0f; 
				m_SustainTime = 0.0f; 
				m_NoteOn = true; 
				m_Code = _KeyId;
				m_Velocity = _Velocity;
				m_Died = false;
			}
		}
		void NoteOff(){ m_NoteOn = false; }
	};

	//_________________________________________________
	struct LFOData
	{
		float				m_Delay;
		float				m_Attack;
		float				m_Magnitude;
		float				m_Rate;
		float				m_BaseValue;
		Wave				m_WF;
		char				m_NoteSync;
	};

	//_________________________________________________
	class LFO
	{
		Waveform			* m_SrcWaveForm;
		float				m_Cursor;
		float				m_CurVal;
		Synth				* m_Synth;

	public:
		LFOData	* m_Data;

		void Init(Synth * _Synth, LFOData	* _Data);
		void Update(float _FrameTime);
		float GetValue(float _NoteTime, bool _ZeroCentered=false);
		void SetOscillator(Wave _Wave);
		void NoteOn();
	};


	//_________________________________________________
	struct OscillatorData
	{
		char			m_OctaveOffset;
		char			m_NoteOffset;	
		Wave			m_WF0;
		Wave			m_WF1;
		ModulationType	m_ModulationType;
	};

	//_________________________________________________
	struct AnalogSourceData
	{
		OscillatorData			m_OscillatorTab[ANALOGSOURCE_OSCILLATOR_NR];
		LFOData					m_LowFreqOscillatorTab[ANALOGSOURCE_LOWFREQ_OSCILLATOR_NR];
		float					m_ADSR_Attack;
		float					m_ADSR_Decay;
		float					m_ADSR_Sustain;
		float					m_ADSR_Release;
		float					m_LeftVolume;
		float					m_RightVolume;
		float					m_PortamentoTime;
		PolyphonyMode			m_PolyphonyMode;
	};

	//_________________________________________________
	class AnalogSource:public SoundSource
	{
		//_________________________________________________
		struct OscillatorInterp
		{
			float		m_Volume;
			float		m_DistortGain;
			float		m_Morph;
			float		m_Cursor;
		};

		//_________________________________________________
		struct OscillatorTransients
		{
			Waveform	* m_SrcWaveForm;
			Waveform	* m_SrcMorphWaveForm;
			float		m_VolumeInterp;
			float		m_DistortGainInterp;
			float		m_MorphInterp;
			float		m_Volume;
			float		m_Step;
			float		m_StepShift;
			float		m_Morph;
			float		m_DistortGain;
		};

		//_________________________________________________
		struct Transients
		{
			OscillatorTransients	m_OscillatorTab[ANALOGSOURCE_OSCILLATOR_NR];
			Note					m_NoteTab[ANALOGSOURCE_POLYPHONY_NOTE_NR];
			OscillatorInterp		m_InterpTab[ANALOGSOURCE_POLYPHONY_NOTE_NR][ANALOGSOURCE_OSCILLATOR_NR];
			int						m_ArpeggioIdx;
			float					m_PortamentoCurFreq;
			float					m_PortamentoStep;
		};

	public:
		AnalogSourceData	* m_Data;
		LFO					m_LowFreqOscillatorTab[ANALOGSOURCE_LOWFREQ_OSCILLATOR_NR];
		Transients			m_Transients;

		void Init(StereoSoundBuf * _OutBuf, Synth * _Synth, AnalogSourceData * _Data, int _Channel);
		void NoteOn(int _KeyId, float _Velocity);
		void NoteOff(int _KeyId);
		void Render(long _SampleNr);
		void SetOscillator(Wave _Wave, Wave _MorphWave, int _Index);
		float GetADSRValue(Note * _Note, float _Time);
	};

	//_________________________________________________
	class Synth : public MIDIListener
	{
		Waveform				m_WaveTab[int(Wave::Max)];
		SoundSource				*m_SourceTab[SOURCE_NR];
		int						m_SourceAllocIndex;

	public:
		StereoSoundBuf			m_OutBuf;
		bool					m_InitDone;

		Synth();
		void Init();
		void Render(unsigned long SamplesToRender);

		void NoteOn(int _Channel, int _KeyId, float _Velocity);
		void NoteOff(int _Channel, int _KeyId);

		SoundSource * AddSource(SourceType _SourceType);
		Waveform * GetWaveForm(Wave _WaveForm) {	return &m_WaveTab[int(_WaveForm)];	}
		void ClearOutBuffers(unsigned long SamplesToRender);
		void PopOutputVal(float & _Left, float & _Right);
	};

}; // namespace SynthOl
