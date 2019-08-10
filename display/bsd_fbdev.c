/**
 * @file bsd_fbdev.c
 *
 */

/*********************
 *    INCLUDES
 *********************/
#include "bsd_fbdev.h"
#if USE_BSD_FBDEV

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <sys/consio.h>
#include <sys/fbio.h>

/*********************
 *     DEFINES
 *********************/
#ifndef FBDEV_PATH
#define FBDEV_PATH	"/dev/fb0"
#endif

/**********************
 *     TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static size_t fbsize;
static unsigned char *fbuffer;
static unsigned char **line_addr;
static int fb_fd=0;
static int bytes_per_pixel;
unsigned xres, yres;

/**********************
 *      MACROS
 **********************/

/**********************
 *  GLOBAL FUNCTIONS
 **********************/

int bsd_fbdev_init()
{
	int y;
	unsigned addr;
	struct fbtype fb;
	unsigned line_length;

	fb_fd = open(FBDEV_PATH, O_RDWR);
	if (fb_fd == -1) {
		perror("open fbdevice");
		return -1;
	}

	if (ioctl(fb_fd, FBIOGTYPE, &fb) != 0) {
		perror("ioctl(FBIOGTYPE)");
		return -1;
	}

	if (ioctl(fb_fd, FBIO_GETLINEWIDTH, &line_length) != 0) {
		perror("ioctl(FBIO_GETLINEWIDTH)");
		return -1;
	}

	xres = (unsigned) fb.fb_width;
	yres = (unsigned) fb.fb_height;

	unsigned pagemask = (unsigned) getpagesize() - 1;
	fbsize = ((line_length*yres + pagemask) & ~pagemask);

	fbuffer = (unsigned char *)mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
	if (fbuffer == (unsigned char *)-1) {
		perror("mmap framebuffer");
		close(fb_fd);
		return -1;
	}
	memset(fbuffer,0, fbsize);

	bytes_per_pixel = (fb.fb_depth + 7) / 8;
	line_addr = malloc (sizeof (char*) * yres);
	addr = 0;
	for (y = 0; y < fb.fb_height; y++, addr += line_length)
		line_addr [y] = fbuffer + addr;

	return 0;
}

int bsd_fbdev_exit()
{
	munmap(fbuffer, fbsize);
	close(fb_fd);
	free (line_addr);

	return (0);
}

void bsd_fbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
	if(fbuffer == NULL ||
			area->x2 < 0 ||
			area->y2 < 0 ||
			area->x1 > (int32_t)xres - 1 ||
			area->y1 > (int32_t)yres - 1) {
		lv_disp_flush_ready(drv);
		return;
	}

	/*Truncate the area to the screen*/
	int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
	int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
	int32_t act_x2 = area->x2 > (int32_t)xres - 1 ? (int32_t)xres - 1 : area->x2;
	int32_t act_y2 = area->y2 > (int32_t)yres - 1 ? (int32_t)yres - 1 : area->y2;

	lv_coord_t w = lv_area_get_width(area);
	unsigned char *location;
	long int byte_location = 0;
	unsigned char bit_location = 0;

	int32_t y,x;
	/* 32 or 24 bits per pixel*/
	if( bytes_per_pixel == 4 || bytes_per_pixel == 3 ){
		for(y = act_y1; y <= act_y2; y++) {
			for( x = act_x1; x<= act_x2; x++){
				location = x * bytes_per_pixel + line_addr[y];
				*location = lv_color_to32(*color_p);
				color_p++ ;
			}
		}
	}

	else if( bytes_per_pixel == 2 ){
		for(y = act_y1; y <= act_y2; y++) {
			for( x = act_x1; x<= act_x2; x++){
				location = x * bytes_per_pixel + line_addr[y];
				*location = lv_color_to16(*color_p);
				color_p++ ;
			}
		}
	}

	else if( bytes_per_pixel == 1){
		for(y = act_y1; y <= act_y2; y++) {
			for( x = act_x1; x<= act_x2; x++){
				location = x * bytes_per_pixel + line_addr[y];
				*location = lv_color_to8(*color_p);
				color_p++ ;
			}
		}
	}
	else {
		return;
	}
	lv_disp_flush_ready(drv);
}

/**********************
 *  STATIC FUNCTIONS
 **********************/

#endif
