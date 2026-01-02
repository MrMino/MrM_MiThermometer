#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "vendor/common/user_config.h"
#include "drivers/8258/gpio_8258.h"

#include "flash.h"

#define MAGIC_WORD 0xABCFF123

void erase_mi_data(){
	uint8_t read[0x100];//ERASE THE MI ID to prevent blocking :D
	memset(read,0x00,0x100);
	flash_read_page(0x78000, 0x100, read);
	if((read[0] != 0xff) && (read[1] != 0xff) && (read[2] != 0xff) && (read[3] != 0xff) && (read[4] != 0xff))
		flash_erase_sector(0x78000);	
}

void init_flash(){	
	erase_mi_data();
}
