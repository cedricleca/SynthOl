#include "SynthOl.h"

namespace SynthOl
{

	//-----------------------------------------------------
void SampleSource::Render(long _SampleNr)
{
	long wc = m_OutBuf->m_WriteCursor;

	for(long i = 0; i < _SampleNr; i++)
	{
		float Val = m_SrcWaveForm->m_Wave[int(m_Cursor)];
		m_OutBuf->m_Left[wc] +=  Val * m_Volume;
		m_OutBuf->m_Right[wc] += Val * m_Volume;
		m_Cursor += m_Step;
		wc++;
		
		if(wc >= m_OutBuf->m_Size)
			wc = 0;

		if(int(m_Cursor) >= m_SrcWaveForm->m_Size)
		{
			if(m_bLoop)
				m_Cursor -= m_SrcWaveForm->m_Size;
			else
				m_Cursor -= m_Step;
		}
	}
}

};
