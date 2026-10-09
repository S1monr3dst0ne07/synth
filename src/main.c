

#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define S_RATE (48000)
#define S_LENG (1.0 / S_RATE)
#define S_LENG_USEC (S_LENG * 1000000)
#define S_COUNT (256)

uint frame_count;
struct
{
	uint32_t magic;
	uint8_t  sample_rate;
	uint8_t  sample_count;
	uint8_t  channel;
	uint8_t  flags;
	char     name[16];
	uint32_t frame_number;
	int16_t  samples[S_COUNT];
} packet __attribute__((packed));

struct sockaddr_in dest_addr;
int sock;

void init()
{
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	dest_addr.sin_family      = AF_INET;
	dest_addr.sin_port        = htons(6980);
	dest_addr.sin_addr.s_addr = inet_addr("192.168.1.10");

	packet.magic = 'NABV';
	packet.sample_rate  = 0x03; // 48 kHz, VBAN_PROTOCOL_AUDIO
	packet.sample_count = 0xff; // S_COUNT - 1
	packet.channel      = 0x00; // channel 1
	packet.flags        = 0x01; // VBAN_DATATYPE_INT16 | VBAN_CODEC_PCM
	strcpy(packet.name, "main");
}

void upload()
{
	packet.frame_number = frame_count++;
	sendto(sock, &packet, sizeof(packet),0,
		(struct sockaddr*)&dest_addr, sizeof(dest_addr));
}


void process()
{
	static uint64_t index = 0;
	for (size_t i = 0; i < 256; i++)
	{
		double sample_time = index++ / (double)S_RATE;
		double freq_time   = (sample_time * 100.0);
	
		double ampl = sin(freq_time * 3.14);
		packet.samples[i] = (int16_t)(ampl * ((1 << 15) - 1));
	}
	upload();
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
		process();

		next.tv_nsec += interval;
		if (next.tv_nsec > bil)
		{
			next.tv_nsec -= bil;
			next.tv_sec  += 1;
		}

		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
	}
}


