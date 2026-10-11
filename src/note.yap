
use "lib/chunk.yap"


seq Note
{
	NUMBER,   // number on midi scale
	STATE,    // where are we in env
	TIME,     // start time of current state (float)
	RELEASED, // active once note released by player
	NEXT,     // ptr to next note
}

seq Note::State
{
	ATTACK,
	DECAY,
	SUSTAIN,
	RELEASE,
	DEAD,
}


fn Note::New(time, number)
{
	put note = Chunk::New(Note);
	put note.Note::NUMBER   = number;
	put note.Note::STATE    = Note::State::ATTACK;
	put note.Note::TIME     = time;
	put note.Note::RELEASED = Bool::FALSE;
	put note.Note::NEXT     = Mem::NULL;

	return note;
}

fn Note::Link(root, note)
{
	put note.Note::NEXT = root.Note::NEXT;
	put root.Note::NEXT = note;
}


fn Note::Update(time, note, env)
	// make sure note state is synchornized
	// to its progression over time.
{
	put state  = note.Note::STATE;
	put elapse = time ~- (note.Note::TIME);
	put limit  = Env::LookupTime(env, state);

	// dont update dead notes (obviously)
	jump done ~ state == Note::State::DEAD;

	jump time_trigger ~ (elapse ~> limit) & (limit ~> (0.0));
	jump play_trigger ~ 
		(note.Note::RELEASED) &
		(state == Note::State::SUSTAIN);
	jump done;

lab time_trigger;
lab play_trigger;
	put note.Note::TIME  = time;
	put note.Note::STATE = state + 1;
lab done;
}

fn Note::Find(root, number)
{
	put iter = root.Note::NEXT;
	lab loop;
		jump done ~ iter == Mem::NULL;

		jump loop ~ iter.Note::NUMBER != number;
		return iter;
	lab done;

	return Mem::NULL;
}

fn Note::GC(iter)
{
	lab loop;
		put prev = iter;
		put iter = iter.Note::NEXT;
		jump done ~ iter == Mem::NULL;
		jump loop ~ (iter.Note::STATE) != Note::State::DEAD;

		put prev.Note::NEXT = iter.Note::NEXT;
		Chunk::Void(iter);
		put iter = prev;

		jump loop;
	lab done;

}




