#include "SynthOl.h"


namespace SynthOl
{

float OctaveFreq[] = 
{
	261.6255653006f,
	277.1826309769f,
	293.6647679174f,
	311.1269837221f,
	329.6275569129f,
	349.2282314330f,
	369.9944227116f,
	391.9954359817f,
	415.3046975799f,
	440.0000000000f,
	466.1637615181f,
	493.8833012561f,
};


//-----------------------------------------------------------------------------
void FloatClear(float * Dest, long len)
{
	for(long i = 0; i < len; i++)
		Dest[i] = 0.0f;
}

//-----------------------------------------------------
void MemClear(unsigned char * Dest, long len)
{
	for(long i = 0; i < len; i++)
		Dest[i] = 0;
}

//-----------------------------------------------------
void FloatClamp01(float & _Val)
{
	_Val = ((_Val > 1.0f)? 1.0f : ((_Val < 0.0f)? 0.0f : _Val) );
}

//-----------------------------------------------------
float GetNoteFreq(int _NoteCode)
{
	float Freq = OctaveFreq[_NoteCode%12];
	while(_NoteCode < 60)	{	Freq *= 0.5f;	_NoteCode += 12;	}
	while(_NoteCode > 71)	{	Freq *= 2.0f;	_NoteCode -= 12;	}
	return Freq;
}

//-----------------------------------------------------
float Distortion(float _Gain, float _Sample)
{
//	float absx = (_Sample<0.0f) ? -_Sample: _Sample;
//	_Sample = _Sample*(absx + _Gain)/(_Sample*_Sample + (_Gain-1.0f)*absx + 1.0f);

	_Sample *= 1.0f + _Gain;
	_Sample = 1.5f*_Sample - 0.5f*_Sample*_Sample*_Sample;

	/*
	while(_Sample > 1.0f || _Sample < -1.0f)
	{
		if(_Sample > 0.0f)
			_Sample = 2.0f - _Sample;
		else
			_Sample = -2.0f - _Sample;
	}
	*/

/*
#define DIST_LIMIT (0.65f)
	if(_Sample > 1.0f)
	{
		while(_Sample > 1.0f || _Sample < DIST_LIMIT)
		{
			if(_Sample > 1.0f)
				_Sample = 2.0f - _Sample;
			if(_Sample < DIST_LIMIT)
				_Sample = DIST_LIMIT + DIST_LIMIT - _Sample;
		}
	}
	else if(_Sample < -1.0f)
	{
		while(_Sample < -1.0f || _Sample > -DIST_LIMIT)
		{
			if(_Sample < -1.0f)
				_Sample = -2.0f - _Sample;
			if(_Sample > -DIST_LIMIT)
				_Sample = -DIST_LIMIT - DIST_LIMIT - _Sample;
		}
	}
*/
	return _Sample;
}

//-----------------------------------------------------
Synth::Synth()
{
	m_SourceAllocIndex = 0;
	m_InitDone = false;
}

//-----------------------------------------------------
SoundSource * Synth::AddSource(SourceType _SourceType)
{
	SoundSource * ret = nullptr;

	if(m_SourceAllocIndex >= SOURCE_NR)
		return ret;

	switch(_SourceType)
	{
	case SourceType::Sample:	
		ret = m_SourceTab[m_SourceAllocIndex++] = new SampleSource;
		break;

	case SourceType::Analog:	
		ret = m_SourceTab[m_SourceAllocIndex++] = new AnalogSource;	
		break;

	case SourceType::EchoFilter:	
		ret = m_SourceTab[m_SourceAllocIndex++] = new EchoFilterSource;	
		break;
	}

	return ret;
}

//-----------------------------------------------------
void Synth::Init()
{	
	m_OutBuf.Allocate(PLAYBACK_FREQ);

	// Init la waveTable
	for(int i = 0; i < (int)Wave::Max; i++)
		m_WaveTab[i].Allocate(PLAYBACK_FREQ);

	for(int i = (int)Wave::Square; i < (int)Wave::Square_Soft; i++)
	{
		m_WaveTab[i].GenerateWave((Wave)i, 0, PLAYBACK_FREQ, PLAYBACK_FREQ, 1.0f);

		if(i != (int)Wave::Rand)
			m_WaveTab[i].Soften(0, PLAYBACK_FREQ, 0.005f);

		m_WaveTab[i].Normalize(0, PLAYBACK_FREQ, 1.0f);
	}
		
	for(int i = (int)Wave::Square_Soft; i < (int)Wave::Max; i++)
	{
		m_WaveTab[i].GenerateWave((Wave)i, 0, PLAYBACK_FREQ, PLAYBACK_FREQ, 1.0f);

		for(int j = 0; j < 5; j++)
		{
			m_WaveTab[i].Soften(0, PLAYBACK_FREQ, 0.001f);
			m_WaveTab[i].Normalize(0, PLAYBACK_FREQ, 1.0f);
		}
	}

	m_InitDone = true;
}

//-----------------------------------------------------
void Synth::Render(unsigned long SamplesToRender)
{
	if(!m_InitDone)
		return;

	// clear out buffers
	ClearOutBuffers(SamplesToRender);

	// render source buffers en reverse
	for(int i = m_SourceAllocIndex-1; i >= 0; i--)
		m_SourceTab[i]->Render(SamplesToRender);
}

//-----------------------------------------------------
void Synth::ClearOutBuffers(unsigned long SamplesToRender)
{
	for(int i = m_SourceAllocIndex-1; i >= 0; i--)
	{
		StereoSoundBuf * Buf = m_SourceTab[i]->GetOutBuf();
		Buf->Clear(Buf->m_WriteCursor, SamplesToRender);
	}
}

//-----------------------------------------------------
void Synth::PopOutputVal(float & _Left, float & _Right)
{
	_Left  = m_OutBuf.m_Left[m_OutBuf.m_WriteCursor];
	_Right = m_OutBuf.m_Right[m_OutBuf.m_WriteCursor];

	if(_Left > 1.0f)		_Left = 1.0f;
	else if(_Left < -1.0f)	_Left = -1.0f;						

	if(_Right > 1.0f)		_Right = 1.0f;
	else if(_Right < -1.0f)	_Right = -1.0f;						

	m_OutBuf.m_WriteCursor++;
	if(m_OutBuf.m_WriteCursor >= m_OutBuf.m_Size)
		m_OutBuf.m_WriteCursor -= m_OutBuf.m_Size;
}

//-----------------------------------------------------
void Synth::NoteOn(int _Channel, int _KeyId, float _Velocity)
{
	for(int i = 0; i < m_SourceAllocIndex; i++)
		m_SourceTab[i]->NoteOn(_Channel, _KeyId, _Velocity);
}

//-----------------------------------------------------
void Synth::NoteOff(int _Channel, int _KeyId)
{
	for(int i = 0; i < m_SourceAllocIndex; i++)
		m_SourceTab[i]->NoteOff(_Channel, _KeyId);
}

};
