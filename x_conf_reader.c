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

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t
#include "ptable.h"

#define DO_ENDIANS_CONV 0

static u32 get_u32(void *buf)
{
#if DO_ENDIANS_CONV == 1
	const uint8_t *p = buf;
	return ((uint32_t) p[3] + ((uint32_t) p[2] << 8) +
	       ((uint32_t) p[1] << 16) + ((uint32_t) p[0] << 24));
#else
	return *(u32 *)buf;
#endif
}

//#define RB_BLOCK_SIZE		0x100
#define RB_BLOCK_SIZE		0x1000
#define RB_MAGIC_HARD	0x64726148 /* "Hard" */
#define RB_MAGIC_SOFT	0x74666F53 /* "Soft" */
#define RB_MAGIC_DAWN	0x6E776144 /* "Dawn" */
#define RB_MAGIC_DTS	0xEDFE0DD0 /* "DTS" */
#define ELF_MAGIC			0x464C457F /* "ELF" */
#define MIBIB_MAGIC		0xFE569FAC /* MIBIB */

static u32 hard_cfg_offset = 0;
static u32 soft_cfg_offset = 0;
static u32 mibib_block_offset = 0;

static int dump_num = 0;
void dump_config(char *data, unsigned int size){
	int fd;
	char fname[32];
	ssize_t wr_len;
	sprintf(fname, "./dumps/%d.bin", dump_num++);
	fd = open(fname, O_WRONLY | O_CREAT);
	if(fd < 0){
		perror("Can't dump!");
		return;
	}
	wr_len = write(fd, data, size);
	printf("   dumped %zd bytes to %s\n", wr_len, fname);
}

void find_configs(void *data, unsigned int size){
	unsigned int offset;

	for (offset = 0; offset < size; offset += 1) {
		u32 magic;

		magic = get_u32(data + offset);
		switch (magic) {
		case RB_MAGIC_HARD:
			printf("Hard config detected at 0x%x\n", offset);
			hard_cfg_offset = offset;
			dump_config(data + offset, 0x1000);
			break;

		case RB_MAGIC_SOFT:
			soft_cfg_offset = offset;
			printf("Soft config detected at 0x%x\n", offset);
			dump_config(data + offset, 0x1000);
			break;
		case RB_MAGIC_DTS:
			printf("DTS config detected at 0x%x\n", offset);
			dump_config(data + offset, 0x5a90);
			break;
		case ELF_MAGIC:
			printf("ELF header detected at 0x%x\n", offset);
			break;
		case MIBIB_MAGIC:
			if(get_u32(data + offset + 0x100) != FLASH_PART_MAGIC1)
				break;
			printf("MIBIB block detected at 0x%x\n", offset);
			if(mibib_block_offset == 0) //comment it for second block
				mibib_block_offset = offset;
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

//https://github.com/forth32/qtools/blob/master/ptable.h
void read_mibib_block(void *data, size_t buflen, int in_dts){
	unsigned char *buf = data + mibib_block_offset;
	u32 *p = (void*)buf + 0x100;
	struct flash_partition_entry *part = NULL;
	int a;
	int numparts = 0;
	printf("MIBIB(0x%08x):\n", mibib_block_offset);
	if(*(p++) != FLASH_PART_MAGIC1){
		printf("  FLASH_PART_MAGIC1 - MISMATCH: 0x%x vs 0x%x\n", *(p++), FLASH_PART_MAGIC1);
		return;
	}
	printf("  FLASH_PART_MAGIC1 - OK\n");
	if(*(p++) != FLASH_PART_MAGIC2){
		printf("  FLASH_PART_MAGIC2 - MISMATCH\n");
		return;
	}
	printf("  FLASH_PART_MAGIC2 - OK\n");
	printf("  Version - %d\n", *(p++));
	numparts = *(p++);
	if(numparts > FLASH_NUM_PART_ENTRIES){
		printf("  Numparts - OWERFLOW! (%d > %d)\n", numparts, FLASH_NUM_PART_ENTRIES);
		return;
	}
	printf("  Numparts - %d\n", numparts);
	if(!in_dts){
		printf("  Parts list:\n");
		//список разделов
		part = (void*)p;
		for(a = 0; a < numparts; a++, part++){
			printf("    part %02d: %-15s offset = 0x%08x, len = 0x%x\n", a, part->name,
				part->offset * 0x10000,
				part->len * 0x10000);
		}
	}
	if(in_dts){
		char spaces[20];
		printf("\n");
		printf("  Parts list in DTS format:\n");
		part = (void*)p;
		for(a = 0; a < in_dts && a < sizeof(spaces); a++)
			spaces[a] = '\t';
		spaces[a] = '\0';
		for(a = 0; a < numparts; a++, part++){
			u32 offset = part->offset * 0x10000;
			u32 len = part->len * 0x10000;
			char *name = part->name;
			if(*name == '0' && *(name + 1) == ':')
				name += 2;
			printf("%s%s@%x {\n", spaces, name, offset);
			printf("%s\tlabel = \"%s\";\n", spaces, name);
			printf("%s\treg = <0x%08x 0x%x>;\n", spaces, offset, len);
			printf("%s\tread-only;\n", spaces);
			printf("%s};\n", spaces);
		}
	}
}

unsigned char data[17*1024*1024];
int main(void){
	int fd;
	size_t len;
	unsigned char *p = data;
	ssize_t rest = sizeof(data);
	//fd = open("./bins/rb450gx4.bin", O_RDONLY);
	//fd = open("/home/prog/openwrt/work/rb962-us/mtd/mtdX1.EU", O_RDONLY);
	fd = open("/home/prog/openwrt/work/RB5009/dumps/mtdblock2.bin", O_RDONLY);
	//fd = open("./bins/old/rb3011.bin", O_RDONLY);
	if(fd < 0){
		perror("Can't open file");
		return -1;
	}

	memset(data, 0x0, sizeof(data));
	while(rest > 0){
		len = read(fd, p, rest);
		if(len <= 0)
			break;
		rest -= len;
		p += len;
	}
	close(fd);

	/* if(rest > 0){
		fprintf(stderr, "Error reading data from file. rest = %zd\n", rest);
		return -2;
	} */
	find_configs(data, sizeof(data));
	//printf("\n");
	read_soft_config(data, 0x1000);

	printf("\n");
	//mibib_block_offset = 0x20000;
	//mibib_block_offset = 0x30000;
	if(mibib_block_offset > 0)
		read_mibib_block(data, 0x20000, 1);

	return 0;
}
