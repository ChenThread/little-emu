#include "common.h"

uint8_t sms_rom[512*1024];

struct SMS sms_current;
struct SMS sms_prev;

int main(int argc, char *argv[])
{
	assert(argc > 1);
	FILE *fp = fopen(argv[1], "rb");
	assert(fp != NULL);
	memset(sms_rom, 0xFF, sizeof(sms_rom));
	int rsiz = fread(sms_rom, 1, sizeof(sms_rom), fp);
	assert(rsiz > 0);

	// TODO: handle other sizes
	if(rsiz <= 48*1024) {
		// Unbanked
		printf("Fill unbanked\n");
		memset(&sms_rom[rsiz], 0xFF, sizeof(sms_rom)-rsiz);

	} else {
		// Banked
		if(rsiz <= 128*1024) {
			printf("Copy 128KB -> 256KB\n");
			memcpy(&sms_rom[128*1024], sms_rom, 128*1024);
		}
		if(rsiz <= 256*1024) {
			printf("Copy 256KB -> 512KB\n");
			memcpy(&sms_rom[256*1024], sms_rom, 256*1024);
		}
	}

	// Set up SMS
	sms_init(&sms_current);
	sms_copy(&sms_prev, &sms_current);

	// Run
	for(;;) {
		struct SMS *sms = &sms_current;
		sms_run(sms, sms->timestamp + 684*313);
		sms_copy(&sms_prev, &sms_current);
		usleep(20000);
	}
	
	return 0;
}

