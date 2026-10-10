
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define SAMPLES 256

uint frame_count = 0;
int sock;
struct __attribute__((packed))
{
	uint32_t magic;
	uint8_t  sample_rate;
	uint8_t  sample_count;
	uint8_t  channel;
	uint8_t  flags;
	char     name[16];
	uint32_t frame_number;
	int16_t  samples[SAMPLES];
} packet;



void init()
{
	struct sockaddr_in addr;

	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(6980);
	addr.sin_addr.s_addr = inet_addr("192.168.1.10");
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	connect(sock, (struct sockaddr*)&addr, sizeof(addr));

	packet.magic = *(uint32_t*)"VBAN";
	packet.sample_rate  = 0x10; // 44.1 kHz, VBAN_PROTOCOL_AUDIO
	packet.sample_count = SAMPLES - 1;
	packet.channel      = 0x00; // channel 1
	packet.flags        = 0x01; // VBAN_DATATYPE_INT16 | VBAN_CODEC_PCM
	strcpy(packet.name, "main");
}


int main()
{
	init();
	while (true)
	{
		for (size_t i = 0; i < SAMPLES; i++)
			fread(&packet.samples[i], 2, 1, stdin);

		packet.frame_number = frame_count++;
		send(sock, &packet, sizeof(packet),0);
	}
}

