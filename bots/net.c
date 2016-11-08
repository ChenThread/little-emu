#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#ifndef SERVER
#include <SDL.h>
#endif
#include "littlesms.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <fcntl.h>

static int sockfd;
#ifdef SERVER
#define PLAYER_MAX 2
#define CLIENT_MAX 256
static uint32_t cli_player[CLIENT_MAX];
static struct sockaddr *cli_addr[CLIENT_MAX];
static socklen_t cli_addrlen[CLIENT_MAX];
static int32_t player_cli[PLAYER_MAX] = {-1, -1};
static uint32_t player_frame_idx[PLAYER_MAX];
static uint32_t player_initial_frame_idx[PLAYER_MAX];
#else
static struct sockaddr *serv_addr;
static socklen_t serv_addrlen;
static uint32_t initial_backlog = 0;
#endif

// a minute should be long enough
#define BACKLOG_CAP 4096
static struct SMS backlog[BACKLOG_CAP];
static uint8_t input_log[BACKLOG_CAP][2];
static uint32_t backlog_idx = 0;

static uint8_t net_intro_packet[13];

const uint8_t player_masks[2][2] = {
	{0x3F, 0x00},
	{0xC0, 0x0F},
};

int player_control = 0;
int32_t player_id = -2;
uint32_t sms_rom_crc = 0;

static uint32_t crc32_sms_net(uint8_t *data, size_t len, uint32_t crc)
{
	crc = ~crc;

	for(size_t i = 0; i < len; i++) {
		crc ^= (uint32_t)(data[i]);
		for(int j = 0; j < 8; j++) {
			crc = (crc>>1)^((crc&1)*0xEDB88320);
		}
	}

	return ~crc;
}

uint8_t bot_hook_input(struct SMS *sms, uint64_t timestamp, int port)
{
#ifndef SERVER
	SDL_Event ev;
	if(!sms->no_draw) {
	while(SDL_PollEvent(&ev)) {
		switch(ev.type) {
			case SDL_KEYDOWN:
				if((player_control&1) != 0) {
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] &= ~0x01; break;
					case SDLK_s: sms->joy[0] &= ~0x02; break;
					case SDLK_a: sms->joy[0] &= ~0x04; break;
					case SDLK_d: sms->joy[0] &= ~0x08; break;
					case SDLK_KP_2: sms->joy[0] &= ~0x10; break;
					case SDLK_KP_3: sms->joy[0] &= ~0x20; break;
					default:
						break;
				}
				}

				if((player_control&2) != 0) {
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] &= ~0x40; break;
					case SDLK_s: sms->joy[0] &= ~0x80; break;
					case SDLK_a: sms->joy[1] &= ~0x01; break;
					case SDLK_d: sms->joy[1] &= ~0x02; break;
					case SDLK_KP_2: sms->joy[1] &= ~0x04; break;
					case SDLK_KP_3: sms->joy[1] &= ~0x08; break;
					default:
						break;
				}
				}
				break;

			case SDL_KEYUP:
				if((player_control&1) != 0) {
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] |= 0x01; break;
					case SDLK_s: sms->joy[0] |= 0x02; break;
					case SDLK_a: sms->joy[0] |= 0x04; break;
					case SDLK_d: sms->joy[0] |= 0x08; break;
					case SDLK_KP_2: sms->joy[0] |= 0x10; break;
					case SDLK_KP_3: sms->joy[0] |= 0x20; break;
					default:
						break;
				}
				}

				if((player_control&2) != 0) {
				switch(ev.key.keysym.sym)
				{
					case SDLK_w: sms->joy[0] |= 0x40; break;
					case SDLK_s: sms->joy[0] |= 0x80; break;
					case SDLK_a: sms->joy[1] |= 0x01; break;
					case SDLK_d: sms->joy[1] |= 0x02; break;
					case SDLK_KP_2: sms->joy[1] |= 0x04; break;
					case SDLK_KP_3: sms->joy[1] |= 0x08; break;
					default:
						break;
				}
				}
				break;

			case SDL_QUIT:
				exit(0);
				break;
			default:
				break;
		}
	}
	}
#endif

	return sms->joy[port&1];
}

void bot_update()
{
#ifdef SERVER
	// Do a quick check
	if(player_cli[0] < 0) {
		player_frame_idx[0] = backlog_idx;
	}
	if(player_cli[1] < 0) {
		player_frame_idx[1] = backlog_idx;
	}
	// Get messages
	for(;;)
	{
		bool jump_out = false;

		// Ensure that we are ahead of both players
		// Unless we have no players, in which case wait until we have players
		int32_t wldiff0 = (int32_t)(backlog_idx-player_frame_idx[0]);
		int32_t wldiff1 = (int32_t)(backlog_idx-player_frame_idx[1]);
		if((wldiff0 < 0 || wldiff1 < 0) && (player_cli[0] >= 0 || player_cli[1] >= 0)) {
			jump_out = true;
		}

		for(;;) {
			uint8_t mbuf[2048];
			uint8_t maddr[128];
			socklen_t maddr_len = sizeof(maddr);
			memset(mbuf, 0, sizeof(mbuf));
			ssize_t rlen = recvfrom(sockfd, mbuf, sizeof(mbuf),
				(jump_out ? MSG_DONTWAIT : 0), (struct sockaddr *)&maddr, &maddr_len);

			if(rlen < 0) {
				break;
			}

			// Try to find client

			int cidx = -1;
			for(int i = 0; i < CLIENT_MAX; i++) {
				if(cli_addrlen[i] == maddr_len
					&& !memcmp(cli_addr[i], maddr, cli_addrlen[i])) {

					cidx = i;
					break;
				}
			}

			printf("SMSG %5d %02X %d\n", (int)rlen, mbuf[0], cidx);

			if(cidx == -1 && mbuf[0] == '\x01') {
				// Someone wants to connect!
				printf("Client trying to connect\n");

				// Validate data
				if(rlen != sizeof(net_intro_packet)) {
					sendto(sockfd,
						"\x02""Bad intro\x00", 11, 0,
						(struct sockaddr *)&maddr,
						maddr_len);
					continue;

				} else if(0 != memcmp(net_intro_packet, mbuf, rlen)) {
					sendto(sockfd,
						"\x02""Bad emu state\x00", 15, 0,
						(struct sockaddr *)&maddr,
						maddr_len);
					continue;

				}

				// Find a slot
				for(int i = 0; i < CLIENT_MAX; i++) {
					if(cli_addrlen[i] == 0) {
						cidx = i;
						break;
					}
				}

				if(cidx == -1) {
					sendto(sockfd,
						"\x02""Server full\x00", 13, 0,
						(struct sockaddr *)&maddr,
						maddr_len);
					continue;
				}

				// Find a suitable player slot
				int pidx = -1;
				if(player_cli[0] < 0) {
					pidx = 0;
				} else if(player_cli[1] < 0) {
					pidx = 1;
				}

				cli_player[cidx] = pidx;
				if(pidx != -1) {
					player_cli[pidx] = cidx;
					player_frame_idx[pidx] = backlog_idx;
					player_initial_frame_idx[pidx] = backlog_idx;
				}
				cli_addrlen[cidx] = maddr_len;
				cli_addr[cidx] = malloc(maddr_len);
				memcpy(cli_addr[cidx], maddr, maddr_len);

				// Send intro
				uint8_t sintro_buf[9];
				sintro_buf[0] = 0x01;
				((uint32_t *)(sintro_buf+1))[0] = (pidx == -1
					? backlog_idx
					: player_frame_idx[pidx]);
				((int32_t *)(sintro_buf+1))[1] = pidx;
				sendto(sockfd, sintro_buf, sizeof(sintro_buf), 0,
					(struct sockaddr *)&maddr, maddr_len);

			} else if(cidx == -1) {
				// We don't have this client, kick it
				sendto(sockfd,
					"\x02""Not connected\x00", 15, 0,
					(struct sockaddr *)&maddr,
					maddr_len);

			} else if(mbuf[0] == '\x01') {
				// Did they not get the intro? Resend the intro.
				int pidx = cli_player[cidx];

				uint8_t sintro_buf[9];
				sintro_buf[0] = 0x01;
				((uint32_t *)(sintro_buf+1))[0] = (pidx == -1
					? backlog_idx
					: player_frame_idx[pidx]);
				((int32_t *)(sintro_buf+1))[1] = pidx;
				sendto(sockfd, sintro_buf, sizeof(sintro_buf), 0,
					(struct sockaddr *)&maddr, maddr_len);

			} else if(mbuf[0] == '\x02') {
				// Someone's sick of us!
				mbuf[sizeof(mbuf)-1] = '\x00';
				printf("Client disconnected: \"%s\"\n", mbuf+1);

				int pidx = cli_player[cidx];
				if(pidx != -1) {
					player_cli[pidx] = -1;
				}
				cli_addrlen[cidx] = 0;
				free(cli_addr[cidx]);
				cli_addr[cidx] = NULL;
			}
		}

		if(jump_out) {
			break;
		}
	}
#else

	// Get messages
	for(;;)
	{
		uint8_t mbuf[2048];
		uint8_t maddr[128];
		socklen_t maddr_len = sizeof(maddr);
		memset(mbuf, 0, sizeof(mbuf));
		ssize_t rlen = recvfrom(sockfd, mbuf, sizeof(mbuf),
			MSG_DONTWAIT, (struct sockaddr *)&maddr, &maddr_len);

		if(rlen < 0) {
			break;
		}

		printf("CMSG %d %02X\n", (int)rlen, mbuf[0]);

		if(mbuf[0] == '\x01') {
			// We connected! Again!
			printf("Acknowledged! Again!\n");
			initial_backlog = ((uint32_t *)(mbuf+1))[0];
			player_id = ((int32_t *)(mbuf+1))[0];
			backlog_idx = initial_backlog;
			player_control = (player_id >= 0 && player_id <= 31
				? (1<<player_id) : 0);

		} else if(mbuf[0] == '\x02') {
			// The server hates us!
			mbuf[sizeof(mbuf)-1] = '\x00';
			printf("KICKED: \"%s\"\n", mbuf+1);
			fflush(stdout);
			abort();
		}
	}
#endif

	// Save frame
	uint32_t idx = (backlog_idx&(BACKLOG_CAP-1));
	sms_copy(&backlog[idx], &sms_current);
	input_log[idx][0] = sms_current.joy[0];
	input_log[idx][1] = sms_current.joy[1];
	backlog_idx++;

	//twait = time_now()-20000;
}

void bot_init(int argc, char *argv[])
{
	sms_current.joy[0] = 0xFF;
	sms_current.joy[1] = 0xFF;

	sms_rom_crc = crc32_sms_net(sms_rom, sizeof(sms_rom), 0);
	//sms_rom_crc = crc32_sms_net(sms_rom, 512*1024, 0);

	net_intro_packet[0] = 0x01;
	((uint32_t *)(net_intro_packet+1))[0] = (uint32_t)sizeof(struct SMS);
	((uint32_t *)(net_intro_packet+1))[1] = sms_rom_crc;
	((uint32_t *)(net_intro_packet+1))[2] = 0
		| (sms_rom_is_banked ? 0x01 : 0)
		| (USE_NTSC ? 0x02 : 0)
		|0;

#ifdef SERVER
	if(argc <= 1) {
		fprintf(stderr, "usage: %s port\n", argv[0]);
		fflush(stderr);
		abort();
	}

	printf("Starting server\n");

	// Get info
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_DGRAM,
		.ai_flags = AI_PASSIVE,
	};
	struct addrinfo *ai;
	int gai_err = getaddrinfo(NULL, argv[1], &hints, &ai);

	if(gai_err != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_err));
		abort();
	}

	printf("Info for localhost:\n");
	struct addrinfo *ai_in4 = NULL;
	struct addrinfo *ai_in6 = NULL;
	for(struct addrinfo *p = ai; p != NULL; p = p->ai_next) {
		char adsbuf[128];
		adsbuf[0] = '\x00';
		switch(p->ai_family) {
			case AF_INET:
				ai_in4 = p;
				inet_ntop(p->ai_family,
					&(((struct sockaddr_in *)(p->ai_addr))->sin_addr),
					adsbuf, sizeof(adsbuf));
				printf(" - IPv4: \"%s\"\n", adsbuf);
				break;

			case AF_INET6:
				ai_in6 = p;
				inet_ntop(p->ai_family,
					&(((struct sockaddr_in6 *)(p->ai_addr))->sin6_addr),
					adsbuf, sizeof(adsbuf));
				printf(" - IPv6: \"%s\"\n", adsbuf);
				break;

			default:
				printf(" - ??? (%d)\n", p->ai_family);
				break;
		}

	}
	printf("\n");

	// Get socket
	struct addrinfo *ai_best = (ai_in6 != NULL ? ai_in6 : ai_in4);
	assert(ai_best != NULL);
	sockfd = socket(ai_best->ai_family, ai_best->ai_socktype, ai_best->ai_protocol);
	assert(sockfd > 0);

	// Bind port
	int yes = 1;
	int sso_err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	assert(sso_err != -1);
	int bind_err = bind(sockfd, ai_best->ai_addr, ai_best->ai_addrlen);
	assert(bind_err == 0);

	// Free info
	freeaddrinfo(ai);

	printf("Server is now online!\n\n");
#else
	if(argc <= 2) {
		fprintf(stderr, "usage: %s hostname port\n", argv[0]);
		fflush(stderr);
		abort();
	}

	printf("Starting client\n");

	// Get info
	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_DGRAM,
	};
	struct addrinfo *ai;
	int gai_err = getaddrinfo(argv[1], argv[2], &hints, &ai);

	if(gai_err != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_err));
		abort();
	}

	// Get socket
	struct addrinfo *ai_best = ai;
	assert(ai_best != NULL);
	printf("%d %d %d\n", ai_best->ai_family, ai_best->ai_socktype, ai_best->ai_protocol);
	sockfd = socket(ai_best->ai_family, ai_best->ai_socktype, ai_best->ai_protocol);
	assert(sockfd > 0);

	// Copy addr
	serv_addrlen = ai_best->ai_addrlen;
	serv_addr = malloc(serv_addrlen);
	memcpy(serv_addr, ai_best->ai_addr, serv_addrlen);

	// Free info
	freeaddrinfo(ai);

	printf("Client is now online!\n\n");

	// Try connecting to server
	printf("Establishing connection...\n");
	bool connected = false;

	for(int attempts = 20; attempts > 0 && !connected; attempts--) {
		sendto(sockfd, net_intro_packet, sizeof(net_intro_packet), 0, serv_addr, serv_addrlen);
		usleep(200000);
		for(;;) {
			uint8_t mbuf[2048];
			uint8_t maddr[128];
			socklen_t maddr_len = sizeof(maddr);
			memset(mbuf, 0, sizeof(mbuf));
			ssize_t rlen = recvfrom(sockfd, mbuf, sizeof(mbuf),
				MSG_DONTWAIT, (struct sockaddr *)&maddr, &maddr_len);

			if(rlen < 0) {
				break;
			}

			printf("CMSG %d %02X\n", (int)rlen, mbuf[0]);

			if(mbuf[0] == '\x01') {
				// We connected!
				assert(rlen == 9);
				printf("Acknowledged!\n");
				initial_backlog = ((uint32_t *)(mbuf+1))[0];
				player_id = ((int32_t *)(mbuf+1))[0];
				backlog_idx = initial_backlog;
				player_control = (player_id >= 0 && player_id <= 31
					? (1<<player_id) : 0);
				connected = true;
				break;
			} else if(mbuf[0] == '\x02') {
				// The server hates us!
				mbuf[sizeof(mbuf)-1] = '\x00';
				printf("KICKED: \"%s\"\n", mbuf+1);
				connected = false;
				break;
			}
		}
	}
	assert(connected);

	// Sync state
	// TODO!
#endif
	printf("ROM CRC: %04X\n", sms_rom_crc);

}

