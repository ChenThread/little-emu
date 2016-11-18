#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#define NET_DEBUG_SYNC 0
#define NET_DEBUG_SYNC_RESIM 1

#ifndef SERVER
#include <SDL.h>
#endif
#include "system/sms/all.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <fcntl.h>

#define BACKLOG_CAP 4096
#define PLAYER_MAX 2

static const uint64_t keepalive_period = 1000000;
static const uint64_t keepalive_death  = 4000000;

static int sockfd;
#ifdef SERVER
#define CLIENT_MAX 256
static uint64_t cli_keepalive_recv[CLIENT_MAX];
static uint64_t cli_keepalive_send;
static uint32_t cli_player[CLIENT_MAX];
static uint32_t cli_frame_idx[CLIENT_MAX];
static struct sockaddr *cli_addr[CLIENT_MAX];
static socklen_t cli_addrlen[CLIENT_MAX];
static int32_t player_cli[PLAYER_MAX] = {-1, -1};
static uint32_t player_frame_idx[PLAYER_MAX];
const uint32_t initial_backlog = 0;
#else
// TODO: unhardcode
static SDL_Keycode keymap[] = {SDLK_w, SDLK_s, SDLK_a, SDLK_d, SDLK_KP_2, SDLK_KP_3};

static struct sockaddr *serv_addr;
static socklen_t serv_addrlen;
static uint64_t serv_keepalive_recv;
static uint64_t serv_keepalive_send;
static uint32_t serv_frame_idx;
static uint32_t serv_full_frame_idx;
uint32_t initial_backlog = 0;
#endif

// a minute should be long enough
#define BLWRAP(i) ((i) & (BACKLOG_CAP-1))
static struct SMS backlog[BACKLOG_CAP];
static uint8_t input_log[BACKLOG_CAP][2];
uint16_t input_pmask[BACKLOG_CAP];
static uint32_t backlog_end = 0;

static uint8_t net_intro_packet[13];

const uint8_t player_masks[PLAYER_MAX][2] = {
	{0x3F, 0x00},
	{0xC0, 0x0F},
};

int player_control = 0;
int32_t player_id = -2;
uint32_t netplay_rom_crc = 0;

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

#ifdef SERVER
static void detach_client(int cidx)
{
	// Don't try to detach unconnected clients
	if(cidx < 0 || cidx >= CLIENT_MAX) {
		return;
	}

	printf("Detaching client %5d\n", cidx);

	int pidx = cli_player[cidx];
	if(pidx != -1) {
		player_cli[pidx] = -1;
	}
	cli_addrlen[cidx] = 0;
	free(cli_addr[cidx]);
	cli_addr[cidx] = NULL;

}

static void kick_client(const char *reason, int cidx, void *maddr, socklen_t maddr_len)
{
	assert(reason[0] == '\x02');

	printf("Kicking: \"%s\"\n", reason+1);

	// Send message
	sendto(sockfd, reason, strlen(reason)+1, 0,
		(struct sockaddr *)maddr, maddr_len);

	// Check if client needs detachment
	detach_client(cidx);
}
#endif

void bot_hook_input(struct EmuGlobal *G, struct SMS *sms, uint64_t timestamp)
{
#ifndef SERVER
	int i;
	SDL_Event ev;

	if(!G->no_draw) {
	while(SDL_PollEvent(&ev)) {
		switch(ev.type) {
			case SDL_KEYDOWN:
				if((player_control&1) != 0) {
					for (i = 0; i < G->input_button_count; i++)
						if (ev.key.keysym.sym == keymap[i])
							lemu_handle_input(G, sms, 0, i, true);
				}

				if((player_control&2) != 0) {
					for (i = 0; i < G->input_button_count; i++)
						if (ev.key.keysym.sym == keymap[i])
							lemu_handle_input(G, sms, 1, i, true);
				}
				break;

			case SDL_KEYUP:
				if((player_control&1) != 0) {
					for (i = 0; i < G->input_button_count; i++)
						if (ev.key.keysym.sym == keymap[i])
							lemu_handle_input(G, sms, 0, i, false);
				}

				if((player_control&2) != 0) {
					for (i = 0; i < G->input_button_count; i++)
						if (ev.key.keysym.sym == keymap[i])
							lemu_handle_input(G, sms, 1, i, false);
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
}

void bot_update(struct SMSGlobal *G)
{
	// Save frame input
#ifndef SERVER
	if(player_id >= 0) {
		input_log[BLWRAP(backlog_end)][0] = G->current.joy[0];
		input_log[BLWRAP(backlog_end)][1] = G->current.joy[1];
	}
	input_pmask[BLWRAP(backlog_end)] |= player_control;
#endif

	// Save backlog frame (for sync purposes)
	sms_copy(&backlog[BLWRAP(backlog_end)], &G->current);

	// Send input to other clients
#ifdef SERVER
	for(int ci = 0; ci < CLIENT_MAX; ci++) {
		uint8_t mbuf[6+2*256];
		uint32_t in_end = backlog_end;
		if(cli_addrlen[ci] != 0) {
			uint32_t loc_offs = cli_frame_idx[ci];
			int32_t loc_len = (in_end-loc_offs);

			if(loc_len < 1) {
				continue;
			}

			//printf("INSEND %3d %10d %3d\n", ci, loc_offs, loc_len);

			if(loc_len < 1 || loc_len > 255) {
				kick_client("\x02""Too much lag",
					ci,
					cli_addr[ci],
					cli_addrlen[ci]);

				continue;
			}

			mbuf[0] = 0x06;
			((uint32_t *)(mbuf+1))[0] = loc_offs;
			mbuf[5] = loc_len;
			uint8_t *p = &mbuf[6];
			for(int i = 0; i < loc_len; i++) {
				int idx = BLWRAP(loc_offs+i);
				p[2*i+0] = input_log[idx][0];
				p[2*i+1] = input_log[idx][1];
			}
			sendto(sockfd, mbuf, 6+2*loc_len, 0,
				cli_addr[ci], cli_addrlen[ci]);
		}
	}
#endif

#if NET_DEBUG_SYNC
	// Send CRC32
	// And no we do NOT want to do a wrap check here
	if((uint32_t)(backlog_end-initial_backlog) >= 3) {
		uint32_t frame_idx = backlog_end-3;
		uint8_t i0 = backlog[BLWRAP(frame_idx)].joy[0];
		uint8_t i1 = backlog[BLWRAP(frame_idx)].joy[1];
		backlog[BLWRAP(frame_idx)].joy[0] = 0xFF;
		backlog[BLWRAP(frame_idx)].joy[1] = 0xFF;
		uint8_t crcpkt[31];
		crcpkt[0] = 0x08;
		*(uint32_t *)(crcpkt+1) = frame_idx;
		*(uint32_t *)(crcpkt+5) = crc32_sms_net(
			(uint8_t *)&backlog[BLWRAP(frame_idx)],
			sizeof(struct SMS), 0);
		*(uint32_t *)(crcpkt+9) = crc32_sms_net(
			(uint8_t *)&backlog[BLWRAP(frame_idx)].z80,
			sizeof(struct Z80), 0);
		*(uint32_t *)(crcpkt+13) = crc32_sms_net(
			(uint8_t *)&backlog[BLWRAP(frame_idx)].psg,
			sizeof(struct PSG), 0);
		*(uint32_t *)(crcpkt+17) = crc32_sms_net(
			(uint8_t *)&backlog[BLWRAP(frame_idx)].vdp,
			sizeof(struct VDP), 0);
		*(uint32_t *)(crcpkt+21) = crc32_sms_net(
			(uint8_t *)&backlog[BLWRAP(frame_idx)].ram,
			8192, 0);
		*(uint32_t *)(crcpkt+25) = crc32_sms_net(
			(uint8_t *)&backlog[BLWRAP(frame_idx)].vdp.vram,
			16384, 0);
		crcpkt[29] = i0;
		crcpkt[30] = i1;
		backlog[BLWRAP(frame_idx)].joy[0] = i0;
		backlog[BLWRAP(frame_idx)].joy[1] = i1;

#ifdef SERVER
		for(int ci = 0; ci < CLIENT_MAX; ci++) {
			if(cli_addrlen[ci] != 0) {
				sendto(sockfd, crcpkt, sizeof(crcpkt), 0,
					cli_addr[ci], cli_addrlen[ci]);
			}
		}
#else
		sendto(sockfd, crcpkt, sizeof(crcpkt), 0,
			serv_addr, serv_addrlen);
#endif
	}
#endif

#ifdef SERVER
	// Do a quick check
	/*
	if(player_cli[0] >= 0 || player_cli[1] >= 0) {
		if(player_cli[0] < 0) {
			player_frame_idx[0] = backlog_end;
		}
		if(player_cli[1] < 0) {
			player_frame_idx[1] = backlog_end;
		}
	}
	*/

	// Get messages
	printf("=== SFrame %d\n", backlog_end);
	for(;;)
	{
		uint64_t now = time_now();
		bool jump_out = false;

		// Ensure both players are ahead of us
		int32_t wldiff0 = (int32_t)(player_frame_idx[0]-backlog_end);
		int32_t wldiff1 = (int32_t)(player_frame_idx[1]-backlog_end);
		//printf("wldiffs %d %d\n", wldiff0, wldiff1);
		if((player_cli[0] >= 0 || player_cli[1] >= 0)) {
			if(player_cli[0] < 0 || wldiff0 > 0) {
			if(player_cli[1] < 0 || wldiff1 > 0) {
				//printf("jump-out activated\n");
				jump_out = true;
			}
			}
		}

		// Send keepalives if necessary
		if(TIME_IN_ORDER(cli_keepalive_send, now)) {
			for(int ci = 0; ci < CLIENT_MAX; ci++) {
				if(cli_addrlen[ci] != 0 &&
					TIME_IN_ORDER(cli_keepalive_recv[ci]+keepalive_death
						, now)) {

					kick_client("\x02""Timed out", ci,
						cli_addr[ci], cli_addrlen[ci]);

				} else if(cli_addrlen[ci] != 0 &&
					TIME_IN_ORDER(cli_keepalive_recv[ci], now)) {

					sendto(sockfd, "\x00", 1, 0,
						cli_addr[ci], cli_addrlen[ci]);
					cli_keepalive_send = now + keepalive_period;
				}
			}
		}

		for(;;) {
			uint8_t mbuf[2048];
			uint8_t maddr[128];
			socklen_t maddr_len = sizeof(maddr);
			memset(mbuf, 0, sizeof(mbuf));
			ssize_t rlen = recvfrom(sockfd, mbuf, sizeof(mbuf),
				//(jump_out ? MSG_DONTWAIT : 0),
				MSG_DONTWAIT,
				(struct sockaddr *)&maddr, &maddr_len);

			if(rlen < 0) {
				break;
			}

			// Try to find client

			int cidx = -1;
			for(int i = 0; i < CLIENT_MAX; i++) {
				if(cli_addrlen[i] == maddr_len
					&& !memcmp(cli_addr[i], maddr, cli_addrlen[i])) {

					cidx = i;
					cli_keepalive_recv[cidx] = now+keepalive_period;
					break;
				}
			}

			//printf("SMSG %5d %02X %d\n", (int)rlen, mbuf[0], cidx);

			if(cidx == -1 && mbuf[0] == '\x01') {
				// Someone wants to connect!
				printf("Client trying to connect\n");

				// Validate data
				if(rlen != sizeof(net_intro_packet)) {
					kick_client("\x02""Bad intro", cidx, maddr, maddr_len);
					continue;

				} else if(0 != memcmp(net_intro_packet, mbuf, rlen)) {
					kick_client("\x02""Bad emu state", cidx, maddr, maddr_len);
					continue;

				}

				// Find a slot
				for(int i = 0; i < CLIENT_MAX; i++) {
					if(cli_addrlen[i] == 0) {
						cidx = i;
						break;
					}
				}

				if(cidx < 0 || cidx >= CLIENT_MAX) {
					kick_client("\x02Server full", cidx, maddr, maddr_len);
					continue;
				}

				// Find a suitable player slot
				int pidx = -1;
				if(player_cli[0] < 0) {
					pidx = 0;
				} else if(player_cli[1] < 0) {
					// TODO: get nonspecs to sync
					pidx = 1;
				}

				cli_player[cidx] = pidx;
				printf("client %5d -> player %5d\n", cidx, pidx);
				if(pidx != -1) {
					player_cli[pidx] = cidx;
					player_frame_idx[pidx] = backlog_end;
				}
				cli_frame_idx[cidx] = backlog_end;
				cli_addrlen[cidx] = maddr_len;
				cli_addr[cidx] = malloc(maddr_len);
				memcpy(cli_addr[cidx], maddr, maddr_len);

				// Send intro
				uint8_t sintro_buf[9];
				sintro_buf[0] = 0x01;
				((uint32_t *)(sintro_buf+1))[0] = cli_frame_idx[cidx];
				((int32_t *)(sintro_buf+1))[1] = pidx;
				cli_keepalive_recv[cidx] = now+keepalive_period;
				sendto(sockfd, sintro_buf, sizeof(sintro_buf), 0,
					(struct sockaddr *)&maddr, maddr_len);

			} else if(cidx == -1) {
				// We don't have this client, kick it
				kick_client("\x02Not connected", cidx, maddr, maddr_len);

			} else if(mbuf[0] == '\x01') {
				// Did they not get the intro? Resend the intro.
				int pidx = cli_player[cidx];

				uint8_t sintro_buf[9];
				sintro_buf[0] = 0x01;
				((uint32_t *)(sintro_buf+1))[0] = cli_frame_idx[cidx];
				((int32_t *)(sintro_buf+1))[1] = pidx;
				sendto(sockfd, sintro_buf, sizeof(sintro_buf), 0,
					(struct sockaddr *)&maddr, maddr_len);

			} else if(mbuf[0] == '\x02') {
				// Someone's sick of us!
				mbuf[sizeof(mbuf)-1] = '\x00';
				printf("Client disconnected: \"%s\"\n", mbuf+1);
				detach_client(cidx);

			} else if(mbuf[0] == '\x03') {
				// Do they need a given state? Send it!
				uint32_t frame_idx = *((uint32_t *)(mbuf+1));
				uint32_t offs = *((uint32_t *)(mbuf+5));
				uint32_t len = *((uint16_t *)(mbuf+9));
				if(offs > sizeof(struct SMS) || offs+len > sizeof(struct SMS)) {
					kick_client("\x02""Out of bounds state read", cidx, maddr, maddr_len);
					continue;
				}
				if(len > 1024 || len < 1) {
					kick_client("\x02""Sync length bad", cidx, maddr, maddr_len);
					continue;
				}

				mbuf[0] = 0x04;
				uint8_t *ps = mbuf+11;
				uint8_t *pd = (uint8_t *)&backlog[BLWRAP(frame_idx)];
				memcpy(ps, pd+offs, len);
				sendto(sockfd, mbuf, 11+len, 0,
					(struct sockaddr *)&maddr, maddr_len);

			} else if(mbuf[0] == '\x06') {
				int pidx = cli_player[cidx];
				if(pidx < 0 || pidx >= PLAYER_MAX) {
					// Extraneous packet - spectators do NOT send input
					continue;
				}

				// WE HAVE INPUTS
				uint32_t in_beg = *((uint32_t *)(mbuf+1));
				uint32_t in_len = mbuf[5];
				uint32_t in_end = in_beg+in_len;

				// Ensure we are in range
				int32_t fdelta0 = (int32_t)(in_beg-backlog_end);
				int32_t fdelta1 = fdelta0+in_len;
				//printf("Deltas %3d %10d %10d\n", cidx, fdelta0, fdelta1);
				if(fdelta0 >= BACKLOG_CAP/4 || -fdelta0 >= BACKLOG_CAP-1) {
					kick_client("\x02""Frame out of range", cidx, maddr, maddr_len);
					continue;
				}
				if(fdelta1 >= BACKLOG_CAP/4 || -fdelta1 >= BACKLOG_CAP-1) {
					kick_client("\x02""Frame out of range", cidx, maddr, maddr_len);
					continue;
				}

				// Set inputs
				uint8_t *p = mbuf+6;
				uint8_t m0 = player_masks[pidx][0];
				uint8_t m1 = player_masks[pidx][1];
				bool history_failure = false;
				for(int i = 0; i < in_len; i++) {
					int idx = BLWRAP(in_beg+i);
					uint8_t new0 = p[2*i+0];
					uint8_t new1 = p[2*i+1];
					uint8_t old0 = input_log[idx][0];
					uint8_t old1 = input_log[idx][1];
					old0 &= ~m0;
					old1 &= ~m1;
					new0 &= m0;
					new1 &= m1;
					new0 |= old0;
					new1 |= old1;
					if((int32_t)(in_beg+i-backlog_end) < 0) {
						if(input_log[idx][0] != new0 || input_log[idx][1] != new1) {
							history_failure = true;
							break;
						}
						continue;
					}
					input_log[idx][0] = new0;
					input_log[idx][1] = new1;
					backlog[idx].joy[0] = new0;
					backlog[idx].joy[1] = new1;
					input_pmask[idx] |= (1<<pidx);
				}

				if(history_failure) {
					kick_client("\x02""Time paradox", cidx, maddr, maddr_len);
					continue;
				}

				// Advance if we can
				if((in_end-player_frame_idx[pidx]) > 0) {
					player_frame_idx[pidx] = in_end;
				}

				// Update source frame head
				/*
				if((int32_t)(in_end - cli_frame_idx[cidx]) > 0) {
					cli_frame_idx[cidx] = in_end;
				}
				*/

				//printf("CSourc %3d frame head: %10d\n", cidx, cli_frame_idx[cidx]);

				// Report head pointer
				uint8_t headpkt[5];
				headpkt[0] = 0x07;
				*((uint32_t *)(headpkt+1)) = player_frame_idx[pidx];
				sendto(sockfd, headpkt, sizeof(headpkt), 0,
					cli_addr[cidx], cli_addrlen[cidx]);

			} else if(mbuf[0] == '\x07') {
				// Frame head
				uint32_t in_offs = *((uint32_t *)(mbuf+1));
				if((int32_t)(in_offs - cli_frame_idx[cidx]) > 0) {
					cli_frame_idx[cidx] = in_offs;
				}

				//printf("Client %3d frame head: %10d\n", cidx, cli_frame_idx[cidx]);
			} else if(mbuf[0] == '\x08') {
				// State CRC32
				uint32_t frame_idx = *(uint32_t *)(mbuf+1);
				if((int32_t)(frame_idx-backlog_end) >= 0) {
					continue;
				}
				uint8_t crcpkt[31];
				crcpkt[0] = 0x08;
				uint8_t i0 = backlog[BLWRAP(frame_idx)].joy[0];
				uint8_t i1 = backlog[BLWRAP(frame_idx)].joy[1];
				backlog[BLWRAP(frame_idx)].joy[0] = 0xFF;
				backlog[BLWRAP(frame_idx)].joy[1] = 0xFF;
				*(uint32_t *)(crcpkt+1) = frame_idx;
				*(uint32_t *)(crcpkt+5) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)],
					sizeof(struct SMS), 0);
				*(uint32_t *)(crcpkt+9) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].z80,
					sizeof(struct Z80), 0);
				*(uint32_t *)(crcpkt+13) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].psg,
					sizeof(struct PSG), 0);
				*(uint32_t *)(crcpkt+17) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].vdp,
					sizeof(struct VDP), 0);
				*(uint32_t *)(crcpkt+21) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].ram,
					8192, 0);
				*(uint32_t *)(crcpkt+25) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].vdp.vram,
					16384, 0);
				crcpkt[29] = i0;
				crcpkt[30] = i1;
				backlog[BLWRAP(frame_idx)].joy[0] = i0;
				backlog[BLWRAP(frame_idx)].joy[1] = i1;

				if(memcmp(crcpkt, mbuf, sizeof(crcpkt))) {
					// Mismatched - kick!
					printf("cur frame = %10d\n", backlog_end);
					printf("exp frame = %10d\n", *(uint32_t *)(crcpkt+1));
					printf("got frame = %10d\n", *(uint32_t *)(mbuf+1));
					printf("exp CRC = %08X\n", *(uint32_t *)(crcpkt+5));
					printf("got CRC = %08X\n", *(uint32_t *)(mbuf+5));
					printf("exp Z80 CRC = %08X\n", *(uint32_t *)(crcpkt+9));
					printf("got Z80 CRC = %08X\n", *(uint32_t *)(mbuf+9));
					printf("exp PSG CRC = %08X\n", *(uint32_t *)(crcpkt+13));
					printf("got PSG CRC = %08X\n", *(uint32_t *)(mbuf+13));
					printf("exp VDP CRC = %08X\n", *(uint32_t *)(crcpkt+17));
					printf("got VDP CRC = %08X\n", *(uint32_t *)(mbuf+17));
					printf("exp RAM CRC = %08X\n", *(uint32_t *)(crcpkt+21));
					printf("got RAM CRC = %08X\n", *(uint32_t *)(mbuf+21));
					printf("exp VRAM CRC = %08X\n", *(uint32_t *)(crcpkt+25));
					printf("got VRAM CRC = %08X\n", *(uint32_t *)(mbuf+25));
					printf("exp input = %02X %02X\n", crcpkt[29], crcpkt[30]);
					printf("got input = %02X %02X\n", mbuf[29], mbuf[30]);
					uint8_t ip0 = backlog[BLWRAP(frame_idx-1)].joy[0];
					uint8_t ip1 = backlog[BLWRAP(frame_idx-1)].joy[1];
					uint8_t in0 = backlog[BLWRAP(frame_idx+1)].joy[0];
					uint8_t in1 = backlog[BLWRAP(frame_idx+1)].joy[1];
					printf(" -1 input = %02X %02X\n", ip0, ip1);
					printf(" +1 input = %02X %02X\n", in0, in1);

					fflush(stdout);
					kick_client("\x02""CRC mismatch", cidx,
						cli_addr[cidx], cli_addrlen[cidx]);
				}
			}
		}

		if(jump_out) {
			break;
		} else {
			usleep(10000);
		}
	}
#else

	// Perform keepalive
	uint64_t now = time_now();
	if(TIME_IN_ORDER(serv_keepalive_send, now)) {
		if(TIME_IN_ORDER(serv_keepalive_recv+keepalive_death, now)) {
			assert(!"connection timed out");
			abort();
		} else {
			sendto(sockfd, "\x00", 1, 0,
				serv_addr, serv_addrlen);
			serv_keepalive_send += keepalive_period;
		}
	}

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
			if(player_id < 0) {
				// Wait for messages
				if((int32_t)(backlog_end-serv_full_frame_idx) >= -1) {
					usleep(1000);
					continue;
				}
			}

			// TODO: put a condition here to compensate for clock drift
			if(false) {
				usleep(1000);
				continue;
			}
			break;
		}

		serv_keepalive_recv = now+keepalive_period;
		//printf("CMSG %d %02X\n", (int)rlen, mbuf[0]);

		if(mbuf[0] == '\x01') {
			// We connected! Again!
			printf("Acknowledged! Again!\n");
			// This time we just ignore everything
			//initial_backlog = ((uint32_t *)(mbuf+1))[0];
			//player_id = ((int32_t *)(mbuf+1))[1];
			//backlog_end = initial_backlog;
			//player_control = ((player_id >= 0 && player_id <= 31)
				//? (1<<player_id) : 0);

		} else if(mbuf[0] == '\x02') {
			// The server hates us!
			mbuf[sizeof(mbuf)-1] = '\x00';
			printf("KICKED: \"%s\"\n", mbuf+1);
			fflush(stdout);
			abort();

		} else if(mbuf[0] == '\x04') {
			// New state!
			// XXX: do we really want to bother with this?
			// It can be sent extraneously in theory,
			// but that's all really
			//printf("STATE\n");

		} else if(mbuf[0] == '\x06') {
			// New inputs!
			uint32_t in_beg = *((uint32_t *)(mbuf+1));
			uint32_t in_len = mbuf[5];
			uint32_t in_end = in_beg+in_len;
			uint8_t *p = &mbuf[6];
			/*
			printf("INPUT %10d %3d %10d %10d\n"
				, in_beg, in_len, in_end, backlog_end);
			*/

			// Ignore if it would create a gap
			if((int32_t)(in_beg-serv_full_frame_idx) > 0) {
				printf("GAP IGNORED\n");
				continue;
			}

			int32_t adj0 = (int32_t)(in_len);
			int32_t adj1 = (int32_t)(backlog_end-in_end);
			int32_t adjx = adj1-adj0;
			int32_t jitter = (int32_t)(in_end-serv_full_frame_idx);
			printf("TS %3d %10d J=%3d A0=%3d A1=%3d D=%4d\n"
				, player_id
				, in_end
				, jitter
				, adj0
				, adj1
				, adj1-adj0
				);

			// Adjust timer based on delay
			if(player_id >= 0) {
				adjx++;
				G->H.twait += (adjx*FRAME_WAIT)/128;
			}

			// Apply + check for diffs
			uint32_t resim_beg = backlog_end;
			uint8_t m0 = (player_id >= 0 ? player_masks[player_id][0] : 0x00);
			uint8_t m1 = (player_id >= 0 ? player_masks[player_id][1] : 0x00);

			for(int i = 0; i < in_len; i++) {
				int idx = BLWRAP(in_beg+i);
				uint8_t i0 = p[i*2+0];
				uint8_t i1 = p[i*2+1];
				assert((i0&m0) == (input_log[idx][0]&m0));
				assert((i1&m1) == (input_log[idx][1]&m1));
				if(i0 != input_log[idx][0] || i1 != input_log[idx][1]) {
					if(in_beg+i < resim_beg) {
						//printf("RESIM %10d %10d %10d %02X%02X %02X%02X\n", in_beg+i, resim_beg, backlog_end, i0, i1, input_log[idx][0], input_log[idx][1]);
						resim_beg = in_beg+i;
					}
				}
				input_log[idx][0] = i0;
				input_log[idx][1] = i1;
				backlog[idx].joy[0] = input_log[idx][0];
				backlog[idx].joy[1] = input_log[idx][1];
			}

			// Advance input further
			int carry_idx = BLWRAP(in_beg+in_len-1);
			int32_t in_extlen = (int32_t)(backlog_end+1-in_beg);
			for(int i = in_len; i < in_extlen; i++) {
				int idx = BLWRAP(in_beg+i);
				uint8_t i0 = input_log[idx][0];
				uint8_t i1 = input_log[idx][1];
				i0 &= m0;
				i1 &= m1;
				i0 |= input_log[carry_idx][0] & ~m0;
				i1 |= input_log[carry_idx][1] & ~m1;
				if(i0 != input_log[idx][0] || i1 != input_log[idx][1]) {
					if(in_beg+i < resim_beg) {
						//printf("RESIM %10d %10d %10d\n", in_beg+i, resim_beg, backlog_end);
						resim_beg = in_beg+i;
					}
				}
				input_log[idx][0] = i0;
				input_log[idx][1] = i1;
				backlog[idx].joy[0] = input_log[idx][0];
				backlog[idx].joy[1] = input_log[idx][1];
			}

			// Perform resim if we need to
			int32_t resim_len = (backlog_end-resim_beg);
			if(resim_len > 0) {
				printf("!!!!!! PERFORM RESIM %3d %10d %3d\n", player_id, resim_beg, resim_len);
				for(int i = 0; i < resim_len; i++) {
					assert((int32_t)(resim_beg+i+1-backlog_end) <= 0);
					int idxp = BLWRAP(resim_beg+i);
					int idxn = BLWRAP(resim_beg+i+1);
					backlog[idxp].joy[0] = input_log[idxp][0];
					backlog[idxp].joy[1] = input_log[idxp][1];
					sms_copy(&backlog[idxn], &backlog[idxp]);
					lemu_run_frame(&(G->H), &backlog[idxn], true);
					backlog[idxn].joy[0] = input_log[idxn][0];
					backlog[idxn].joy[1] = input_log[idxn][1];
				}

#if NET_DEBUG_SYNC_RESIM
				uint32_t frame_idx = backlog_end-100;
				//uint32_t frame_idx = resim_beg+1;
				uint8_t i0 = backlog[BLWRAP(frame_idx)].joy[0];
				uint8_t i1 = backlog[BLWRAP(frame_idx)].joy[1];
				backlog[BLWRAP(frame_idx)].joy[0] = 0xFF;
				backlog[BLWRAP(frame_idx)].joy[1] = 0xFF;
				uint8_t crcpkt[31];
				crcpkt[0] = 0x08;
				*(uint32_t *)(crcpkt+1) = frame_idx;
				*(uint32_t *)(crcpkt+5) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)],
					sizeof(struct SMS), 0);
				*(uint32_t *)(crcpkt+9) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].z80,
					sizeof(struct Z80), 0);
				*(uint32_t *)(crcpkt+13) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].psg,
					sizeof(struct PSG), 0);
				*(uint32_t *)(crcpkt+17) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].vdp,
					sizeof(struct VDP), 0);
				*(uint32_t *)(crcpkt+21) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].ram,
					8192, 0);
				*(uint32_t *)(crcpkt+25) = crc32_sms_net(
					(uint8_t *)&backlog[BLWRAP(frame_idx)].vdp.vram,
					16384, 0);
				crcpkt[29] = i0;
				crcpkt[30] = i1;
				backlog[BLWRAP(frame_idx)].joy[0] = i0;
				backlog[BLWRAP(frame_idx)].joy[1] = i1;

				sendto(sockfd, crcpkt, sizeof(crcpkt), 0,
					serv_addr, serv_addrlen);
#endif
			}
			//printf("IN %02X %10d\n"
				//, input_log[BLWRAP(in_end-1)][0]
				//, (int32_t)(backlog_end-resim_beg)
			//);

			//assert(!((backlog_end-in_beg) >= BACKLOG_CAP-1));

			// Check if server ahead
			if((int32_t)(in_end-serv_full_frame_idx) > 0) {
				//printf("Bump %d -> %d\n", serv_frame_idx, in_end);
				serv_full_frame_idx = in_end;
			}

			// Report head pointer
			uint8_t headpkt[5];
			headpkt[0] = 0x07;
			*((uint32_t *)(headpkt+1)) = serv_full_frame_idx;
			sendto(sockfd, headpkt, sizeof(headpkt), 0,
				serv_addr, serv_addrlen);

		} else if(mbuf[0] == '\x07') {
			// Frame head
			uint32_t in_offs = *((uint32_t *)(mbuf+1));
			if((int32_t)(in_offs - serv_frame_idx) > 0) {
				serv_frame_idx = in_offs;
			}

			//printf("Server frame head: %10d\n", serv_frame_idx);
		} else if(mbuf[0] == '\x08') {
			// State CRC32
			uint32_t frame_idx = *(uint32_t *)(mbuf+1);
			if((int32_t)(frame_idx-backlog_end) >= -1) {
				continue;
			}
			uint8_t crcpkt[31];
			crcpkt[0] = 0x08;
			uint8_t i0 = backlog[BLWRAP(frame_idx)].joy[0];
			uint8_t i1 = backlog[BLWRAP(frame_idx)].joy[1];
			backlog[BLWRAP(frame_idx)].joy[0] = 0xFF;
			backlog[BLWRAP(frame_idx)].joy[1] = 0xFF;
			*(uint32_t *)(crcpkt+1) = frame_idx;
			*(uint32_t *)(crcpkt+5) = crc32_sms_net(
				(uint8_t *)&backlog[BLWRAP(frame_idx)],
				sizeof(struct SMS), 0);
			*(uint32_t *)(crcpkt+9) = crc32_sms_net(
				(uint8_t *)&backlog[BLWRAP(frame_idx)].z80,
				sizeof(struct Z80), 0);
			*(uint32_t *)(crcpkt+13) = crc32_sms_net(
				(uint8_t *)&backlog[BLWRAP(frame_idx)].psg,
				sizeof(struct PSG), 0);
			*(uint32_t *)(crcpkt+17) = crc32_sms_net(
				(uint8_t *)&backlog[BLWRAP(frame_idx)].vdp,
				sizeof(struct VDP), 0);
			*(uint32_t *)(crcpkt+21) = crc32_sms_net(
				(uint8_t *)&backlog[BLWRAP(frame_idx)].ram,
				8192, 0);
			*(uint32_t *)(crcpkt+25) = crc32_sms_net(
				(uint8_t *)&backlog[BLWRAP(frame_idx)].vdp.vram,
				16384, 0);
			crcpkt[29] = i0;
			crcpkt[30] = i1;
			backlog[BLWRAP(frame_idx)].joy[0] = i0;
			backlog[BLWRAP(frame_idx)].joy[1] = i1;

			if(memcmp(crcpkt, mbuf, sizeof(crcpkt))) {
				// Mismatched - bail!
				printf("cur frame = %10d\n", backlog_end);
				printf("exp frame = %10d\n", *(uint32_t *)(crcpkt+1));
				printf("got frame = %10d\n", *(uint32_t *)(mbuf+1));
				printf("exp CRC = %08X\n", *(uint32_t *)(crcpkt+5));
				printf("got CRC = %08X\n", *(uint32_t *)(mbuf+5));
				printf("exp Z80 CRC = %08X\n", *(uint32_t *)(crcpkt+9));
				printf("got Z80 CRC = %08X\n", *(uint32_t *)(mbuf+9));
				printf("exp PSG CRC = %08X\n", *(uint32_t *)(crcpkt+13));
				printf("got PSG CRC = %08X\n", *(uint32_t *)(mbuf+13));
				printf("exp VDP CRC = %08X\n", *(uint32_t *)(crcpkt+17));
				printf("got VDP CRC = %08X\n", *(uint32_t *)(mbuf+17));
				printf("exp RAM CRC = %08X\n", *(uint32_t *)(crcpkt+21));
				printf("got RAM CRC = %08X\n", *(uint32_t *)(mbuf+21));
				printf("exp VRAM CRC = %08X\n", *(uint32_t *)(crcpkt+25));
				printf("got VRAM CRC = %08X\n", *(uint32_t *)(mbuf+25));
				printf("exp input = %02X %02X\n", crcpkt[29], crcpkt[30]);
				printf("got input = %02X %02X\n", mbuf[29], mbuf[30]);
				uint8_t ip0 = backlog[BLWRAP(frame_idx-1)].joy[0];
				uint8_t ip1 = backlog[BLWRAP(frame_idx-1)].joy[1];
				uint8_t in0 = backlog[BLWRAP(frame_idx+1)].joy[0];
				uint8_t in1 = backlog[BLWRAP(frame_idx+1)].joy[1];
				printf(" -1 input = %02X %02X\n", ip0, ip1);
				printf(" +1 input = %02X %02X\n", in0, in1);
				fflush(stdout);
				assert(!"CRC mismatch!");
				abort();
			}
		}
	}
#endif

	uint32_t idx = BLWRAP(backlog_end);

#ifndef SERVER
	if(player_id >= 0) {
		// Send frame input to server
		// TODO: batch these!
		uint8_t myinputpkt[6+256*2];
		uint32_t loc_offs = serv_frame_idx;
		int32_t loc_len = (backlog_end+1-loc_offs);
		assert(loc_len >= 1 && loc_len <= 255);
		myinputpkt[0] = 0x06;
		((uint32_t *)(myinputpkt+1))[0] = loc_offs;
		myinputpkt[5] = loc_len;
		uint8_t *p = &myinputpkt[6];
		for(int i = 0; i < loc_len; i++) {
			uint32_t idx = BLWRAP(loc_offs+i);
			p[i*2+0] = input_log[idx][0];
			p[i*2+1] = input_log[idx][1];
		}
		sendto(sockfd, myinputpkt, 6+loc_len*2, 0,
			serv_addr, serv_addrlen);
	}
#endif

	// Load frame backup
	backlog[idx].joy[0] = input_log[idx][0];
	backlog[idx].joy[1] = input_log[idx][1];
#if 0
	printf("DECIDE %10d %10d %02X %02X\n"
#ifdef SERVER
		, -9999999
#else
		, player_id
#endif
		, backlog_end
		, input_log[idx][0]
		, input_log[idx][1]
		);
#endif
	sms_copy(&G->current, &backlog[idx]);
	backlog_end++;

#ifndef SERVER

	// If spectator, play catchup if too far behind
	if(player_id < 0) {
		//printf("***** SPEC %d\n", backlog_end);
		if((int32_t)(serv_frame_idx-backlog_end) >= 40) {
			G->H.twait = time_now()-FRAME_WAIT*35;
		}
	}
#endif

#ifdef SERVER

	// If server... don't wait.
	G->H.twait = time_now()-FRAME_WAIT;
#endif
}

void bot_init(struct SMSGlobal *G, int argc, char *argv[])
{
	G->current.joy[0] = 0xFF;
	G->current.joy[1] = 0xFF;

	memset(input_log, 0xFF, sizeof(input_log));

	netplay_rom_crc = crc32_sms_net(G->rom, sizeof(G->rom), 0);
	//netplay_rom_crc = crc32_sms_net(G->rom, 512*1024, 0);

	net_intro_packet[0] = 0x01;
	((uint32_t *)(net_intro_packet+1))[0] = (uint32_t)sizeof(struct SMS);
	((uint32_t *)(net_intro_packet+1))[1] = netplay_rom_crc;
	((uint32_t *)(net_intro_packet+1))[2] = 0
		| (G->rom_is_banked ? 0x01 : 0)
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
	cli_keepalive_send = time_now();
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
				initial_backlog = ((uint32_t *)(mbuf+1))[0];
				player_id = ((int32_t *)(mbuf+1))[1];
				backlog_end = initial_backlog;
				serv_frame_idx = backlog_end;
				serv_full_frame_idx = backlog_end;
				player_control = ((player_id >= 0 && player_id <= 31)
					? (1<<player_id) : 0);
				printf("Acknowledged! id=%02d control=%08X\n"
					, player_id, player_control);
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
	uint8_t has_byte[sizeof(struct SMS)];
	uint32_t bytes_remain = sizeof(struct SMS);
	memset(has_byte, 0x00, sizeof(has_byte));

	printf("Sending sync requests...\n");
	for(uint32_t i = 0; i < sizeof(struct SMS); ) {
		uint32_t nlen = sizeof(struct SMS)-i;
		if(nlen > 1024) {
			nlen = 1024;
		}

		uint8_t pktbuf[11];
		pktbuf[0] = 0x03;
		*((uint32_t *)(pktbuf+1)) = backlog_end;
		*((uint32_t *)(pktbuf+5)) = i;
		*((uint16_t *)(pktbuf+9)) = nlen;
		sendto(sockfd, pktbuf, sizeof(pktbuf), 0, serv_addr, serv_addrlen);

		i += nlen;
	}

	printf("Syncing state...\n");
	int sleeps_until_resync = 10;
	while(bytes_remain > 0) {
		usleep(100000);
		bool had_packet = false;
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

			//printf("CMSG %d %02X\n", (int)rlen, mbuf[0]);

			if(mbuf[0] == '\x04') {
				had_packet = true;
				// Synced state has arrived!
				assert(rlen >= 11);
				uint32_t frame_idx = *((uint32_t *)(mbuf+1));
				uint32_t offs = *((uint32_t *)(mbuf+5));
				uint32_t len = *((uint16_t *)(mbuf+9));
				printf("Sync packet: frame_idx=%08X, offs=%08X, len=%04X\n"
					, frame_idx, offs, len);
				uint8_t *ps = mbuf+11;
				uint8_t *pd = (uint8_t *)&G->current;
				assert(offs <= sizeof(struct SMS));
				assert(offs+len <= sizeof(struct SMS));
				memcpy(pd+offs, ps, len);
				for(int i = 0; i < len; i++) {
					if(has_byte[offs+i] == 0){
						has_byte[offs+i] = 0xFF;
						bytes_remain--;
					}
				}

			} else if(mbuf[0] == '\x02') {
				// The server hates us!
				mbuf[sizeof(mbuf)-1] = '\x00';
				printf("KICKED: \"%s\"\n", mbuf+1);
				fflush(stdout);
				abort();
			}
		}

		if(!had_packet) {
			sleeps_until_resync--;
			if(sleeps_until_resync <= 0) {
				printf("Sending resync (%d remain)\n", bytes_remain);
				sleeps_until_resync = 10;

				for(uint32_t i = 0; i < sizeof(struct SMS); ) {
					uint32_t nlen = sizeof(struct SMS)-i;
					if(nlen > 1024) {
						nlen = 1024;
					}

					bool need_byte = false;
					for(int j = 0; j < nlen; j++) {
						if(has_byte[i+j] == 0) {
							need_byte = true;
							break;
						}
					}

					if(need_byte) {
						uint8_t pktbuf[11];
						pktbuf[0] = 0x03;
						*((uint32_t *)(pktbuf+1)) = backlog_end;
						*((uint32_t *)(pktbuf+5)) = i;
						*((uint16_t *)(pktbuf+9)) = nlen;
						sendto(sockfd, pktbuf, sizeof(pktbuf), 0, serv_addr, serv_addrlen);
					}

					i += nlen;
				}
			}
		}
	}

	printf("State is now synced. Let's go.\n");
	serv_keepalive_recv = serv_keepalive_send = time_now();
#endif
	printf("ROM CRC: %04X\n", netplay_rom_crc);

}

