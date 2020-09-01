#include "SynthOl.h"
#include <tuple>
#include <algorithm>

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
void FloatClear(float * Dest, long len) { std::memset(Dest, 0, len*sizeof(float)); }

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

Synth::Synth()
{
	m_WaveTab[(int)WaveType::Square].WaveformSquare(PlaybackFreq, 1.f, false);
	m_WaveTab[(int)WaveType::Square_Soft].WaveformSquare(PlaybackFreq, 1.f, true);
	m_WaveTab[(int)WaveType::Saw].WaveformSaw(PlaybackFreq, 1.f, false);
	m_WaveTab[(int)WaveType::Saw_Soft].WaveformSaw(PlaybackFreq, 1.f, true);
	m_WaveTab[(int)WaveType::RampUp].WaveformRamp(PlaybackFreq, 1.f, false);
	m_WaveTab[(int)WaveType::RampUp_Soft].WaveformRamp(PlaybackFreq, 1.f, true);
	m_WaveTab[(int)WaveType::Rand].WaveformRand(PlaybackFreq, 1.f, false);
}

//-----------------------------------------------------
void Synth::Render(unsigned long SamplesToRender)
{
	// clear out buffers
	for(auto It = std::end(m_SourceTab)-1; It >= std::begin(m_SourceTab); --It)
		(*It)->GetDest().Clear(SamplesToRender);

	// render source buffers en reverse
	for(auto It = std::end(m_SourceTab)-1; It >= std::begin(m_SourceTab); --It)
		(*It)->Render(SamplesToRender);
}

//-----------------------------------------------------
void Synth::PopOutputVal(float & Left, float & Right)
{
	std::tie(Left, Right) = m_OutBuf.m_Data[m_OutBuf.m_WriteCursor];
	Left = std::clamp(Left, 0.f, 1.f);
	Right = std::clamp(Right, 0.f, 1.f);
	m_OutBuf.m_WriteCursor = (m_OutBuf.m_WriteCursor + 1) % m_OutBuf.m_Data.size(); 
}

//-----------------------------------------------------
void Synth::NoteOn(int _Channel, int _KeyId, float _Velocity)
{
	for(auto & Source : m_SourceTab)
		Source->NoteOn(_Channel, _KeyId, _Velocity);
}

//-----------------------------------------------------
void Synth::NoteOff(int _Channel, int _KeyId)
{
	for(auto & Source : m_SourceTab)
		Source->NoteOff(_Channel, _KeyId);
}

};
