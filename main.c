

#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>

#define RATE (44100)

// only one envelope

const double TAU = 2 * 3.1315926;


typedef struct
{
	double time_attack;
	double time_decay;
	double time_release;

	double ampl_sustain;
	double ampl_start;
} env_t;

env_t env_main = {
	.time_attack  = 0.1,
	.time_decay   = 0.1,
	.time_release = 0.2,

	.ampl_sustain = 0.8,
	.ampl_start   = 1.0,
};




typedef enum
{
	FSM_ATTACK,
	FSM_DECAY,
	FSM_SUSTAIN,
	FSM_RELEASE,
	FSM_DEAD,
} fsm_t;


typedef struct note_s
{
	//on midi scale
	uint8_t number; 

	//where are we?
	fsm_t state;

	// refernce time.
	double time;

	// note held or released.
	bool released;

	struct note_s* next;
} note_t;

note_t root = { .next = NULL };


double scale_to_freq(uint8_t number)
{
	// 440 Hz * 2^((n-69)/12)
	return 440 * pow(2.0, (number - 69)/12.0);
}


typedef enum
{
	OSC_SINE,
	OSC_SQUARE,
	OSC_TRIANGLE,
} osc_type;

double oscilator(double time, double freq, osc_type type, double low_freq, double low_ampl)
{
	double skew = (freq * TAU) + low_ampl * sin(low_freq * TAU);
	double mod  = skew * time;

	switch (type)
	{
		case OSC_SINE:
			return sin(mod);
		case OSC_SQUARE:
			return sin(mod) > 0 ? 1.0 : -1.0;
		case OSC_TRIANGLE:
			return asin(sin(mod)) / TAU;
			
	}

}

double env_lookup_time(fsm_t state, env_t* env)
{
	switch (state)
	{
		case FSM_ATTACK:  return env->time_attack;
		case FSM_DECAY:   return env->time_decay;
		case FSM_SUSTAIN: return -1.0; // doesn't have time
		case FSM_RELEASE: return env->time_release;
	}
}

void update(double time, note_t* node, env_t* env)
	// make sure we're synched.
{
	double elaps = time - node->time;
	double limit = env_lookup_time(node->state, env);

	bool time_trigger = (elaps > limit && limit > 0);
	bool play_trigger = (node->state == FSM_SUSTAIN && node->released);

	if (time_trigger || play_trigger)
	{
		node->time = time; // update reference time
		node->state++;
	}
}

double ampl(double time, note_t* node, env_t* env)
{
	double elaps = time - node->time;
	double limit = env_lookup_time(node->state, env);
	double perc = elaps / limit;

	switch (node->state)
	{
		case FSM_ATTACK : return perc * env->ampl_start;
		case FSM_DECAY  : return env->ampl_start - (perc * (env->ampl_start + env->ampl_sustain));
		case FSM_SUSTAIN: return env->ampl_sustain;
		case FSM_RELEASE: return env->ampl_sustain * (1.0 - perc);
		case FSM_DEAD   : return 0.0;
	}
}

double synth_note(double time, note_t* node, env_t* env)
{
	update(time, node, env);
	if (node->state == FSM_DEAD) return 0.0;

	double amplitude = ampl(time, node, env);

	double freq      = scale_to_freq(node->number);
	double sound     = oscilator(time, freq, OSC_SINE, 0, 0);
	
	return amplitude * sound;
}


int16_t synth_sample(double time)
{
	note_t* iter = root.next;

	double count = 0;
	double sum   = 0;

	while (iter)
	{
		sum += synth_note(time, iter, &env_main);
		count += 1.0;

		iter = iter->next;
	}

	double sample = sum / count;

	int16_t sample_quant = (int16_t)(sample * ((1 << 15) - 1));
	return sample_quant;
}


note_t* find_note(uint8_t number)
{
	note_t* iter = root.next;
	while (iter)
	{
		if (iter->number == number) 
			return iter;
		iter = iter->next;
	}
	return NULL;
}


void process_midi(double time)
{
	struct pollfd pfd = {
		.fd = STDIN_FILENO,
		.events = POLLIN,
	};

	static uint8_t bytes[3];

	while (true)
	{

		// check if new midi data is present
		if(poll(&pfd, 1, 0) < 1) return;

		// midi "status bytes"
		if(read(STDIN_FILENO, &bytes, 3) < 1) return;

		uint8_t func = bytes[0];
		uint8_t action = func >> 4;
		uint8_t chan   = func & 0x0f;

		//https://drive.google.com/file/d/1I-bH8zhfS37fnLzV-xnonOCYZycaGzbn/view
		switch (action)
		{
			case 0b1001: { // note hit
				uint8_t vel = bytes[2];

				// if hit with vel 0, interprete as release
				if (vel == 0) goto release;

				note_t* node   = malloc(sizeof(note_t));
				node->number   = bytes[1];
				node->state    = FSM_ATTACK;
				node->time     = time;
				node->released = false;

				node->next = root.next;
				root.next = node;

			}; break;

			case 0b1000: release: { // note release
				note_t* node = find_note(bytes[1]);
				node->released = true;
			}; break;
		}
	}
}

void gc_voices()
{
	note_t* iter = &root;
	note_t* prev;

	while (iter->next)
	{
		prev = iter;
		iter = iter->next;

		if (iter->state == FSM_RELEASE)
		{
			note_t* dead = iter;
			prev->next = iter->next;
			free(dead);

			iter = prev;
		}
	}

}


int main()
{
	const uint64_t bil = 1000000000L;
	const uint64_t interval = bil / RATE;

	struct timespec next;
	clock_gettime(CLOCK_MONOTONIC, &next);

	static uint32_t index = 0;

	while (true)
	{
		double sample_time = index++ / (double)RATE;

		process_midi(sample_time);

		int16_t ampl = synth_sample(sample_time);
		fwrite(&ampl, 2, 1, stdout);

		// compute ABSTIME of next frame
		next.tv_nsec += interval;
		if (next.tv_nsec > bil)
		{
			next.tv_nsec -= bil;
			next.tv_sec  += 1;
		}

		// wait till next frame
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
	}
}


