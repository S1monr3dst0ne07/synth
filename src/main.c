

#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

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

void synth_packet()
{
	static uint64_t index = 0;
	for (size_t i = 0; i < 256; i++)
	{
		double sample_time = index++ / (double)S_RATE;
		double freq_time   = (sample_time * 100.0);
	
		//double ampl = sin(freq_time * 3.14);
		//packet.samples[i] = (int16_t)(ampl * ((1 << 15) - 1));
	}

	packet.frame_number = frame_count++;
	send(pcm_sock, &packet, sizeof(packet),0);
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
		if(poll(&pfd, 1, -1) < 1) return;

		// midi "status bytes"
		if(read(STDIN_FILENO, &bytes, 3) < 1) return;

		uint8_t func = bytes[0];
		uint8_t action = func >> 4;
		uint8_t chan   = func & 0x0f;

		printf("%d\n", action);

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


