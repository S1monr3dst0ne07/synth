
use "src/note.yap"
use "src/env.yap"
use "src/osc.yap"

fn Synth::ScaleToFreq(scale)
{
	//   440 Hz * 2 ^ ((scale-69)/12)
	// = 440 Hz * 2 root 12 ^ (scale - 69)
	// ~ 440 Hz * 1.05946309436 ^ (scale - 69)

	return (440.0) ~* Float::IntPow(1.05946309436, scale - 69);
}

fn Synth::Ampl(time, note, env)
{
	put state  = note.Note::STATE;
	put elapse = time ~- (note.Note::TIME);
	put limit  = Env::LookupTime(env, state);
	put perc   = elapse ~/ limit;
	
	jump attach  ~ state == Note::State::ATTACK;
	jump decay   ~ state == Note::State::DECAY;
	jump sustain ~ state == Note::State::SUSTAIN;
	jump release ~ state == Note::State::RELEASE;
	jump dead    ~ state == Note::State::DEAD;

	lab attach;  return perc ~* (env.Env::AMPL_START);
	lab decay;   return (env.Env::AMPL_START) ~- 
		                (perc ~* ((env.Env::AMPL_START) ~- (env.Env::AMPL_SUSTAIN)));
	lab sustain; return (env.Env::AMPL_SUSTAIN);
	lab release; return (env.Env::AMPL_SUSTAIN) ~* ((1.0) ~- perc);
	lab dead;    return (0.0);
}


fn Synth::Note(time, note, env)
{
	Note::Update(time note, env);
	jump dead ~ (note.Note::STATE) == (Note::State::DEAD);


	put ampl   = Synth::Ampl(time, note, env);
	put bright = ampl ~* ampl ~* ampl;
	put scale  = (note.Note::NUMBER) ~- 12;

	put base = Synth::ScaleToFreq(scale);
	put sound = 

		// sound pretty nice idk.
		//((1.0) ~* Osc(time, Synth::ScaleToFreq(scale   ), Osc::Kind::SINE, 5.0, 0.01))  ~+
		//((0.5) ~* Osc(time, Synth::ScaleToFreq(scale+12), Osc::Kind::SINE, 1.0, 0.01))  ~+
		//((0.1) ~* Osc(time, Synth::ScaleToFreq(scale+24), Osc::Kind::SQUARE, 0.0, 0.0))  ;

		// piano (maybe)
		((1.00) ~* Osc(time, base ~* (1.0), Osc::Kind::SINE, 10.0, 0.001)) ~+

		((0.45) ~* Float::IntPow(bright, 1) ~* Osc(time, base ~* (2.002), Osc::Kind::SINE, 0.0, 0.0)) ~+
		((0.20) ~* Float::IntPow(bright, 2) ~* Osc(time, base ~* (3.010), Osc::Kind::SINE, 0.0, 0.0)) ~+
		((0.12) ~* Float::IntPow(bright, 3) ~* Osc(time, base ~* (4.025), Osc::Kind::SINE, 0.0, 0.0)) ~+
		((0.07) ~* Float::IntPow(bright, 4) ~* Osc(time, base ~* (5.050), Osc::Kind::SINE, 0.0, 0.0)) ;
		

	return ampl ~* sound;

lab dead;
	return 0.0;
}


fn Synth::Sample(time, root, env)
{
	put iter = root.Note::NEXT;
	put sum  = 0.0;

	lab loop;
		jump done ~ iter == Mem::NULL;

		put sum = sum ~+ Synth::Note(time, iter, env);
		put iter = iter.Note::NEXT;

		jump loop;
	lab done;

	put sample = sum ~* (0.2);
	jump normal ~ sample ~< (1.0);
		put sample = 1.0;
	lab normal;

	put sample_scaled    = sample ~* (32767.0);
	put sample_quantized = Float::ToSigned(sample_scaled);
	put sample_truncated = sample_quantized & 65535;

	return sample_truncated;
}



