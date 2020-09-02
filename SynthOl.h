
#pragma once

#include "MIDIListener.h"
#include <vector>
#include <memory>
#include <array>
#include <utility>
#include <map>

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

	static const unsigned int PlaybackFreq = 44100;
	static const int PrimaryBufferSize = PlaybackFreq*2;

	enum class WaveType : char
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
	float Distortion(float _Gain, float _Sample);
	float GetNoteFreq(int _NoteCode);

	//_________________________________________________
	template <class DataType = float, size_t Size = 16>
	struct SoundBuf
	{
		std::vector<DataType>	m_Data;
		long					m_WriteCursor = 0;

		SoundBuf()	{ m_Data.resize(Size); }
	};

	struct StereoSoundBuf : SoundBuf<std::pair<float, float>, PlaybackFreq>
	{
		void Clear(long NbSamples)
		{
			for(int i = 0; i < NbSamples; i++)
			{
				m_Data[m_WriteCursor] = {0.f, 0.f};
				m_WriteCursor = (m_WriteCursor + 1) % PlaybackFreq;
			}
		}
	};

	//_________________________________________________
	class Waveform : public SoundBuf<float, PlaybackFreq> 
	{
	public:
		void Soften(long Index, long Size, float Coef);
		void Normalize(long Index, long Size, float Coef);
		void operator *= (float Coef);
	};

	Waveform WaveformSquare(bool Soft);
	Waveform WaveformSaw(bool Soft);
	Waveform WaveformRamp(bool Soft);
	Waveform WaveformRand(bool Soft);

	//_________________________________________________
	class SoundSource
	{
	protected:
		StereoSoundBuf	* m_Dest;
		Synth			* m_Synth;
		int				m_Channel = 0;

	public:
		SoundSource(StereoSoundBuf * Dest, Synth * Synth, int Channel) : m_Dest(Dest), m_Synth(Synth), m_Channel(Channel)
		{}

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
		virtual void NoteOn(int _KeyId, float _Velocity) = 0;
		virtual void NoteOff(int _KeyId) = 0;
		virtual void Render(long _SampleNr) = 0;
		virtual StereoSoundBuf & GetDest(){ return *m_Dest; }
	};

	//_________________________________________________
	class FilterSource : public SoundSource
	{
	protected:
		StereoSoundBuf	m_SrcWaveForm;

	public:
		long			m_Cursor;
	};

	//_________________________________________________
	template <size_t Size = 16>
	class EchoFilterSource : public FilterSource
	{
		SoundBuf<std::pair<float, float>, Size>	m_DelayWaveForm;
		float			m_S0 = 0.f;
		float			m_S1 = 0.f;

	public:
		long	m_DelayLen = 0;
		long	m_ResoDelayLen = 0;
		long	m_ResoSteps;
		float	m_ResoFeedback = 0.f;
		float	m_Feedback = 0.f;

		EchoFilterSource(long DelayLen, SoundSource * Dest, Synth * Synth, int Channel) : 
			FilterSource(Dest, Synth, Channel)
		{}

		virtual void Render(long _SampleNr) override;
	};

	//_________________________________________________
	class SampleSource : public SoundSource
	{
		const Waveform	* m_SrcWaveForm;

	public:
		bool			m_bLoop = true;
		float			m_Volume = 0.0f;
		float			m_Step = 1.0f;
		float			m_Cursor = 0.0f;

		SampleSource(StereoSoundBuf * Dest, Synth * Synth, int Channel, const Waveform * SrcWaveForm) :
			SoundSource(Dest, Synth, Channel),
			m_SrcWaveForm(SrcWaveForm)
		{}

		void NoteOn(int KeyId, float Velocity) override { m_Cursor = 0; }
		void NoteOff(int _KeyId) override {};
		void Render(long SampleNr) override;
	};

	//_________________________________________________
	class Note
	{
	public:
		float	m_Time = 0.f;
		float	m_SustainTime = 0.f;
		float	m_Velocity = 0.f;
		int		m_Code = 0;
		bool	m_Died = true;
		bool	m_NoteOn = false;

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
		float				m_Delay = .0f;
		float				m_Attack = .0f;
		float				m_Magnitude = .0f;
		float				m_Rate = .0f;
		float				m_BaseValue = .0f;
		WaveType			m_WF = WaveType::Square;
		char				m_NoteSync = 0;
	};

	//_________________________________________________
	class LFO
	{
		const Waveform		* m_SrcWaveForm = nullptr;
		float				m_Cursor = .0f;
		float				m_CurVal = .0f;
		Synth				* m_Synth = nullptr;

	public:
		LFOData	* m_Data = nullptr;

		void Init(Synth * _Synth, LFOData	* _Data);
		void Update(float _FrameTime);
		float GetValue(float _NoteTime, bool _ZeroCentered=false);
		void SetOscillator(WaveType _Wave);
		void NoteOn();
	};


	//_________________________________________________
	struct OscillatorData
	{
		char			m_OctaveOffset = 0;
		char			m_NoteOffset = 0;	
		WaveType		m_WF0 = WaveType::Square;
		WaveType		m_WF1 = WaveType::Square;
		ModulationType	m_ModulationType = ModulationType::Mul;
	};

	static const int AnalogsourceOscillatorNr = 2;
	static const int AnalogsourcePolyphonyNoteNr = 3;

	//_________________________________________________
	struct AnalogSourceData
	{
		OscillatorData			m_OscillatorTab[AnalogsourceOscillatorNr];
		LFOData					m_LFOTab[AnalogsourceOscillatorNr][int(LFODest::Max)];
		float					m_ADSR_Attack = 0.f;
		float					m_ADSR_Decay = 0.f;
		float					m_ADSR_Sustain = 0.f;
		float					m_ADSR_Release = 0.f;
		float					m_LeftVolume = 0.f;
		float					m_RightVolume = 0.f;
		float					m_PortamentoTime = 0.f;
		PolyphonyMode			m_PolyphonyMode = PolyphonyMode::Poly;
	};

	//_________________________________________________
	class AnalogSource : public SoundSource
	{
		struct OscillatorInterp
		{
			float		m_Volume = 0.f;
			float		m_DistortGain = 0.f;
			float		m_Morph = 0.f;
			float		m_Cursor = 0.f;
		};

		struct OscillatorTransients
		{
			const Waveform	* m_SrcWaveForm = nullptr;
			const Waveform	* m_SrcMorphWaveForm = nullptr;
			float			m_VolumeInterp = 0.f;
			float			m_DistortGainInterp = 0.f;
			float			m_MorphInterp = 0.f;
			float			m_Volume = 0.f;
			float			m_Step = 0.f;
			float			m_StepShift = 0.f;
			float			m_Morph = 0.f;
			float			m_DistortGain = 0.f;
		};

		struct Transients
		{
			OscillatorTransients	m_OscillatorTab[AnalogsourceOscillatorNr];
			Note					m_NoteTab[AnalogsourceOscillatorNr];
			OscillatorInterp		m_InterpTab[AnalogsourcePolyphonyNoteNr][AnalogsourceOscillatorNr];
			int						m_ArpeggioIdx = 0;
			float					m_PortamentoCurFreq = 0.f;
			float					m_PortamentoStep = 0.f;
		};

	public:
		AnalogSourceData	* m_Data;
		LFO					m_LFOTab[AnalogsourceOscillatorNr][int(LFODest::Max)];
		Transients			m_Transients;

		AnalogSource(StereoSoundBuf * Dest, Synth * Synth, int Channel, AnalogSourceData * Data);
		void NoteOn(int _KeyId, float _Velocity) override;
		void NoteOff(int _KeyId) override;
		void Render(long _SampleNr) override;
		void SetOscillator(WaveType _Wave, WaveType _MorphWave, int _Index);
		float GetADSRValue(Note * _Note, float _Time);
	};

	//_________________________________________________
	class Synth : public MIDIListener
	{
		std::vector<SoundSource *>					m_SourceTab;
		std::array<Waveform, int(WaveType::Max)>	m_WaveTab;

	public:
		StereoSoundBuf								m_OutBuf;

		Synth();
		void Render(unsigned int SamplesToRender);
		void NoteOn(int _Channel, int _KeyId, float _Velocity);
		void NoteOff(int _Channel, int _KeyId);
		void AddSource(SoundSource & NewSource) { m_SourceTab.push_back(&NewSource); }
		const Waveform & GetWaveForm(WaveType Type) const { return m_WaveTab[int(Type)]; }
		void PopOutputVal(float & OutLeft, float & OutRight);
		void PopOutputVal(short & OutLeft, short & OutRight);
	};

}; // namespace SynthOl
