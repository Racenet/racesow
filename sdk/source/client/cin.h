/*
   Copyright (C) 2002-2003 Victor Luchits

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 */

#define RoQ_HEADER1		4228
#define RoQ_HEADER2		-1
#define RoQ_HEADER3		30

#define RoQ_FRAMERATE		30

#define RoQ_INFO		0x1001
#define RoQ_QUAD_CODEBOOK	0x1002
#define RoQ_QUAD_VQ		0x1011
#define RoQ_SOUND_MONO		0x1020
#define RoQ_SOUND_STEREO	0x1021

#define RoQ_ID_MOT		0x00
#define RoQ_ID_FCC		0x01
#define RoQ_ID_SLD		0x02
#define RoQ_ID_CCC		0x03

typedef struct
{
	qbyte y[4], u, v;
} roq_cell_t;

typedef struct
{
	qbyte idx[4];
} roq_qcell_t;

typedef struct
{
	unsigned short id;
	unsigned int size;
	unsigned short argument;
} roq_chunk_t;

typedef struct
{
	char *name;

	roq_chunk_t chunk;
	roq_cell_t cells[256];
	roq_qcell_t qcells[256];

	qbyte *vid_buffer;
	qbyte *vid_pic[2];

	qboolean new_frame;

	int s_rate;
	int s_width;
	int s_channels;

	int width;
	int height;

	int file;
	int headerlen;

	unsigned int time;              // Sys_Milliseconds for first cinematic frame
	unsigned int frame;

	qbyte *pic;
	qbyte *pic_pending;

	mempool_t *mempool;
} cinematics_t;

void RoQ_Init( void );
void RoQ_ReadChunk( cinematics_t *cin );
void RoQ_SkipChunk( cinematics_t *cin );
void RoQ_ReadInfo( cinematics_t *cin );
void RoQ_ReadCodebook( cinematics_t *cin );
qbyte *RoQ_ReadVideo( cinematics_t *cin );
void RoQ_ReadAudio( cinematics_t *cin );
