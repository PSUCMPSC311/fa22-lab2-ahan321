//CMPSC 311 FA22
//LAB 2

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "mdadm.h"
#include "jbod.h"

uint32_t create_opcode(uint32_t DiskID, uint32_t BlockID, uint32_t Command, uint32_t Reserved) {
	uint32_t opcode = Reserved;
	opcode = (opcode << 6) | Command;
	opcode = (opcode << 4) | DiskID;
	opcode = (opcode << 8) | BlockID;

	return opcode;
}

int mdadm_mount(void) {
	int result = jbod_operation(create_opcode(0,0,JBOD_MOUNT,0), NULL);
	if (result == 0) {
		return 1;
	}
	else {
		return -1;
	}
 }

int mdadm_unmount(void) {
	int result = jbod_operation(create_opcode(0,0,JBOD_UNMOUNT,0), NULL);
	if (result == 0) {
		return 1;
	}
	else {
		return -1;
	}

 }

int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
	return 0;
}

