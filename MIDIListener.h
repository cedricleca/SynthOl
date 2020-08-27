#ifndef __MIDIListener__
#define __MIDIListener__

class MIDIListener
{
public:
	virtual void NoteOn(int _Channel, int _KeyId, float _Velocity) = 0;
	virtual void NoteOff(int _Channel, int _KeyId) = 0;
};

#endif

