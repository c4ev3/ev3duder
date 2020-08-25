#ifndef EV3DUDER_UF2_H
#define EV3DUDER_UF2_H

#include <stdint.h>

#define UF2_MAGIC_1 0x0A324655
#define UF2_MAGIC_2 0x9E5D5157
#define UF2_MAGIC_3 0x0AB16F30

#define UF2_FLAG_IGNORE 0x00000001
#define UF2_FLAG_FILE   0x00001000

#define UF2_FILENAME_MAX 150
#define UF2_PAYLOAD_MAX  466
#define UF2_FILE_MAX     64*1024*1024

typedef struct {
	uint32_t magic1;
	uint32_t magic2;
	uint32_t flags;
	uint32_t file_offset;
	uint32_t data_bytes;
	uint32_t block_number;
	uint32_t block_count;
	uint32_t file_size;
	uint8_t  data[476];
	uint32_t magic3;
} uf2_block_t;

extern const char *uf2_basename(const char *fullname, int backslash_trigger);
extern int uf2_mkdir_for(const char *filename);
extern int uf2_write_into(const char *realname, uint8_t *data,
                          uint32_t data_bytes, uint32_t file_offset,
                          uint32_t file_size);

#endif // EV3DUDER_UF2_H
