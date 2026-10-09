

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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define S_RATE (48000)
#define S_COUNT (256)

uint frame_count;
struct __attribute__((packed))
{
	uint32_t magic;
	uint8_t  sample_rate;
	uint8_t  sample_count;
	uint8_t  channel;
	uint8_t  flags;
	char     name[16];
	uint32_t frame_number;
	int16_t  samples[S_COUNT];
} packet;


int pcm_sock;
int midi_sock;

void init()
{
	struct sockaddr_in addr;

	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(6980);
	addr.sin_addr.s_addr = inet_addr("192.168.1.10");
	pcm_sock = socket(AF_INET, SOCK_DGRAM, 0);
	connect(pcm_sock, (struct sockaddr*)&addr, sizeof(addr));

	packet.magic = *(uint32_t*)"VBAN";
	packet.sample_rate  = 0x03; // 48 kHz, VBAN_PROTOCOL_AUDIO
	packet.sample_count = 0xff; // S_COUNT - 1
	packet.channel      = 0x00; // channel 1
	packet.flags        = 0x01; // VBAN_DATATYPE_INT16 | VBAN_CODEC_PCM
	strcpy(packet.name, "main");
}




typedef struct voice_s
{
	// both 7 bits
	uint8_t number;
	uint8_t vel;

	double freq;
	double mag; // in [0, 1]

	struct voice_s* next;
} voice_t;

voice_t root;


void compute_voice_freq(voice_t* node)
{
	// 440 Hz * 2^((n-69)/12)

	node->freq = 440 * pow(2.0, 
		(node->number - 69)/12.0
	);

}


double synth_sample(double sample_time)
	// time in seconds to ampl in [-1, 1]
{
	voice_t* iter = root.next;
	size_t count = 0;
	double sum = 0;

	while (iter)
	{
		double freq_time = sample_time * iter->freq;
		double sample_ampl = sin(freq_time * 3.1415926);

		sum += sample_ampl * iter->mag;

		iter = iter->next;
		count++;
	}

	return sum / (double)count;
}

void synth_packet()
{
	static uint64_t index = 0;
	for (size_t i = 0; i < 256; i++)
	{
		double sample_time = index++ / (double)S_RATE;
		double ampl = synth_sample(sample_time);
		packet.samples[i] = (int16_t)(ampl * ((1 << 15) - 1));
	}

	packet.frame_number = frame_count++;
	send(pcm_sock, &packet, sizeof(packet),0);
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
				voice_t* new = malloc(sizeof(voice_t));
				new->next = root.next;
				root.next = new;

				new->number = bytes[1];
				new->vel    = bytes[2];
				new->mag    = 1.0;

				compute_voice_freq(new);
			}; break;

			case 0b1000: { // note release
				voice_t* node = find_voice(bytes[1]);
				node->mag = 0.0;
			}; break;
		}
	}
}

void gc_voices()
{
	voice_t* iter = &root;

	while (iter->next)
	{
		if (iter->next->mag < 0.001)
			// remove dead voice
		{
			voice_t* dead = iter->next;
			iter->next = dead->next;
			free(dead);
		}
		else
			iter = iter->next;
	}

}


int main()
{
	init();

	const uint64_t bil = 1000000000L;
	const uint64_t interval = (bil / S_RATE) * 256;

	struct timespec next;
	clock_gettime(CLOCK_MONOTONIC, &next);

	while (true)
	{
		// process incoming midi commands
		process_midi();

		// synthesize and upload more pcm data
		synth_packet();

		// remove dead voices
		gc_voices();


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


