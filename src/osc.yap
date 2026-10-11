
fn Osc::Sin(raw)
{
	put pi = 3.141529;
	put raw = raw ~/ pi;
	put frac = ((raw ~+ (1.0)) ~% (2.0)) ~- (1.0);

	put abs = frac & ((0 - 1) >> 1);
	return ((1.0) ~- abs) ~* frac ~* (4.0);
}


seq Osc::Kind
{
	SINE,
	SQUARE,
	TRIANGLE,
}


fn Osc(time, freq, kind, low_freq, low_ampl)
{
	put tau = 6.283185307;

	put mod = (tau  ~* freq ~* time) ~+ 
	          (freq ~* low_ampl ~* Osc::Sin(low_freq ~* time ~* tau));

	jump sine   ~ kind == Osc::Kind::SINE;
	jump square ~ kind == Osc::Kind::SQUARE;

lab sine;   return Osc::Sin(mod);
lab square; 
	jump on ~ Osc::Sin(mod) ~> 1;
	jump off;

	lab on;  return          (1.0);
	lab off; return (0.0) ~- (1.0);
}





