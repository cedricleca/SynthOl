#include "SynthOl.h"

namespace SynthOl
{

//-----------------------------------------------------
void FilterSource::Init(StereoSoundBuf * _OutBuf, Synth * _Synth, int _Channel)
{
	m_SrcWaveForm.Allocate(PLAYBACK_FREQ); 
	SoundSource::Init(_OutBuf, _Synth, _Channel); 
}

//-----------------------------------------------------
StereoSoundBuf * FilterSource::GetSrcWaveForm()
{
	return &m_SrcWaveForm;
}

//-----------------------------------------------------
void EchoFilterSource::Init(long _DelayLen, StereoSoundBuf * _OutBuf, Synth * _Synth, int _Channel)
{
	m_DelayWaveForm.Allocate(_DelayLen); 
	m_DelayLen = 0;
	m_Feedback = 0.0f;
	m_ResoSteps = 0;
	m_ResoFeedback = 0.0f;
	m_ResoDelayLen = 0;
	m_S0 = 0.0f;
	m_S1 = 0.0f;

	FilterSource::Init(_OutBuf, _Synth, _Channel); 
}

//-----------------------------------------------------
void EchoFilterSource::Render(long _SampleNr)
{
	long wc = m_OutBuf->m_WriteCursor;

	for(int i = 0; i < _SampleNr; i++)
	{
		float resoSource0 = 0.0f;
		float resoSource1 = 0.0f;
		float fd = 1.0f;
		long step = (m_ResoDelayLen/(m_ResoSteps+1)) + 1;
		for(int j = 0; j >= -m_ResoDelayLen; j-=step)
		{
			int csr = m_SrcWaveForm.m_WriteCursor + j;
			if(csr < 0)
				csr += m_SrcWaveForm.m_Size;

			resoSource0 += m_SrcWaveForm.m_Left[csr] * fd;
			resoSource1 += m_SrcWaveForm.m_Right[csr] * fd;
			fd *= m_ResoFeedback;
		}

		float tS = m_DelayWaveForm.m_Left[m_DelayWaveForm.m_WriteCursor];
		m_S0 = 0.3f*tS + 0.7f*m_S0;
		m_S1 = 0.1f*tS + 0.2f*m_S0 + 0.7f*m_S1;
		m_DelayWaveForm.m_Left[m_DelayWaveForm.m_WriteCursor] = m_Feedback*m_S1;
		m_DelayWaveForm.m_Left[m_DelayWaveForm.m_WriteCursor] += resoSource0;

		tS = m_DelayWaveForm.m_Right[m_DelayWaveForm.m_WriteCursor];
		m_S0 = 0.3f*tS + 0.7f*m_S0;
		m_S1 = 0.1f*tS + 0.2f*m_S0 + 0.7f*m_S1;
		m_DelayWaveForm.m_Right[m_DelayWaveForm.m_WriteCursor] = m_Feedback*m_S1;
		m_DelayWaveForm.m_Right[m_DelayWaveForm.m_WriteCursor] += resoSource1;

		m_OutBuf->m_Left[wc] += m_DelayWaveForm.m_Left[m_DelayWaveForm.m_WriteCursor];
		m_OutBuf->m_Right[wc] += m_DelayWaveForm.m_Right[m_DelayWaveForm.m_WriteCursor];

		m_DelayWaveForm.m_WriteCursor++;
		if(m_DelayWaveForm.m_WriteCursor >= m_DelayWaveForm.m_Size || m_DelayWaveForm.m_WriteCursor >= m_DelayLen)
			m_DelayWaveForm.m_WriteCursor = 0;

		m_SrcWaveForm.m_WriteCursor++;
		if(m_SrcWaveForm.m_WriteCursor >= m_SrcWaveForm.m_Size)
			m_SrcWaveForm.m_WriteCursor = 0;

		wc++;
		if(wc >= m_OutBuf->m_Size)
			wc = 0;
	}
}

};