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

#include "pidfilter.h"

#define PID_MASK_HI    0x1F
uint16_t get_pid(uint8_t *pid)
{
	uint16_t pp = 0;

	pp = (pid[0] & PID_MASK_HI)<<8;
	pp |= pid[1];

	return pp;
}


#define ADAPT_FIELD    0x20
#define PAYLOAD        0x10
#define PAY_START      0x40
/*
  conversion
*/

void init_trans(trans *p)
{
	int i;

	p->found = 0;
	p->pes = 0;
	p->is_full = 0;
	p->pes_start = 0;
	p->pes_started = 0;
	p->set = 0;

	for (i = 0; i < MASKL*MAXFILT ; i++){
		p->mask[i] = 0;
		p->filt[i] = 0;
	}
	for (i = 0; i < MAXFILT ; i++){
		p->sec[i].found = 0;
		p->sec[i].length = 0;
	}	
}

int set_trans_filt(trans *p, int filtn, uint16_t pid, uint8_t *mask, uint8_t *filt, int pes)
{
	int i;
	int off;

	if ( filtn > MAXFILT-1 || filtn<0 ) return -1;
	p->pid[filtn] = pid;
	if (pes) p->pes |= (tflags)(1 << filtn);
	else {
		off = MASKL*filtn;
		p->pes &= ~((tflags) (1 << filtn) );
		for (i = 0; i < MASKL ; i++){
			p->mask[off+i] = mask[i];
			p->filt[off+i] = filt[i];
		}
	}		
	p->set |= (tflags) (1 << filtn);
	return 0;
}

void clear_trans_filt(trans *p,int filtn)
{
	int i;

	p->set &= ~((tflags) (1 << filtn) );
	p->pes &= ~((tflags) (1 << filtn) );
	p->is_full &= ~((tflags) (1 << filtn) );
	p->pes_start &= ~((tflags) (1 << filtn) );
	p->pes_started &= ~((tflags) (1 << filtn) );

	for (i = MASKL*filtn; i < MASKL*(filtn+1) ; i++){
		p->mask[i] = 0;
		p->filt[i] = 0;
	}
	p->sec[filtn].found = 0;
	p->sec[filtn].length = 0;
}

int filt_is_set(trans *p, int filtn)
{
	if (p->set & ((tflags)(1 << filtn))) return 1;
	return 0;
}

int pes_is_set(trans *p, int filtn)
{
	if (p->pes & ((tflags)(1 << filtn))) return 1;
	return 0;
}

int pes_is_started(trans *p, int filtn)
{
	if (p->pes_started & ((tflags)(1 << filtn))) return 1;
	return 0;
}

int pes_is_start(trans *p, int filtn)
{
	if (p->pes_start & ((tflags)(1 << filtn))) return 1;
	return 0;
}

int filt_is_ready(trans *p,int filtn)
{
	if (p->is_full & ((tflags)(1 << filtn))) return 1;
	return 0;
}

void trans_filt(uint8_t *buf, int count, trans *p)
{
	int c=0;
	//fprintf(stderr,"trans_filt\n");
	

	while (c < count && p->found <1 ){
		if ( buf[c] == 0x47) p->found = 1;
		c++;
		p->packet[0] = 0x47;
	}
	if (c == count) return;
	
	while( c < count && p->found < 188 && p->found > 0 ){
		p->packet[p->found] = buf[c];
		c++;
		p->found++;
	}
	if (p->found == 188){
		p->found = 0;
		tfilter(p);
	}

	if (c < count) trans_filt(buf+c,count-c,p);
} 


void tfilter(trans *p)
{
	int l,c;
	int tpid;
	uint8_t flag,flags;
	uint8_t adapt_length = 0;
	uint8_t cpid[2];


	//	fprintf(stderr,"tfilter\n");

	cpid[0] = p->packet[1];
	cpid[1] = p->packet[2];
	tpid = get_pid(cpid);

	if ( p->packet[1]&0x80){
		fprintf(stderr,"Error in TS for PID: %d\n", 
			tpid);
	}

	flag = cpid[0];
	flags = p->packet[3];
	
	if ( flags & ADAPT_FIELD ) {
		// adaption field
		adapt_length = p->packet[4];
	}

	c = 5 + adapt_length - (int)(!(flags & ADAPT_FIELD));
	if (flags & PAYLOAD){
		for ( l = 0; l < MAXFILT ; l++){
			if ( filt_is_set(p,l) ) {
				if ( p->pid[l] == tpid) {
					if ( pes_is_set(p,l) ){
						if (cpid[0] & PAY_START){
							p->pes_started |= 
								(tflags) 
								(1 << l);
							p->pes_start |= 
								(tflags) 
								(1 << l);
						} else {
							p->pes_start &= ~ 
								((tflags) 
								(1 << l));
						}
						pes_filter(p,l,c);
					} else {
						sec_filter(p,l,c);
					}	
				}
			}
		}
	}
}	


void pes_filter(trans *p, int filtn, int off)
{
	int count,c;
	uint8_t *buf;

	if (filtn < 0 || filtn >= MAXFILT) return; 

	count = 188 - off;
	c = 188*filtn;
	buf = p->packet+off;
	if (pes_is_started(p,filtn)){
		p->is_full |= (tflags) (1 << filtn);
		memcpy(p->transbuf+c,buf,count);
		p->transcount[filtn] = count;
	}
}

section *get_filt_sec(trans *p, int filtn)
{
	section *sec;
	
	sec = &p->sec[filtn];
	p->is_full &= ~((tflags) (1 << filtn) );
	return sec;
}

int get_filt_buf(trans *p, int filtn,uint8_t **buf)
{
	*buf = p->transbuf+188*filtn;
	p->is_full &= ~((tflags) (1 << filtn) );
	return p->transcount[filtn];
}




void sec_filter(trans *p, int filtn, int off)
{
	int i,j;
	int error;
	int count,c;
	uint8_t *buf, *secbuf;
	section *sec;

	//	fprintf(stderr,"sec_filter\n");

	if (filtn < 0 || filtn >= MAXFILT) return; 

	count = 188 - off;
	c = 0;
	buf = p->packet+off;
	sec = &p->sec[filtn];
	secbuf = sec->payload;
	if(!filt_is_ready(p,filtn)){
		p->is_full &= ~((tflags) (1 << filtn) );
		sec->found = 0;
		sec->length = 0;
	}
		
	if ( !sec->found ){
		c = buf[c]+1;
		if (c >= count) return;
		sec->id = buf[c];
		secbuf[0] = buf[c];
		c++;
		sec->found++;
		sec->length = 0;
	}
	
	while ( c < count && sec->found < 3){
		secbuf[sec->found] = buf[c];
		c++;
		sec->found++;
	}
	if (c == count) return;
	
	if (!sec->length && sec->found == 3){
		sec->length |= ((secbuf[1] & 0x0F) << 8); 
		sec->length |= (secbuf[2] & 0xFF);
	}
	
	while ( c < count && sec->found < sec->length+3){
		secbuf[sec->found] = buf[c];
		c++;
		sec->found++;
	}

	if ( sec->length && sec->found == sec->length+3 ){
		error=0;
		for ( i = 0; i < MASKL; i++){
			if (i > 0 ) j=2+i;
			else j = 0;
			error += (sec->payload[j]&p->mask[MASKL*filtn+i])^
				(p->filt[MASKL*filtn+i]&
				 p->mask[MASKL*filtn+i]);
		}
		if (!error){
			p->is_full |= (tflags) (1 << filtn);
		}
		if (buf[0]+1 < c ) c=count;
	}
	
	if ( c < count ) sec_filter(p, filtn, off);

}


extern int errno;
const char * strerrno (void)
{
	return strerror(errno);
}
