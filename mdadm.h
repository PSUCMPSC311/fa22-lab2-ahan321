//CMPSC 311 SP22
//LAB 2
#ifndef MDADM_H_
#define MDADM_H_

#include <stdint.h>
#include "jbod.h"

/* Returns opcode */
uint32_t create_opcode(uint32_t DiskID, uint32_t BlockID, uint32_t Command, uint32_t Reserved);

/* Return 1 on success and -1 on failure */
int mdadm_mount(void);

/* Return 1 on success and -1 on failure */
int mdadm_unmount(void);

/* Return the number of bytes read on success, -1 on failure. */
int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf);

#endif
