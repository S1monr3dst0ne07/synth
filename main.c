

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

typedef struct voice_s
{
	// both 7 bits
	uint8_t number;
	uint8_t vel;

	double freq;
	double mag; // in [0, 1]

	bool released;
	struct voice_s* next;
} voice_t;

voice_t root = {
	.next = NULL
};


void compute_voice_freq(voice_t* node)
{
	// 440 Hz * 2^((n-69)/12)

	node->freq = 440 * pow(2.0, 
		(node->number - 69)/12.0
	);

}

void update_voice(voice_t* node)
{
	// 1.0 -> raise in 100ms
	// 48000 * 0.1 = 4800
	double vel_norm = (node->vel + 1) / 128.0;
	double vel_prop = vel_norm / 4800.0;

	if (!node->released && node->mag < 1.0)
		node->mag += vel_prop;

	if (node->released)
		node->mag -= vel_prop;

	if (node->mag > 1.0) node->mag = 1.0;
	if (node->mag < 0.0) node->mag = 0.0;
		
}


int16_t synth_sample(double sample_time)
{
	voice_t* iter = root.next;

	double total = 0;
	double sum = 0;

	while (iter)
	{
		update_voice(iter);

		double freq_time = sample_time * iter->freq;
		double sample_ampl = sin(freq_time * 3.1415926 * 2);
		//double sample_ampl = sin(freq_time * 3.1415926 * 2) > 0 ? 1.0 : -1.0;

		sum   += sample_ampl * iter->mag;
		total += iter->mag;

		iter = iter->next;
	}

	if (total < 1.0) total = 1.0;
	double sample = sum / total;

	int16_t sample_quant = (int16_t)(sample * ((1 << 15) - 1));
	return sample_quant;
}


voice_t* find_voice(uint8_t number)
{
	voice_t* iter = root.next;
	while (iter)
	{
		if (iter->number == number) 
			return iter;
		iter = iter->next;
	}
	return NULL;
}


void process_midi()
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

				voice_t* node = malloc(sizeof(voice_t));
				node->next = root.next;
				root.next = node;

				node->number = bytes[1];
				node->vel    = vel;
				node->mag    = 0.0;
				node->released = false;

				compute_voice_freq(node);
			}; break;

			case 0b1000: release: { // note release
				voice_t* node = find_voice(bytes[1]);
				node->released = true;
			}; break;
		}
	}
}

void gc_voices()
{
	voice_t* iter = &root;
	voice_t* prev;

	while (iter->next)
	{
		prev = iter;
		iter = iter->next;

		if (iter->released && iter->mag < 0.001)
			// remove dead voice
		{
			voice_t* dead = iter;
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

		process_midi();
		gc_voices();

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


