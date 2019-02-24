#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

static u32 get_u32(void *buf)
{
	return *(u32 *)buf;
}

#define RB_BLOCK_SIZE		0x1000
#define RB_MAGIC_HARD	0x64726148 /* "Hard" */
#define RB_MAGIC_SOFT	0x74666F53 /* "Soft" */
#define RB_MAGIC_DAWN	0x6E776144 /* "Dawn" */
#define RB_MAGIC_DTS	0xEDFE0DD0 /* "DTS" */

static u32 hard_cfg_offset = 0;
static u32 soft_cfg_offset = 0;

void find_configs(void *data, unsigned int size){
	unsigned int offset;

	for (offset = 0; offset < size; offset += RB_BLOCK_SIZE) {
		u32 magic;

		magic = get_u32(data + offset);
		switch (magic) {
		case RB_MAGIC_HARD:
			printf("Hard config detected at 0x%x\n", offset);
			hard_cfg_offset = offset;
			break;

		case RB_MAGIC_SOFT:
			soft_cfg_offset = offset;
			printf("Soft config detected at 0x%x\n", offset);
			break;
		case RB_MAGIC_DTS:
			printf("DTS config detected at 0x%x\n", offset);
			break;
		}
	}
}

/*
 * ID values for Software settings
 */
#define RB_ID_TERMINATOR	0
#define RB_ID_UART_SPEED	1
#define RB_ID_BOOT_DELAY	2
#define RB_ID_BOOT_DEVICE	3
#define RB_ID_BOOT_KEY		4
#define RB_ID_CPU_MODE		5
#define RB_ID_FW_VERSION	6
#define RB_ID_SOFT_07		7
#define RB_ID_SOFT_08		8
#define RB_ID_BOOT_PROTOCOL	9
#define RB_ID_SOFT_10		10
#define RB_ID_SOFT_11		11
#define RB_ID_CPU_FREQ		12
#define RB_ID_BOOTER		13
void read_config(void *data, size_t cfg_offset, size_t buflen){
	int id;
	int len;
	unsigned char *buf = data + cfg_offset;
	/* skip magic and CRC value */
	buf += 8;
	buflen -= 8;

	while (buflen > 4){
		u32 id_and_len = get_u32(buf);
		buf += 4;
		buflen -= 4;
		id = id_and_len & 0xFFFF;
		len = id_and_len >> 16;
		printf("id = 0x%x, len = 0x%x, val = 0x%x\n", id, len, *(u32 *)buf);
		if (id == RB_ID_TERMINATOR){
			break;
		}
		if (buflen < len)
			break;
		buf += len;
		buflen -= len;
	}
}
void read_soft_config(void *data, size_t buflen){
	printf("Soft config tags:\n");
	read_config(data, soft_cfg_offset, buflen);
}

unsigned char data[2*1024*1024];
int main(void){
	int fd;
	size_t len;
	unsigned char *p = data;
	ssize_t rest = sizeof(data);
	fd = open("./spi-nor.bin", O_RDONLY);
	//fd = open("./ex1/soft.bin", O_RDONLY);
	if(fd < 0){
		perror("Can't open spi-nor.bin");
		return -1;
	}

	while(rest > 0){
		len = read(fd, p, rest);
		if(len <= 0)
			break;
		rest -= len;
		p += len;
	}
	close(fd);

	if(rest > 0){
		fprintf(stderr, "Error reading data from spi-nor.bin\n");
		return -2;
	}
	find_configs(data, sizeof(data));
	printf("\n");
	read_soft_config(data, 0x1000);

	return 0;
}
