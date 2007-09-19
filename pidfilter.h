/*
 *  dvb-mpegtools for the Siemens Fujitsu DVB PCI card
 *
 * Copyright (C) 2000, 2001 Marcus Metzler 
 *            for convergence integrated media GmbH
 * Copyright (C) 2002 Marcus Metzler 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 * 

 * The author can be reached at mocm@metzlerbros.de, 
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>

#include <sys/param.h>


#ifndef __PIDFILTER_H
#define __PIDFILTER_H

#define MAX_PLENGTH 0xFFFF

	typedef struct sectionstruct {
		int  id;
		int length;
		int found;
		uint8_t payload[4096+3];
	} section;


	typedef uint32_t tflags;
#define MAXFILT 32
#define MASKL 16
	typedef struct trans_struct {
		int found;
		uint8_t packet[188];
		uint16_t pid[MAXFILT];
		uint8_t mask[MAXFILT*MASKL];
		uint8_t filt[MAXFILT*MASKL];
		uint8_t transbuf[MAXFILT*188];
		int transcount[MAXFILT];
		section sec[MAXFILT];
	        tflags is_full;
	        tflags pes_start;
	        tflags pes_started;
	        tflags pes;
	        tflags set;
	} trans;


	void init_trans(trans *p);
	int set_trans_filt(trans *p, int filtn, uint16_t pid, uint8_t *mask, 
			   uint8_t *filt, int pes);

	void clear_trans_filt(trans *p,int filtn);
	int filt_is_set(trans *p, int filtn);
	int filt_is_ready(trans *p,int filtn);

	void trans_filt(uint8_t *buf, int count, trans *p);
	void tfilter(trans *p);
	void sec_filter(trans *p, int filtn, int off);
	int get_filt_buf(trans *p, int filtn,uint8_t **buf); 
	section *get_filt_sec(trans *p, int filtn); 


	uint16_t get_pid(uint8_t *pid);
	void pes_filter(trans *p, int filtn, int off);

#endif //__PIDFILTER_H
