/*
 ___________________________________________________________
/				  			  								\
|				  Screen 8 side scroller					|
\___________________________________________________________/
*/
//
// Works on MSX2 and upper	
// Need external files 
//

#define __SDK_OPTIMIZATION__ 1 
// #define CPULOAD
// #define VDPLOAD
// #define WBORDER

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "header\msx_fusion.h"
#include "header\vdp_graph2.h"

#include "header\myheader.h"

static FCB file;							// Init the FCB Structure varaible
static FastVDP MyCommand;
static FastVDP MyBorder;

unsigned char page;							// VDP active page

  signed int  WLevelx;						// (X,Y) position in the level map 4096x192 of
unsigned char WLevely;						//  the 240x176 window screen 

unsigned char LevelW;						// size of the actual map in tile (initialised at level change
unsigned char LevelH;

  signed int WLevelDX;						// scrolling direction and speed F8.8 on X
  signed int WLevelDY;						// scrolling direction and speed F8.8 on Y

#define MaxLevelW 256U						// Max size of the map in tile
#define MaxLevelH 11U

#define WindowW 240U						// size of the screen in pixels
#define WindowH 176U

unsigned char newx,page;
unsigned char OldIsr[3];

unsigned char LevelMap[MaxLevelW*MaxLevelH];
unsigned char buffer[256*8];

/*
SAT = 0FA00h ; R5 = F7h;R11 = 01h
SPG = 0F000h ; R6 = 1Eh
*/

void main(void) 
{
	unsigned char rd;

	rd = ReadMSXtype();					  	// Read MSX Type

	if (rd==0) FT_errorHandler(3,"msx 1 ");	// If MSX1 got to Error !
	
	MyLoadTiles("tile0.bin");				// load tile set in segments 4,5,6,7 at 0x8000
	MyLoadMap("datamap.bin",LevelMap);		// load level map 256x11 arranged by columns
	
	// printf("\n\rLevelW: %d%\n\r",LevelW);
	// printf("LevelH: %d%\n\r",LevelH);
		
	Screen(8);						  		// Init Screen 8
	myVDPwrite(0,7);						// borders	
	VDPlineSwitch();						// 192 lines
	
	VDP60Hz();

	HMMV(0,0,256,512, 0);					// Clear all VRAM  by Byte 0 (Black)
	DisableInterrupt();
	VDPready();								// wait for command completion
	EnableInterrupt();

	ObjectsInit();							// initialise logical object 
	SprtInit();								// initialize sprites in VRAM 

	myInstISR();							// install a fake ISR to cut the overhead

	page = 0;
	mySetAdjust(0,8);						// same as myVDPwrite((0-8) & 15,18);	
	
	for (WLevelx = 0;WLevelx<0+WindowW;) {
		myFT_wait(1);		
		NewLine(WLevelx,0,WLevelx);WLevelx++;
		NewLine(WindowW-WLevelx,0,WindowW-WLevelx);	WLevelx++;
		NewLine(WLevelx,0,WLevelx);WLevelx++;
		NewLine(WindowW-WLevelx,0,WindowW-WLevelx);	WLevelx++;
	}

	WLevelx = 0;	

	MyBorder.ny = WindowH;
	MyBorder.col = 0;
	MyBorder.param = 0;
	MyBorder.cmd = opHMMV;
	
	MyCommand.ny = WindowH;
	MyCommand.col = 0;
	MyCommand.param = 0;
	MyCommand.cmd = opHMMM;
	
	
	while (myCheckkbd(7)==0xFF)
	{
		WaitLineInt();
		if ((myCheckkbd(8)==0x7F) && (WLevelx<16*(LevelW-15)))  { 
			WLevelx++;
			SatUpdate(WLevelx);			
			ScrollRight(WLevelx & 15);
		}
		else if ((myCheckkbd(8)==0xEF) && (WLevelx>0)) { 
			WLevelx--;
			SatUpdate(WLevelx);						
			ScrollLeft(WLevelx & 15);
		}
		else {
			SatUpdate(WLevelx);
		}
	}

	myISRrestore();
	Screen(0);
	Exit(0);
}

void ScrollRight(char step) __sdcccall(1) 
{
	// from left to right 
	myVDPwrite((step-8) & 15,18);			
	switch (step) {
		case 0: 
			page ^=1;							// case 0
			SetDisplayPage(page);
			MyBorder.dx = 240;
			MyBorder.nx = 15;
			MyBorder.dy = 256*page;
			myfVDP(&MyBorder);
			BorderLinesR(WindowW-1,page, WLevelx+WindowW-1);		
		break;
		default:								// case 1-15
			MyCommand.sx = 16*step;
			MyCommand.dx = MyCommand.sx - 16;;
			MyCommand.sy = 256*page;
			MyCommand.dy = MyCommand.sy ^ 256;
			MyCommand.nx = 16;
			myfVDP(&MyCommand);		
			BorderLinesR(step+WindowW-1,page,WLevelx+WindowW-1);
		break;
	}
	if (step==15) PatchPlotOneTile(step+WindowW-1-16,page^1,WLevelx+WindowW-1);		
}

void ScrollLeft(char step) __sdcccall(1)
{
	// from right to left
	myVDPwrite((step-8) & 15,18);	
	switch (step) {
		case 15: 
			page ^=1;					
			SetDisplayPage(page);				// case 15
			MyBorder.dx = 0;	
			MyBorder.nx = 15;
			MyBorder.dy = 256*page;
			myfVDP(&MyBorder);
			BorderLinesL(step,page,WLevelx);		
		break;				
		default:								// case 14-0
			MyCommand.sx = 16*step;
			MyCommand.dx = MyCommand.sx + 16;
			MyCommand.sy = 256*page;
			MyCommand.dy = MyCommand.sy ^ 256;		
			MyCommand.nx = 16;						
			myfVDP(&MyCommand);					
			BorderLinesL(step,page,WLevelx);			
		break;		
	}
	if (step==0) PatchPlotOneTile(16,page^1,WLevelx);				
}


static unsigned char *p;
__at(0xF3DF) unsigned char RG0SAV;
__at(0xF3E0) unsigned char RG1SAV;

__at(0xFFE7) unsigned char RG8SAV;
__at(0xFFE8) unsigned char RG9SAV;
__at(0xFFED) unsigned char RG14SA;


void PlotOneColumnTile(void) __sdcccall(1) 
{
__asm 
	exx
	ld	hl,(_p)
	ld	a,(hl)
	rlca	
	rlca
	and a,#3
	add a,#4
	out (#0xfe),a		; set segment in 4..7
	ld	a,(hl)
	inc	hl
	ld	(_p),hl			; save next tile
	and a,#0x3F			; tile number
	add	a,#0x80			; address of the segment
	ld	h,a				; address of the tile in the segment
	ld	l,d
	exx 

	.rept #16
	out (c),e			; set vram address in 14 bits
	out (c),d
	inc d				; new line
	exx 
	outi				; write data
	exx
	.endm
__endasm;
}

void PlotOneColumnTileAndMask(void) __sdcccall(1) 
{
__asm 
	exx
	ld	hl,(_p)
	ld	a,(hl)
	rlca	
	rlca
	and a,#3
	add a,#4
	out (#0xfe),a		; set segment in 4..7
	ld	a,(hl)
	inc	hl
	ld	(_p),hl			; save next tile
	and a,#0x3F			; tile number
	add	a,#0x80			; address of the segment
	ld	h,a				; address of the tile in the segment
	ld	l,d
	exx 
	
	.rept #16
	out (c),e			; set vram address in 14 bits
	out (c),d
	exx 
	outi				; write data
	exx
	out (c),l			; set vram address in 14 bits for border
	out (c),d
	inc d				; new line
	xor a,a				; write border
	out (#0x98),a
	.endm
__endasm;
}


void BorderLinesL(unsigned char ScrnX,char page, int MapX) __sdcccall(1) __naked
{
	ScrnX;
	page;
	MapX;
__asm

	pop bc				; get ret address
	pop de				; de = MapX
	push bc 			; save ret address

	ex af,af'			; a' = ScrnX
	ld a,l				; l = page
	add a,a
	add a,a
	ld (_RG14SA),a

	ld	c,e				; C = low(mapx)
	
	sra	d				; DE/16
	rr	e
	sra	d
	rr	e
	sra	d
	rr	e
	sra	d
	rr	e
	ld	l,e
	ld	h,d
	add	hl,hl
	add	hl,hl
	add	hl,de	
	add	hl,hl
	add	hl,de				; DE/16 * 11
	
	ld 	de,#_LevelMap	
	add	hl,de
	ld	(_p), hl
	
#ifdef	CPULOAD	
		ld	a,#0xC0				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
	
	ex af,af'				; a' = ScrnX	
	ld	e,a 				; DE vramm address for new border data
	add	a,#240 				; L = E +/- 240U according to the scroll direction
	ld	l,a 				; DL hold vramm address for blank border
	
	ld	a,c					; C = low(MapX)
	and	a,#15
	add	a,a
	add	a,a
	add	a,a
	add	a,a
	exx
	ld	d,a 				; common offeset of the address in the tile
	ld	c,#0x98 			; used by _PlotOneColumnTileAndMask
	exx
	di
	ld	a,(_RG14SA) 		; set address in vdp(14)
	out	(#0x99), a
	inc	a
	ld	(_RG14SA),a 		; save next block
	ld	a,#0x8E
	out	(#0x99), a
	ld	c,#0x99
	ld	d,#0x40
	call	_PlotOneColumnTileAndMask	; plot 4 tiles
	call	_PlotOneColumnTileAndMask
	call	_PlotOneColumnTileAndMask
	call	_PlotOneColumnTileAndMask
	ld	a,(_RG14SA) 		; set address in vdp(14)
	out	(#0x99), a
	inc	a
	ld	(_RG14SA),a 		; save next block
	ld	a,#0x8E
	out	(#0x99), a
	ld	d,#0x40
	call	_PlotOneColumnTileAndMask	; plot 4 tiles
	call	_PlotOneColumnTileAndMask
	call	_PlotOneColumnTileAndMask
	call	_PlotOneColumnTileAndMask
	ld	a,(_RG14SA) 		; set address in vdp(14)
	out	(#0x99), a
	ld	a,#0x8E
	out	(#0x99), a
	ld	d,#0x40
	call	_PlotOneColumnTileAndMask	; plot 3 tiles
	call	_PlotOneColumnTileAndMask
	call	_PlotOneColumnTileAndMask
	ld	a,#1
	out	(#0xfe),a 			; restore page 1
	
#ifdef	CPULOAD	
		xor a,a				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
	ei
	ret
__endasm;
}

void BorderLinesR(unsigned char ScrnX,char page, int MapX) __sdcccall(1) __naked
{
	ScrnX;
	page;
	MapX;
__asm

	pop bc				; get ret address
	pop de				; DE = MapX+WindowW
	push bc 			; save ret address

	ex af,af'			; a' = ScrnX
	ld a,l				; l = page
	add a,a
	add a,a
	ld (_RG14SA),a

	ld	c,e				; C = low(mapx)
	
	sra	d				; DE/16
	rr	e
	sra	d
	rr	e
	sra	d
	rr	e
	sra	d
	rr	e
	ld	l,e
	ld	h,d
	add	hl,hl
	add	hl,hl
	add	hl,de	
	add	hl,hl
	add	hl,de				; DE/16 * 11
	
	ld 	de,#_LevelMap	
	add	hl,de
	ld	(_p), hl
	
#ifdef	CPULOAD	
		ld	a,#0xC0				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
	
	ex af,af'				; a' = ScrnX	
	ld	e,a 				; DE vramm address for new border data
	sub	a,#240 				; L = E +/- 240U according to the scroll direction
	ld	l,a 				; DL hold vramm address for blank border
	
	ld	a,c					; C = low(MapX)
	and	a,#15
	add	a,a
	add	a,a
	add	a,a
	add	a,a
	exx
	ld	d,a 				; common offeset of the address in the tile
	ld	c,#0x98 			; used by _PlotOneColumnTileAndMask
	exx
	di
	ld	a,(_RG14SA) 		; set address in vdp(14)
	out	(#0x99), a
	inc	a
	ld	(_RG14SA),a 		; save next block
	ld	a,#0x8E
	out	(#0x99), a
	ld	c,#0x99
	ld	d,#0x40
	call	_PlotOneColumnTileAndMask	; plot 4 tiles
	call	_PlotOneColumnTileAndMask	
	call	_PlotOneColumnTileAndMask	
	call	_PlotOneColumnTileAndMask	
	ld	a,(_RG14SA) 		; set address in vdp(14)
	out	(#0x99), a
	inc	a
	ld	(_RG14SA),a 		; save next block
	ld	a,#0x8E
	out	(#0x99), a
	ld	d,#0x40
	call	_PlotOneColumnTileAndMask	; plot 4 tiles
	call	_PlotOneColumnTileAndMask	
	call	_PlotOneColumnTileAndMask	
	call	_PlotOneColumnTileAndMask	
	ld	a,(_RG14SA) 		; set address in vdp(14)
	out	(#0x99), a
	ld	a,#0x8E
	out	(#0x99), a
	ld	d,#0x40
	call	_PlotOneColumnTileAndMask	; plot 3 tiles
	call	_PlotOneColumnTileAndMask	
	call	_PlotOneColumnTileAndMask	
	ld	a,#1
	out	(#0xfe),a 			; restore page 1
	
#ifdef	CPULOAD	
		xor a,a				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
	ei
	ret
__endasm;
}

void NewLine(unsigned char ScrnX,char page, int MapX) __sdcccall(1) __naked
{
	ScrnX;
	page;
	MapX;

__asm
	pop bc				; get ret address
	pop de				; de = MapX
	push bc 			; save ret address

	ex af,af'			; a' = ScrnX
	ld a,l				; l = page
	add a,a
	add a,a
	ld (_RG14SA),a

	ld	c,e				; C = low(mapx)

	sra	d				; DE/16
	rr	e
	sra	d
	rr	e
	sra	d
	rr	e
	sra	d
	rr	e
	ld	l,e
	ld	h,d
	add	hl,hl
	add	hl,hl
	add	hl,de	
	add	hl,hl
	add	hl,de				; DE/16 * 11

	ld 	de,#_LevelMap	
	add	hl,de
	ld	(_p), hl
	
#ifdef	CPULOAD	
		ld	a,#0xC0				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
	ex af,af'			; a' = ScrnX	
	ld	e,a				; DE vramm address for new border data 
	
	ld	a,c				; C = low(MapX)
	and a,#15
	add a,a
	add a,a
	add a,a
	add a,a
	exx
	ld	d,a				; common offeset of the address in the tile 
	ld	c,#0x98			; used by _PlotOneColumnTile
	exx
	
	di

	ld	a,(_RG14SA)	; set address in vdp(14)
	out	(#0x99), a
	inc	a
	ld	(_RG14SA),a	; save next block
	ld	a,#0x8E
	out	(#0x99), a			

	ld 	c,#0x99
	ld	d,#0x40
	call _PlotOneColumnTile		; 4 tiles
	call _PlotOneColumnTile
	call _PlotOneColumnTile
	call _PlotOneColumnTile

	ld	a,(_RG14SA)	; set address in vdp(14)
	out	(#0x99), a
	inc	a
	ld	(_RG14SA),a	; save next block
	ld	a,#0x8E
	out	(#0x99), a			

	ld	d,#0x40
	call _PlotOneColumnTile		; 4 tiles
	call _PlotOneColumnTile
	call _PlotOneColumnTile
	call _PlotOneColumnTile

	ld	a,(_RG14SA)	; set address in vdp(14)
	out	(#0x99), a
	ld	a,#0x8E
	out	(#0x99), a			

	ld	d,#0x40
	call _PlotOneColumnTile		; 3 tiles
	call _PlotOneColumnTile		
	call _PlotOneColumnTile		
	
	ld a,#1
	out (#0xfe),a		; restore page 1
	
#ifdef	CPULOAD	
		xor a,a				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
	ei
	ret
	__endasm;}



void PatchPlotOneTile(unsigned char ScrnX,char page, int MapX) __sdcccall(1) __naked
{
	ScrnX;
	page;
	MapX;

__asm
	pop bc				; get ret address
	pop de				; DE = MapX
	push bc 			; save ret address

	ex af,af'			; a' = ScrnX
	ld a,l				; l = page
	add a,a
	add a,a
	ld (_RG14SA),a

	ld	c,e				; C = low(mapx)

	sra	d				; DE/16
	rr	e
	sra	d
	rr	e
	sra	d
	rr	e
	sra	d
	rr	e
	ld	l,e
	ld	h,d
	add	hl,hl
	add	hl,hl
	add	hl,de	
	add	hl,hl
	add	hl,de				; DE/16 * 11

	ld 	de,#_LevelMap	
	add	hl,de
	ld	(_p), hl

	ex af,af'				; a' = ScrnX	
	ld	e,a 				; DE vramm address for new border data

	ld	a,c					; C = low(MapX)
	and	a,#15
	add	a,a
	add	a,a
	add	a,a
	add	a,a

	exx
	ld	d,a 				; common offeset of the address in the tile
	ld	c,#0x98 			; used by _PlotOneColumnTile
	exx
	
	di
#ifdef	CPULOAD	
		ld	a,#0xC0				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif

	ld	a,(_RG14SA)	; set address in vdp(14)
	out	(#0x99), a
	ld	a,#0x8E
	out	(#0x99), a			

	ld	d,#0x40
	ld 	c,#0x99
	
	call _PlotOneColumnTile		; 1 tile
	
	ld a,#1
	out (#0xfe),a		; restore page 1
		
#ifdef	CPULOAD	
		xor a,a				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
	ei
	ret
__endasm;
}


void 	myVDPwrite(char data, char vdpreg) __sdcccall(1) __naked
{
	vdpreg;
	data;

__asm	
		di
		out (#0x99),a
		ld  a,#128
		or 	a,l
		out (#0x99),a            ;R#A := L
		ei 
        ret
__endasm;

}	

unsigned char myInPort(unsigned char port) __sdcccall(1) __naked
{
	port;
__asm	
		ld		c, a			;	port
		in		a, (c)			; return value in A
		ret
__endasm;
}

void myOutPort(unsigned char port,unsigned char data) __sdcccall(1) __naked
{
	port;
	data;
__asm	
		ld		c, a			; port in A
		out		(c),l			; value in L
		ret
__endasm;
}



void  	myfVDP(void *Address)  __sdcccall(1)  __naked
{
	Address;
__asm
		di
		ld  a,#32                ; Start with Reg 32
		out (#0x99),a
		ld  a,#128+#17
		out (#0x99),a            ;R#17 := 32

		ld  c,#0x9b           	 ; c=#0x9b

fvdpWait:
		ld  a,#2
		out (#0x99),a           
		ld  a,#128+#15
		out (#0x99),a
		in  a,(#0x99)
		rrca
		jp  c, fvdpWait        ; wait CE
		
.rept #15		
		OUTI    
.endm

		xor  a,a           ; set Status Register #0 for reading
		out (#0x99),a
		ld  a,#0x8f
		out (#0x99),a

        ei
        ret
 __endasm;
}

/*
char	myPoint( unsigned int X,  unsigned int Y ) __sdcccall(1) __naked
{
	X,Y;
__asm
		; hl = m
		; de = n
		
		di
		  
		ld    a,#32
		out   (#0x99),a
		ld    a,#128+#17
		out   (#0x99),a    ;R#17 := 32

		ld   c,#0x9b

		out	(c),l    ; R32 DX low byte
		out	(c),h    ; R33 DX high byte
		out	(c),e    ; R34 DY low byte
		out (c),d    ; R35 DY high byte
		  
		ld  a,#0b01000000		; Point command
		out (#0x99),a
		ld  a,#128+#46
		out (#0x99),a

		ld  a,#7     		; set Status Register #7 for reading
		out (#0x99),a
		ld  a,#0x8f
		out (#0x99),a

		in  a,(#0x99)
		ld l,a

		xor a				; set Status Register #0 for reading
		out (#0x99),a
		ld  a,#0x8f
		out (#0x99),a
		ei
		ld	a,l

		ret
__endasm;
}
*/	
/* --------------------------------------------------------- */
/* SETADJUST   Adjust screen offset                          */
/* --------------------------------------------------------- */

void mySetAdjust(signed char x, signed char y) __sdcccall(1)
{
		unsigned char value = ((x-8) & 15) | (((y-8) & 15)<<4);
		Poke(0xFFF1,value);     					// Reg18 Save
		myVDPwrite(value,18);
}

/* ---------------------------------
				FT_Wait

				Waiting 
-----------------------------------*/ 

void myFT_wait(unsigned char cicles) __sdcccall(1) __naked {
	cicles;
	
__asm

#ifdef	VDPLOAD
		push af
		ld		l,#7
		ld		a,#12
		call	_myVDPwrite		
		di
		call	_VDPready
		ei
		ld		l,#7
		xor 	a,a
		call	_myVDPwrite		
		pop af
#endif

		or	a, a
00004$:
		ret	Z
		halt
		dec	a
		jp	00004$
__endasm;
}

void WaitLineInt(void) __sdcccall(1) __naked {
	
__asm
#ifdef	VDPLOAD
		ld		l,#7
		ld		a,#12
		call	_myVDPwrite		
		di
		call	_VDPready
		ei
		ld		l,#7
		xor 	a,a
		call	_myVDPwrite		
#endif
		di
		ld  a,#1           ; set Status Register #1 for reading
		out (#0x99),a
		ld  a,#0x8f
		out (#0x99),a

WaitLI:
		in  a,(#0x99)	
		rrca
		jr	nc,WaitLI

		xor  a,a           ; set Status Register #0 for reading
		out (#0x99),a
		ld  a,#0x8f
		out (#0x99),a
		ei
		ret
__endasm;
}


/* ---------------------------------
				FT_SetName

	Set the name of a file to load
				(MSX DOS)
-----------------------------------*/ 
void FT_SetName( FCB *p_fcb, const char *p_name ) __sdcccall(1) 
{
	char i, j;
	memset( p_fcb, 0, sizeof(FCB) );
	for( i = 0; i < 11; i++ ) {
		p_fcb->name[i] = ' ';
	}
	for( i = 0; (i < 8) && (p_name[i] != 0) && (p_name[i] != '.'); i++ ) {
		p_fcb->name[i] =  p_name[i];
	}
	if( p_name[i] == '.' ) {
		i++;
		for( j = 0; (j < 3) && (p_name[i + j] != 0) && (p_name[i + j] != '.'); j++ ) {
			p_fcb->ext[j] =  p_name[i + j] ;
		}
	}
}


/* ---------------------------------
			FT_errorHandler

		  In case of Error
-----------------------------------*/ 
void FT_errorHandler(char n, char *name) __sdcccall(1) 
{
	Screen(0);
	SetColors(15,6,6);
	switch (n)
	{
	  case 1:
		  Print("\n\rFAILED: fcb_open(): ");
		  Print(name);
	  break;

	  case 2:
		  Print("\n\rFAILED: fcb_close():");
		  Print(name);
	  break;  

	  case 3:
		  Print("\n\rStop Kidding, run me on MSX2 !");
	  break;
	  
	  case 4:
		  Print("\n\rUnespected end of file:");
		  Print(name);		  
	  break; 
	}
	Exit(0);
}


/* ---------------------------------
		  FT_LoadSc8Image

	Load a SC8 Picture and put datas
  on screen, begining at start_Y line
-----------------------------------*/ 
/*
char myFT_LoadSc8Image(char *file_name, unsigned int start_Y, char *buffer) __sdcccall(1) 
{
	unsigned int rd,nbl;
	FT_SetName( &file, file_name );
	if(fcb_open( &file ) != FCB_SUCCESS) {
		FT_errorHandler(1, file_name);
		return (0);
	}
	rd = fcb_read( &file, buffer, 8 );  		// Skip 7 first bytes of the file FIX STRANGE OFFSET
	while(rd!=0) {
		rd = fcb_read( &file, buffer, 256U*192U );	// 192 lines of 256 bytes
		if (rd!=0) {
			nbl=rd/256;
			HMMC(buffer, 0,start_Y,256,nbl ); 	// Move the buffer to VRAM. mbl lines x 256 pixels	 from memory
			start_Y += nbl; 
		}
	}
	if( fcb_close( &file ) != FCB_SUCCESS ) {
		FT_errorHandler(2, file_name);
		return (0);
	}
	return(1);
}
*/

////////////////////////////////////////////////////////////

char MyLoadTiles(char *file_name) __sdcccall(1)
{
	unsigned int rd;
	unsigned char n,m;
		
	for (n=0;n<4;n++) {
	
		FT_SetName( &file, file_name );
		if(fcb_open( &file ) != FCB_SUCCESS) {
			FT_errorHandler(1, file_name);
			return (0);
		}

		for (m=0;m<8;m++) {							// 64 tiles of 256 bytes
			
			rd = fcb_read( &file, buffer, 256U*8U );	
			myOutPort(0xFE,4+n);
			memmove((unsigned char*) 0x8000+256*8*m,buffer,256U*8U);
	
			if(rd != 256U*8U) {
				FT_errorHandler(4, file_name);
				return (0);
			}
		}

		if( fcb_close( &file ) != FCB_SUCCESS ) {
			FT_errorHandler(2, file_name);
			return (0);
		}
		
		file_name[4]++;
	}
	return(1);
}


char MyLoadMap(char *file_name,unsigned char* p ) __sdcccall(1)
{
	unsigned int rd;

	FT_SetName( &file, file_name );
	if(fcb_open( &file ) != FCB_SUCCESS) {
		FT_errorHandler(1, file_name);
		return (0);
	}

	LevelW = MaxLevelW;
	LevelH = MaxLevelH;

	rd = fcb_read( &file, &LevelW, 1);		// Level Width
	rd = fcb_read( &file, &LevelH, 1);		// Level Height

	rd = fcb_read( &file, p, 256U*11U );	// 256 columns x 11 rows

	if(rd != 256U*11U) {
		FT_errorHandler(4, file_name);
		return (0);
	}

	if( fcb_close( &file ) != FCB_SUCCESS ) {
		FT_errorHandler(2, file_name);
		return (0);
	}
	return (1);	
}


void myISR(void) __sdcccall(1) __naked
{
__asm 
	push af
#ifdef WBORDER	
	ld	a,#0x03			// blue
	out	(#0x99),a
	ld	a,#128+#7
	out	(#0x99),a 
#endif
	xor  a,a           ; set Status Register #0 for reading
	out (#0x99),a
	ld  a,#0x8f
	out (#0x99),a

	in  a,(#0x99)		; mimimum ISR
#ifdef WBORDER	
	xor	a,a
	out	(#0x99),a
	ld	a,#128+#7
	out	(#0x99),a 
#endif

	pop	af
	ei 
	ret
__endasm;
}

void myInstISR(void) __sdcccall(1) __naked
{
	myVDPwrite(WindowH,19);
	// RG0SAV |= 16;
	// myVDPwrite(RG0SAV,0);
	
__asm 
	ld	hl,#0x38
	ld  de,#_OldIsr
	ld 	bc,#3
	ldir
	di
	ld	a,#0xC3
	ld  (#0x38+#1),a
	ld	hl,#_myISR
	ld  (#0x38+#1),hl
	ei
	ret
__endasm;
}

void myISRrestore(void) __sdcccall(1) __naked
{
	RG0SAV &= 0xEF;
	myVDPwrite(RG0SAV,0);
	
__asm 
	ld	hl,#_OldIsr
	ld  de,#0x38
	ld 	bc,#3
	di 
	ldir
	ei
	ret
__endasm;
}

	

unsigned char myCheckkbd(unsigned char nrow) __sdcccall(1) __naked
{
	nrow;
__asm 
; // Line Bit_7 Bit_6 Bit_5 Bit_4 Bit_3 Bit_2 Bit_1 Bit_0
; // 0 "7" "6" "5" "4" "3" "2" "1" "0"
; // 1 ";" "]" "[" "\" "=" "-" "9" "8"
; // 2 "B" "A" ??? "/" "." "," "'" "`"
; // 3 "J" "I" "H" "G" "F" "E" "D" "C"
; // 4 "R" "Q" "P" "O" "N" "M" "L" "K"
; // 5 "Z" "Y" "X" "W" "V" "U" "T" "S"
; // 6 F3 F2 F1 CODE CAP GRAPH CTRL SHIFT
; // 7 RET SEL BS STOP TAB ESC F5 F4
; // 8 RIGHT DOWN UP LEFT DEL INS HOME SPACE

; checkkbd:
		ld	e,a
		di
		in	a,(#0xaa)
		and a,#0b11110000			; upper 4 bits contain info to preserve
		or	a,e
		out (#0xaa),a
		in	a,(#0xa9)
		ld	l,a
		ei
		ret
__endasm;
}

void sprite_patterns(void) __naked
{
__asm
    // .incbin "matlab\sprites\linktest_frm.bin"
    // .incbin "matlab\sprites\linktest_all_frm.bin"
    .incbin "matlab\sprites\knight_frm.bin"
__endasm;	
}

void sprite_colors(void) __naked
{
__asm
    // .incbin "matlab\sprites\linktest_clr.bin"
    // .incbin "matlab\sprites\linktest_all_clr.bin"
    .incbin "matlab\sprites\knight_clr.bin"
__endasm;	
}



struct {
	signed int x;
	signed int y;
	unsigned char type;
	unsigned char frame;
	unsigned char status;
} object[MaxObjNum];


void ObjectsInit(void) {
	unsigned char t;
	for (t=0;t<MaxObjNum;t++)
	{
		object[t].x = t*LevelW*16/MaxObjNum;
		object[t].y = LevelH*16/2;
		object[t].frame = t;
	}
}

unsigned char sprite_sat[] = {
     2+16, 64, 8,255,   2+16, 64, 12,255,
	 2+00, 64, 0,255,   2+00, 64,  4,255,
	 
    34+00, 96, 16,255, 34+00, 96,20,255,
    34+16, 96, 24,255, 34+16, 96,28,255,

    66+00,128, 32,255, 66+00,128,36,255,
    66+16,128, 40,255, 66+16,128,44,255,

    98+00,128, 48,255, 98+00,128,52,255,
    98+16,128, 56,255, 98+16,128,60,255,

     2+00,160, 64,255,  2+00,160,68,255,
     2+16,160, 72,255,  2+16,160,76,255,

    34+00,160, 80,255, 34+00,160,84,255,
    34+16,160, 88,255, 34+16,160,92,255,

    66+00,160, 96,255, 66+00,160,100,255,
    66+16,160,104,255, 66+16,160,108,255,

    98+00,160,112,255, 98+00,160,116,255,
    98+16,160,120,255, 98+16,160,124,255,
	
	216
};
 
void SatUpdate(int MapX) __sdcccall(1)
{
	MapX;
__asm
	ex de,hl
__endasm;	

#ifdef CPULOAD
	myVDPwrite(255,7);
#endif

__asm

	ld a,#0
	ld	hl,#0x0FA00 
	call SetVdp_Write
	ld 	hl,#_sprite_sat
	ld	a,#15
	and a,e
	ld  e,a
	ld  c,#0x98
		
	.rept 32
	outi
	ld a,e
	add a,(hl)
	out (0x98),a
	inc hl
	outi
	outi
	.endm

__endasm;	
#ifdef CPULOAD
	myVDPwrite(0,7);
#endif

}

void SprtInit(void) __sdcccall(1) __naked
{
	RG1SAV |= 2;
	myVDPwrite(RG1SAV,1);
	RG8SAV |= 32;
	myVDPwrite(RG8SAV,8);
	
__asm
	ld a,#0
	ld	hl,#0x0F000
	call SetVdp_Write
	ld 	hl,#_sprite_patterns
	ld	de,#12*#4*#32
	call Vram_Write
	
	ld a,#0
	ld	hl,#0x0F800
	call SetVdp_Write
	ld 	hl,#_sprite_colors
	ld	de,#8*#4*#16
	call Vram_Write

	ld a,#0
	ld	hl,#0x0FA00 
	call SetVdp_Write
	ld 	hl,#_sprite_sat
	ld	de,#128
	call Vram_Write

	ret 


Vram_Write:
	ld  c,#0x98
00005$:
	outi
	dec de
	ld a,d
	or a,e
	jr nz,00005$
	ret
					; Set VDP address counter to write from address AHL (17-bit)
					; Enables the interrupts
SetVdp_Write:
    rlc h
    rla
    rlc h
    rla
    srl h
    srl h
    di
    out (#0x99),a
    ld a,#0x8E
    out (#0x99),a
    ld a,l
    out (#0x99),a
    ld a,h
    or a,#0x40
    ei
    out (#0x99),a
    ret
__endasm;	
}