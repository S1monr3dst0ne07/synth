

#include <sys/time.h>
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

uint32_t sample_index = 0;
uint64_t last = 0;


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

void process(uint64_t upoint)
{
	double utime = upoint / 1000000.0;
	double ufreq = (utime * 1000.0);

	double ampl = sin(ufreq * 3.14);
	packet.samples[sample_index++] = (int16_t)(ampl * ((1 << 15) - 1));

	if (sample_index > S_COUNT)
	{
		sample_index = 0;
		upload();
	}
}

int main()
{
	struct timeval tv;
	static double point = 0.0;

	init();

	while (true)
	{
		gettimeofday(&tv, NULL);
		uint64_t delta = tv.tv_usec - last;
		if (delta < S_LENG_USEC) continue;
		last = tv.tv_usec;

		process(last);
	}


}


