/*
 * mmio.c: Ruby Extension to read/write from/to any location in memory.
 *
 *  Copyright (C) 2008, Doug Bradbury (bradbury@8thlight.com)
 *
 * This extension is based on the devmem2 program availible under the
 * same license:  http://www.simtec.co.uk/appnotes/AN0014/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <ruby.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
  
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
 
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)


VALUE method_read(VALUE self, VALUE address, VALUE access_type_rb);
VALUE method_write(VALUE address, VALUE size, VALUE data, VALUE self);

//Helpers
static void* virtual_address(int fd, off_t target);
static int read_access_type(VALUE access_type_rb);
static off_t read_target(VALUE address);
static int open_dev_mem();

void Init_mmio() {
 	VALUE module = rb_define_module("MemoryMappedIO");
	rb_define_method(module, "read", method_read, 2);
	rb_define_method(module, "write", method_write, 3);
}

VALUE method_read(VALUE self, VALUE address_rb, VALUE access_type_rb)
{	
	unsigned long read_result;
    int access_type = read_access_type(access_type_rb);
    off_t target = read_target(address_rb);
    int fd = open_dev_mem();
    void* virt_addr = virtual_address(fd, target);
    
    switch(access_type) {
		case 'b':
			read_result = *((unsigned char *) virt_addr);
			break;
		case 'h':
			read_result = *((unsigned short *) virt_addr);
			break;
		case 'w':
			read_result = *((unsigned long *) virt_addr);
			break;
		default:
			fprintf(stderr, "Illegal data type '%c'.\n", access_type);
			exit(2);
	}

    return ULONG2NUM(read_result);
}

VALUE method_write(VALUE self, VALUE address_rb, VALUE access_type_rb, VALUE data_rb) 
{
    unsigned long read_result;
    int access_type = read_access_type(access_type_rb);
    off_t target = read_target(address_rb);
    int fd = open_dev_mem();
    void* virt_addr = virtual_address(fd, target);

    unsigned long writeval = NUM2ULONG(data_rb);
	switch(access_type) {
		case 'b':
			*((unsigned char *) virt_addr) = writeval;
			read_result = *((unsigned char *) virt_addr);
			break;
		case 'h':
			*((unsigned short *) virt_addr) = writeval;
			read_result = *((unsigned short *) virt_addr);
			break;
		case 'w':
			*((unsigned long *) virt_addr) = writeval;
			read_result = *((unsigned long *) virt_addr);
			break;
	}
	return ULONG2NUM(read_result);
}


static void* virtual_address(int fd, off_t target)
{   
    void *map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) FATAL;
    return (map_base + (target & MAP_MASK));
}

static int read_access_type(VALUE access_type_rb)
{
    return (int)tolower(*StringValuePtr(access_type_rb));
}

static off_t read_target(VALUE address)
{
    return NUM2ULONG(address);
}

static int open_dev_mem() 
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd == -1) FATAL;
    return fd;
}
