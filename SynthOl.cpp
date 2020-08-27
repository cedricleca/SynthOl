#include "SynthOl.h"


float SO_OctaveFreq[] = 
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
float SO_GetNoteFreq(int _NoteCode)
{
	float Freq = SO_OctaveFreq[_NoteCode%12];
	while(_NoteCode < 60)	{	Freq *= 0.5f;	_NoteCode += 12;	}
	while(_NoteCode > 71)	{	Freq *= 2.0f;	_NoteCode -= 12;	}
	return Freq;
}

//-----------------------------------------------------
float SO_Distortion(float _Gain, float _Sample)
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
SynthOl::SynthOl()
{
	m_SourceAllocIndex = 0;
	m_InitDone = false;
}

//-----------------------------------------------------
SO_SoundSource * SynthOl::AddSource(SO_SourceType _SourceType)
{
	SO_SoundSource * ret = NULL;

	if(m_SourceAllocIndex >= SO_SOURCE_NR)
		return ret;

	switch(_SourceType)
	{
	case SO_SourceType_Sample:	
		ret = m_SourceTab[m_SourceAllocIndex++] = new SO_SampleSource;
		break;

	case SO_SourceType_Analog:	
		ret = m_SourceTab[m_SourceAllocIndex++] = new SO_AnalogSource;	
		break;

	case SO_SourceType_EchoFilter:	
		ret = m_SourceTab[m_SourceAllocIndex++] = new SO_EchoFilterSource;	
		break;
	}

	return ret;
}

//-----------------------------------------------------
void SynthOl::Init()
{	
	m_OutBuf.Allocate(SO_PLAYBACK_FREQ);

	// Init la waveTable
	for(int i = 0; i < SO_Wave_Max; i++)
		m_WaveTab[i].Allocate(SO_PLAYBACK_FREQ);

	for(int i = SO_Wave_Square; i < SO_Wave_Square_Soft; i++)
	{
		m_WaveTab[i].GenerateWave((SO_WaveForms)i, 0, SO_PLAYBACK_FREQ, SO_PLAYBACK_FREQ, 1.0f);

		if(i != SO_Wave_Rand)
			m_WaveTab[i].Soften(0, SO_PLAYBACK_FREQ, 0.005f);

		m_WaveTab[i].Normalize(0, SO_PLAYBACK_FREQ, 1.0f);
	}
		
	for(int i = SO_Wave_Square_Soft; i < SO_Wave_Max; i++)
	{
		m_WaveTab[i].GenerateWave((SO_WaveForms)i, 0, SO_PLAYBACK_FREQ, SO_PLAYBACK_FREQ, 1.0f);

		for(int j = 0; j < 5; j++)
		{
			m_WaveTab[i].Soften(0, SO_PLAYBACK_FREQ, 0.001f);
			m_WaveTab[i].Normalize(0, SO_PLAYBACK_FREQ, 1.0f);
		}
	}

	m_InitDone = true;
}

//-----------------------------------------------------
void SynthOl::Render(DWORD SamplesToRender)
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
void SynthOl::ClearOutBuffers(DWORD SamplesToRender)
{
	for(int i = m_SourceAllocIndex-1; i >= 0; i--)
	{
		SO_StereoSoundBuf * Buf = m_SourceTab[i]->GetOutBuf();
		Buf->Clear(Buf->m_WriteCursor, SamplesToRender);
	}
}

//-----------------------------------------------------
void SynthOl::PopOutputVal(float & _Left, float & _Right)
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
void SynthOl::NoteOn(int _Channel, int _KeyId, float _Velocity)
{
	for(int i = 0; i < m_SourceAllocIndex; i++)
		m_SourceTab[i]->NoteOn(_Channel, _KeyId, _Velocity);
}

//-----------------------------------------------------
void SynthOl::NoteOff(int _Channel, int _KeyId)
{
	for(int i = 0; i < m_SourceAllocIndex; i++)
		m_SourceTab[i]->NoteOff(_Channel, _KeyId);
}


