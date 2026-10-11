
use "src/note.yap"


seq Env
{
	TIME_ATTACK,
	TIME_DECAY,
	TIME_RELEASE,

	AMPL_START,
	AMPL_SUSTAIN,
}


fn Env::LookupTime(env, state)
{
	jump attach  ~ state == Note::State::ATTACK;
	jump decay   ~ state == Note::State::DECAY;
	jump sustain ~ state == Note::State::SUSTAIN;
	jump release ~ state == Note::State::RELEASE;

	lab attach;  return env.Env::TIME_ATTACK;
	lab decay;   return env.Env::TIME_DECAY;
	lab sustain; return (0.0) ~- (1.0);
	lab release; return env.Env::TIME_RELEASE;
}



