#include "SynthOl.h"

namespace SynthOl
{

	//-----------------------------------------------------
void SampleSource::Render(long _SampleNr)
{
	long & wc = m_Dest->m_WriteCursor;

	for(long i = 0; i < _SampleNr; i++)
	{
		float Val = m_SrcWaveForm->m_Data[unsigned int(m_Cursor)];
		m_Dest->m_Data[wc].first +=  Val * m_Volume;
		m_Dest->m_Data[wc].second += Val * m_Volume;
		wc = (wc + 1) % m_Dest->m_Data.size();

		m_Cursor += m_Step;
		if(unsigned int(m_Cursor) >= m_SrcWaveForm->m_Data.size())
			m_Cursor -= m_bLoop ? m_SrcWaveForm->m_Data.size() : m_Step;
	}
}

};
