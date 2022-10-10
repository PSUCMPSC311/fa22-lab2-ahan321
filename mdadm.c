//CMPSC 311 FA22
//LAB 2

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "mdadm.h"
#include "jbod.h"

/* isMounted is used to track whether the disks are mounted or not.  This is used later in the
   read function to prevent unmounted disks from being accessed. */
int isMounted = 0;

/* the create_opcode function is used to simplify the task of creating the opcode and reducing
   redundant code.*/
uint32_t create_opcode(uint32_t DiskID, uint32_t BlockID, uint32_t Command, uint32_t Reserved) {
	uint32_t opcode = Reserved;
	opcode = (opcode << 6) | Command;
	opcode = (opcode << 4) | DiskID;
	opcode = (opcode << 8) | BlockID;

	return opcode;
}

/* This function mounts the disk by calling the jbod_operation function.  isMounted is alos updated
   to reflect the changes. */
int mdadm_mount(void) {
	int result = jbod_operation(create_opcode(0,0,JBOD_MOUNT,0), NULL);
	if (result == 0) {
		isMounted = 1;
		return 1;
	}
	else return -1;
 }

/* This function unmounts the disk by calling the jbod_operation function.  isMounted is also updated
   to reflect the changes. */
int mdadm_unmount(void) {
	int result = jbod_operation(create_opcode(0,0,JBOD_UNMOUNT,0), NULL);
	if (result == 0) {
		isMounted = 0;
		return 1;
	}
	else return -1;
 }

/* This function reads the data stored between a given start and end address and places it in a
   read buffer. */
int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {

	/* The below 5 if statements checks that the inputted parameters are met and that the disk
	   is mounted. */
	if (isMounted == 0) {
		return -1;
	}

	if (read_buf == NULL && read_len == 0) {
		return 0;
	}

	if (start_addr < 0 || start_addr + read_len - 1 >= 16*JBOD_DISK_SIZE) {
		return -1;
	}

	if (read_len > 2048) {
		return -1;
	}

	if (read_buf == NULL && read_len != 0) {
		return -1;
	}

	// Determining start and end disks and blocks for use later in the program.
	uint32_t start_disk = start_addr / JBOD_DISK_SIZE;
	uint32_t end_disk = (start_addr + read_len - 1) / JBOD_DISK_SIZE;
	uint32_t start_block = start_addr % JBOD_DISK_SIZE / JBOD_BLOCK_SIZE;
	uint32_t end_block = ((start_addr + read_len - 1) % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;

	// Initial Seek to Disk
	jbod_operation(create_opcode(start_disk,0,JBOD_SEEK_TO_DISK,0),NULL);

	// Initial Seek to Block
	jbod_operation(create_opcode(0,start_block,JBOD_SEEK_TO_BLOCK,0),NULL);

	/* the c_block and c_disk variables are initialized in order to keep track of the current
	   block and disk.  The c_pointer is used to keep track of the current position within the
	   read buffer.  The read variable tracks the number of bytes read until now. */
	int c_block = start_block;
	int c_disk = start_disk;
	uint8_t* c_pointer = read_buf;
	int read = 0;

	/* This loop keeps repeating until the number of bytes read equals the length of what we want
	   to read. */
	while (read < read_len) {

		/* This block executes when reading within a singular block. */
		if (start_disk == end_disk && start_block == end_block) {
			int start_pos = start_addr % JBOD_BLOCK_SIZE;
			int end_pos = (start_addr + read_len - 1) % JBOD_BLOCK_SIZE;
			int len = ((JBOD_BLOCK_SIZE-start_pos+1)-(JBOD_BLOCK_SIZE-end_pos-1) - 1);
			uint8_t temp[JBOD_BLOCK_SIZE];
			jbod_operation(create_opcode(0,0,JBOD_READ_BLOCK,0),temp);
			int j = 0;
			for (int i = start_pos; i <= end_pos; i++) {
				c_pointer[j] = temp[i];
				j++;
			}
			c_pointer += len;
			read += len;
		}
		
		/* This block executes on the first iteration of a multi-block read, accounting for
		   the edge case where the first byte lies within a block. */
		else if (start_disk == c_disk && start_block == c_block) {	
			int start_pos = start_addr % JBOD_BLOCK_SIZE;
			int len = JBOD_BLOCK_SIZE-start_pos;
			uint8_t temp[JBOD_BLOCK_SIZE];
			jbod_operation(create_opcode(0,0,JBOD_READ_BLOCK,0),temp);
			int j = 0;
			for (int i = start_pos; i < JBOD_BLOCK_SIZE; i++) {
				c_pointer[j] = temp[i];
				j++;
			}
			c_pointer += (len);
			read += len;
		}

		/* This block executes on the last iteration of a multi-block read, accounting for
		   the edge case where the last byte lies within a block. */
		else if (end_disk == c_disk && end_block == c_block) {
			int end_pos = (start_addr + read_len - 1) % JBOD_BLOCK_SIZE;
			uint8_t temp[JBOD_BLOCK_SIZE];
			jbod_operation(create_opcode(0,0,JBOD_READ_BLOCK,0),temp);
			for (int i = 0; i <= end_pos; i++) {
				c_pointer[i] = temp[i];
			}
			c_pointer += (end_pos + 1);
			read += (end_pos + 1);
		}

		/* This block executes when the entire block needs to be read.  It accounts for the
		   majority of block reads. */
		else {
			uint8_t temp[JBOD_BLOCK_SIZE];
			jbod_operation(create_opcode(0,0,JBOD_READ_BLOCK,0),temp);
			for (int i = 0; i < JBOD_BLOCK_SIZE; i++) {
				c_pointer[i] = temp[i];
			}
			c_pointer += JBOD_BLOCK_SIZE;
			read += JBOD_BLOCK_SIZE;

		}

		/* The if statement below iterates the current block and disk based on the value of
		   the current block.  This allows multi-block and multi-disk reads to occur. */
		c_block++;
		if (c_block > 255) {
			c_block = 0;
			c_disk += 1;
			jbod_operation(create_opcode(c_disk,0,JBOD_SEEK_TO_DISK,0),NULL);
		}
	}
	return read_len;
}