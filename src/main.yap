
use "lib/float.yap"
use "lib/io.yap"

use "src/synth.yap"
use "src/midi.yap"

seq Config
{
	RATE = 44100, //sample rate
}


fn send_pcm_sample(amplitude)
{
	static 1 ~ packet;
	put packet.0 = amplitude;

	syscall(
		SYSCALL::WRITE,
		IO::STD::OUT,
		packet,
		2,
	);
}


fn main()
{
	put bil = 1000000000;
	put interval = bil / Config::RATE;

	static Time::Spec ~ next;
	syscall(SYSCALL::CLOCK_GETTIME,
		Time::Clock::MONOTONIC,
		next,
	);

	put index = 0;

	put main_env = Chunk::New(Env);
	put main_env.Env::TIME_ATTACK  = 0.05;
	put main_env.Env::TIME_DECAY   = 0.2;
	put main_env.Env::TIME_RELEASE = 0.7;

	put main_env.Env::AMPL_START   = 1.0;
	put main_env.Env::AMPL_SUSTAIN = 0.8;

	static Note ~ root;

	lab loop;
		put time = Float::FromInt(index) ~/ Float::FromInt(Config::RATE);
		put index = index + 1;

		Midi::Process(time, root);
		Note::GC(root);

		put sample16 = Synth::Sample(time, root, main_env);
		send_pcm_sample(sample16);

		// compute time of next frame
		put next.Time::Spec::NSEC = (next.Time::Spec::NSEC) + interval;
		jump no_overflow ~ (next.Time::Spec::NSEC) < bil;
			put next.Time::Spec::NSEC = (next.Time::Spec::NSEC) - bil;
			put next.Time::Spec::SEC  = (next.Time::Spec::SEC ) +   1;
		lab no_overflow;

		// wait to start of next frame
		syscall(SYSCALL::CLOCK_NANOSLEEP,
			Time::Clock::MONOTONIC,
			Time::Timer::ABSTIME,
			next,
			Mem::NULL,
		);
	jump loop;
}
