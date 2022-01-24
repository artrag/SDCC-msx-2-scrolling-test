/*
 ___________________________________________________________
/				  			  								\
|				  E X A M P L E	 C O D E					|
\___________________________________________________________/
*/
//
// Works on MSX2 and upper	
// Need external files 
//

#define __SDK_OPTIMIZATION__ 1 
// #define CPULOAD
// #define VDPLOAD

#include <string.h>
#include "header\msx_fusion.h"
#include "header\vdp_graph2.h"

#include "header\myheader.h"

#define DELAY 1

static FCB file;							// Init the FCB Structure varaible
static FastVDP MyCommand;
static FastVDP MyBorder;

unsigned char page;							// VDP active page

  signed int  WLevelx;						// (X,Y) position in the level map 4096x192 of
unsigned char WLevely;						//  the 240x176 window screen 

#define LevelW 256U							// size of the map in tile
#define LevelH 11U

#define WindowW 240U						// size of the screen in pixels
#define WindowH 176U

unsigned char newx,page;
unsigned char OldIsr[3];

unsigned char LevelMap[LevelW*LevelH];
unsigned char buffer[256*8];

void main(void) 
{
	unsigned char n,rd;

	rd = ReadMSXtype();					  	// Read MSX Type

	if (rd==0) FT_errorHandler(3,"msx 1 ");	// If MSX1 got to Error !
	
	MyLoadTiles("tile0.bin");				// load tile set in segments 4,5,6,7 at 0x8000
	MyLoadMap("datamap.bin",LevelMap);		// load level map 256x11 arranged by columns
	
	Screen(8);						  		// Init Screen 8
	myVDPwrite(0,7);						// borders	
	VDPlineSwitch();						// 192 lines
	
	VDP60Hz();

	HMMV(0,0,256,512, 0);					// Clear all VRAM  by Byte 0 (Black)
	DisableInterrupt();
	VDPready();								// wait for command completion
	EnableInterrupt();

	myInstISR();							// install a fake ISR to cut the overhead
		

	page = 0;

	mySetAdjust(15,8);						// same as myVDPwrite((15-8) & 15,18);	

	WLevelx = 1024+64+16;							// initialise visible page
	for (n=15;n<255;n++) {
		myFT_wait(1);
		NewLinesL(n,page);
		WLevelx++;
	}
	WLevelx = 1024+64+16;							// initialise visible page

	while (myCheckkbd(7)==0xFF)
	{
		// init left to right scrolling
			
		MyCommand.nx = 16;
		MyCommand.ny = WindowH;
		MyCommand.col = 0;
		MyCommand.param = 0;
		MyCommand.cmd = opHMMM;
		
		MyBorder.dx = 0;	
		MyBorder.nx = 15;
		MyBorder.ny = WindowH;
		MyBorder.col = 0;
		MyBorder.param = 0;
		MyBorder.cmd = opHMMV;


		while (myCheckkbd(8)==0xFF)
		{
			myFT_wait(DELAY);
			SetDisplayPage(page);
			
			myVDPwrite((15-8) & 15,18);		

			MyBorder.dy = 256*(page^1);
			myfVDP(&MyBorder);

			NewBorderLinesL(15,page);
			WLevelx--;
			
			MyCommand.nx = 16;
			MyCommand.sx = 224;		
			MyCommand.dx = 240;	
			MyCommand.sy = 256*page;
			MyCommand.dy = MyCommand.sy ^ 256;		
			
			for (n=14;n>0;n--) {
				myFT_wait(DELAY);
				myVDPwrite((n-8) & 15,18);	
				myfVDP(&MyCommand);					
				MyCommand.sx -= 16;
				MyCommand.dx -= 16;
				NewBorderLinesL(n,page);				
				WLevelx--;
			}

			myFT_wait(DELAY);
			myVDPwrite((0-8) & 15,18);		
			MyCommand.nx = 15;
			MyCommand.sx = 1;
			MyCommand.dx = 17;
			myfVDP(&MyCommand);					
			NewBorderLinesL(0,page);
			NewLinesL(16,page^1);				// patch
			WLevelx--;
			
			page ^=1;		
		}

		while (myCheckkbd(8)!=0xFF) {}
		page ^=1;	
		
		

	/*	
		newx = 0;
		page = 0;

		mySetAdjust(0,8);						// same as myVDPwrite((0-8) & 15,18);	
		
		WLevelx = -WindowW;						// initialise visible page
		for (n=0;n<240;n++) {
			NewLinesR(n,page);
			WLevelx++;
		}
	*/
	
		// init right to left scrolling
	
		MyCommand.ny = WindowH;
		MyCommand.col = 0;
		MyCommand.param = 0;
		MyCommand.cmd = opHMMM;
		
		MyBorder.dx = 240;
		MyBorder.nx = 16;
		MyBorder.ny = WindowH;
		MyBorder.col = 0;
		MyBorder.param = 0;
		MyBorder.cmd = opHMMV;
		

		while (myCheckkbd(8)==0xFF)
		{
			myFT_wait(DELAY);
			SetDisplayPage(page);
			
			myVDPwrite((0-8) & 15,18);						
			
			MyBorder.dy = 256*(page^1);
			myfVDP(&MyBorder);

			NewLinesR(WindowW-1,page);
			WLevelx++;

			MyCommand.nx = 16;
			MyCommand.sx = 16;
			MyCommand.dx = 0;
			MyCommand.sy = 256*page;
			MyCommand.dy = MyCommand.sy ^ 256;
			for (n=1;n<15;n++) {
				myFT_wait(DELAY);
				myVDPwrite((n-8) & 15,18);						
				myfVDP(&MyCommand);					
				MyCommand.sx += 16;
				MyCommand.dx += 16;
				NewBorderLinesR(n+WindowW-1,page);
				WLevelx++;			
			}
			myFT_wait(DELAY);
			myVDPwrite((15-8) & 15,18);		
			
			MyCommand.nx = 14;
			myfVDP(&MyCommand);
			NewBorderLinesR(16+WindowW-2,page);				
			NewLinesR(WindowW-2,page^1);

			WLevelx++;	
			page ^=1;		
		}
		while (myCheckkbd(8)!=0xFF) {}
		page ^=1;			
	}
	
	myFT_wait(120);

	myISRrestore();
	Screen(0);
	Exit(0);
}

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

void PlotOneColumnBorder(void) __sdcccall(1) 
{
	__asm
#ifdef	CPULOAD	
		ld	a,#0xC0				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
		ld 	a,(_vaddrL)
		ld	e,a				; DE vramm address for new border data 
		exx
		ld  a,(_WLevelx)
		and a,#15
		add a,a
		add a,a
		add a,a
		add a,a
		ld	d,a				; common offeset of the address in the tile 
		ld	c,#0x98			; used by _PlotOneColumnTile
		exx
		
		di

		ld	a,(_vaddrH3bit)	; set address in vdp(14)
		out	(#0x99), a
		inc	a
		ld	(_vaddrH3bit),a	; save next block
		ld	a,#0x8E
		out	(#0x99), a			

		ld	d,#0x40
		ld 	bc,#0x0499
00004$:		
		call _PlotOneColumnTile
		djnz 00004$			; 4 tiles

		ld	a,(_vaddrH3bit)	; set address in vdp(14)
		out	(#0x99), a
		inc	a
		ld	(_vaddrH3bit),a	; save next block
		ld	a,#0x8E
		out	(#0x99), a			

		ld	d,#0x40
		ld 	b,#4
00005$:		
		call _PlotOneColumnTile
		djnz 00005$			; 4 tiles

		ld	a,(_vaddrH3bit)	; set address in vdp(14)
		out	(#0x99), a
		inc	a
		ld	(_vaddrH3bit),a	; save next block
		ld	a,#0x8E
		out	(#0x99), a			

		ld	d,#0x40
		ld 	b,#3
00006$:		
		call _PlotOneColumnTile
		djnz 00006$			; 3 tiles
		
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

void PlotOneColumnBorderAndMask(void) __sdcccall(1) 
{
	__asm
#ifdef	CPULOAD		
		ld a,#0xC0				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
		ld 	a,(_vaddrL)
		ld	e,a				; DE vramm address for new border data 
		add a,l				; L is +/- WindowW according to the scroll direction
		ld	l,a				; D and L hold vramm address for blank border
		exx
		ld  a,(_WLevelx)
		and a,#15
		add a,a
		add a,a
		add a,a
		add a,a
		ld	d,a				; common offeset of the address in the tile 
		ld	c,#0x98			; used by	_PlotOneColumnTileAndMask	
		exx
		
		di
		
		ld	a,(_vaddrH3bit)	; set address in vdp(14)
		out	(#0x99), a
		inc	a
		ld	(_vaddrH3bit),a	; save next block
		ld	a,#0x8E
		out	(#0x99), a			
		ld	d,#0x40
		ld 	bc,#0x0499
00004$:		
		call _PlotOneColumnTileAndMask
		djnz 00004$			; 4 tiles

		ld	a,(_vaddrH3bit)	; set address in vdp(14)
		out	(#0x99), a
		inc	a
		ld	(_vaddrH3bit),a	; save next block
		ld	a,#0x8E
		out	(#0x99), a			
		ld	d,#0x40
		ld 	b,#4
00005$:		
		call _PlotOneColumnTileAndMask
		djnz 00005$			; 4 tiles

		ld	a,(_vaddrH3bit)	; set address in vdp(14)
		out	(#0x99), a
		inc	a
		ld	(_vaddrH3bit),a	; save next block
		ld	a,#0x8E
		out	(#0x99), a			

		ld	d,#0x40
		ld 	b,#3
00006$:		
		call _PlotOneColumnTileAndMask
		djnz 00006$			; 3 tiles
		
		ld a,#1
		out (#0xfe),a		; restore page 1
#ifdef	CPULOAD	
		xor a,a				; color test
		out	(#0x99), a
		ld	a,#0x87
		out	(#0x99), a			
#endif
		ei
	__endasm;
}


static unsigned char *p;
static unsigned char vaddrL,vaddrH,vaddrH3bit;


void NewBorderLinesR(unsigned char x,char page) __sdcccall(1) __naked
{
	vaddrH3bit = page*4;
	vaddrL = x;	
	
	p = &LevelMap[(WLevelx+WindowW)/16 * LevelH];		// pointer to the column in the level map
	
__asm
		ld	l,#-240
		jp _PlotOneColumnBorderAndMask
__endasm;
}

void NewBorderLinesL(unsigned char x,char page) __sdcccall(1) __naked
{
	vaddrH3bit = page*4;
	vaddrL = x;	
	
	p = &LevelMap[(WLevelx)/16 * LevelH];		// pointer to the column in the level map
	
__asm
		ld	l,#240
		jp _PlotOneColumnBorderAndMask
__endasm;
}


void NewLinesR(unsigned char x,char page) __sdcccall(1) __naked
{
	vaddrH3bit = page*4;
	vaddrL = x;	
	p = &LevelMap[(WLevelx+WindowW)/16 * LevelH];		// pointer to the column in the level map
__asm
		jp _PlotOneColumnBorder
__endasm;
}

void NewLinesL(unsigned char x,char page) __sdcccall(1) __naked
{
	vaddrH3bit = page*4;
	vaddrL = x;	
	p = &LevelMap[(WLevelx)/16 * LevelH];		// pointer to the column in the level map
__asm
		jp _PlotOneColumnBorder
__endasm;
}

/*
static unsigned char k;
void NewBorderLines(unsigned char x,char page) __sdcccall(1) __naked
{

	k = page*4;
	vaddrL = x;

	// p = &ScreenSlice[192*(newx & 255)];
	
	p = (unsigned char*) 0x8000 + 256*(newx & 63);
	
	newx++;

__asm
		
		ld	c,#3
		ld	a,(_k)
		ld	e,a
		ld 	a,(_vaddrL)
		ld	l,a		
		sub a,#240

		exx 
		ld	e,a
		ld	c,#0x98
		ld	hl,(_p)
		exx 		
		
		di
00002$:
;		ld	a,c				; color test
;		out	(#0x99), a
;		ld	a,#0x87
;		out	(#0x99), a			

		ld	a,e				; set address in vdp(14)
		out	(#0x99), a
		inc	e
		ld	a,#0x8E
		out	(#0x99), a			

		ld	h,#0x40
		ld 	b,h
00001$:
		ld 	a,l				; set vram address in 14 bits
		out (#0x99),a
		ld 	a,h
		out (#0x99),a
		
		exx 
		outi				; write data

		ld 	a,e				; set vram address in 14 bits
		exx
		out (#0x99),a
		ld 	a,h
		out (#0x99),a

		xor a,a
		out (#0x98),a

		inc h
		djnz 00001$


		dec c
		jr nz,00002$
		
;		ld a,#255				; color test
;		out	(#0x99), a
;		ld	a,#0x87
;		out	(#0x99), a			

		ei
		ret
__endasm;

}
*/

void 	myVDPwrite(char data, char vdpreg) __sdcccall(1) __naked
{
	vdpreg;
	data;

__asm	
		di
		out (#0x99),a
		ld  a,#128
		or l
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


void readLine( unsigned char *p) __sdcccall(1) __naked
{
	p;
__asm	
		ld		bc,#0x0098		; read 256 points from port 98h
		ld		de,#191
00005$:
		ini						; and put tham as a column of matrix at HL
		add hl,de
		jr	nz,00005$
		ret
__endasm;
	
}


void  	myfVDP(void *Address)  __z88dk_fastcall  __naked
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
		ld		l,#0x07
		ld		a,#12
		call	_myVDPwrite		
		di
		call	_VDPready
		ei
		ld		l,#0x07
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

/* ---------------------------------
				FT_SetName

	Set the name of a file to load
				(MSX DOS)
-----------------------------------*/ 
void FT_SetName( FCB *p_fcb, const char *p_name ) 
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
void FT_errorHandler(char n, char *name)
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
char myFT_LoadSc8Image(char *file_name, unsigned int start_Y, char *buffer)
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


/*	
void NewBlank(unsigned char x,char page) __sdcccall(1) __naked
{
	// myVDPwrite(12,7);
	
	k = page*4;
	vaddrL  = x;

__asm
		ld	c,#3
		ld	a,(_k)
		ld	e,a
		ld 	a,(_vaddrL)
		ld	l,a		
		di		
00002$:
		ld	a,e
		out	(#0x99), a
		inc	e
		ld	a,#0x8E
		out	(#0x99), a			

		ld	h,#0x40
		ld 	b,h
00001$:
		ld 	a,l
		out (#0x99),a
		ld 	a,h
		out (#0x99),a
		
		xor a,a
		out (#0x98),a
		
		inc h
		djnz 00001$

		dec c
		jr nz,00002$
		ei
		ret
__endasm;
	// myVDPwrite(0,7);
}



void NewLine(unsigned char x,char page) __sdcccall(1) __naked
{
	
	k = page*4;
	vaddrL = x;

	// p = &ScreenSlice[192*(newx & 255)];
	
	p = (unsigned char*) 0x8000 + 256*(newx & 63);

	newx++;

__asm
		exx 
		ld	c,#0x98
		ld	hl,(_p)
		exx 		
		
		ld	c,#3
		ld	a,(_k)
		ld	e,a
		ld 	a,(_vaddrL)
		ld	l,a		
		di
00002$:
;		ld	a,c				; color test
;		out	(#0x99), a
;		ld	a,#0x87
;		out	(#0x99), a			

		ld	a,e				; set address in vdp(14)
		out	(#0x99), a
		inc	e
		ld	a,#0x8E
		out	(#0x99), a			

		ld	h,#0x40
		ld 	b,h
00001$:
		ld 	a,l				; set vram address in 14 bits
		out (#0x99),a
		ld 	a,h
		out (#0x99),a
		
		exx 
		outi				; write data
		exx

		inc h
		djnz 00001$


		dec c
		jr nz,00002$

;		ld a,#255				; color test
;		out	(#0x99), a
;		ld	a,#0x87
;		out	(#0x99), a		
		
		ei
		ret
__endasm;
	
}

*/

void myISR(void) __sdcccall(1) __naked
{
__asm 
		push af
		// ld	a,#0xE0			// green 
		// out	(#0x99),a
		// ld	a,#128+#7
		// out	(#0x99),a 
		
		xor  a,a           ; set Status Register #0 for reading
		out (#0x99),a
		ld  a,#0x8f
		out (#0x99),a

		in  a,(#0x99)		; mimimum ISR
		pop	af
		ei 
		ret
__endasm;
}

void myInstISR(void) __sdcccall(1) __naked
{
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

	