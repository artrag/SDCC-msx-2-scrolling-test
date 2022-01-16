



void FT_SetName( FCB *p_fcb, const char *p_name );
void FT_errorHandler(char n, char *name);
char myFT_LoadSc8Image(char *file_name, unsigned int start_Y, char *buffer);

void NewLine(unsigned char x,char page) __sdcccall(1) __naked;
void NewBlank(unsigned char x,char page) __sdcccall(1) __naked;

void  	myfVDP(void *Address)  __z88dk_fastcall  __naked;

void 	myVDPwrite( char vdpreg, char data ) __sdcccall(1) __naked;	// write to VDP Register

void	myFT_wait(unsigned char cicles) __sdcccall(1) __naked;

char	myPoint( unsigned int X,  unsigned int Y ) __sdcccall(1) __naked;

void mySetAdjust(signed char x, signed char y) __sdcccall(1);

void NewBorderLines(unsigned char x,char page) __sdcccall(1) __naked;

void myISR(void) __sdcccall(1) __naked;
void myInstISR(void) __sdcccall(1) __naked;
void myISRrestore(void) __sdcccall(1) __naked;

unsigned char myCheckkbd(unsigned char nrow) __sdcccall(1) __naked;

void myOutPort(unsigned char port,unsigned char data) __sdcccall(1) __naked;
unsigned char myInPort( unsigned char port ) __sdcccall(1) __naked;
void readLine( unsigned char *p) __sdcccall(1) __naked;

char MyLoadTiles(char *file_name) __sdcccall(1);
char MyLoadMap(char *file_name,unsigned char* p ) __sdcccall(1);


void NewBorderLinesR(unsigned char x,char page) __sdcccall(1) __naked;
void NewBorderLinesL(unsigned char x,char page) __sdcccall(1) __naked;


void NewLinesR(unsigned char x,char page) __sdcccall(1) __naked;
void NewLinesL(unsigned char x,char page) __sdcccall(1) __naked;

