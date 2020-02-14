#include <windows.h>
#include <stdio.h>
#include "AgemoDebug.h"
#include "../libpcsxcore/psxmem.h"

#include "libGPU.h"

unsigned long lUsedAddr[3];
extern int agemo_flag_gpu_upload;

__inline BOOL CheckForEndlessLoop(unsigned long laddr)
{
	if(laddr == lUsedAddr[1]) return TRUE;
	if(laddr == lUsedAddr[2]) return TRUE;

	if(laddr < lUsedAddr[0])
		lUsedAddr[1] = laddr;
	else
		lUsedAddr[2] = laddr;
	
	lUsedAddr[0] = laddr;
	return FALSE;
}

void Agemo_OnGpuPackets(unsigned long *, unsigned long, int);
FILE *gpuLog = NULL;

void Agemo_OnGpuDmaChain(unsigned long *baseAddrL, unsigned long addr)
{
	//解析 GPU DMA Chain

	 unsigned long dmaMem;
	 unsigned char * baseAddrB;
	 short count;
	 unsigned int DMACommandCounter = 0;

	 lUsedAddr[0]=lUsedAddr[1]=lUsedAddr[2]=0xffffff;

	 baseAddrB = (unsigned char*) baseAddrL;

	 gpuLog = fopen("gpu.log","ac");
	 fprintf(gpuLog, "====DMA==== @ %X\n", addr);

	 do
	 {
		addr&=0x1FFFFC;

		if(DMACommandCounter++ > 2000000) break;
		if(CheckForEndlessLoop(addr)) break;

		count = baseAddrB[addr+3];

		dmaMem=addr+4;

		if(count>0)	Agemo_OnGpuPackets(baseAddrL, dmaMem, count);

		addr = baseAddrL[addr>>2]&0xffffff;
	 
	 }while (addr != 0xffffff);

	 fflush(gpuLog);
	 fclose(gpuLog);
}

//每个prim()解析的入口参数
unsigned char *g_PrimAddrB = NULL;
unsigned long *g_PrimAddrL = NULL;
unsigned long g_PrimWord = 0;				//当前和prim命令公用的字

//每个prim()解析的出口参数
char g_PrimDesc[2048];

void DumpPoly4(char *pDmp, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3)
{
	//检测是否是矩形。
	//(x0 , y0)-(x1 , y1)-(x2 , y2)-(x3 , y3) 
	//(152, 46)-(160, 46)-(152, 54)-(160, 54)

	static int rectW, rectH;
	if( x0 == x2 && y0 == y1 && x1 == x3 && y2 == y3)
	{
		rectW = x1 - x0;
		rectH = y2 - y0;

		sprintf(pDmp, "(%d, %d)*(%d, %d)",
			x0, y0, rectW, rectH); 
	}
	else
	{
		sprintf(pDmp, "(%d, %d)-(%d, %d)-(%d, %d)-(%d, %d)",
			x0, y0, x1, y1, x2, y2, x3, y3); 
	}

}

void DumpUV(char *pDmp, int tpage, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3)
{
	static int rectW, rectH;
	int tpagex, tpagey, tpagebit;
	int nMultiple = 1;

	tpagex = GET_TPAGE_X(tpage);
	tpagey = GET_TPAGE_Y(tpage);
	tpagebit = GET_TPAGE_BIT(tpage);
	if(tpagebit==0)nMultiple = 4;
	if(tpagebit==1)nMultiple = 2;
	if(tpagebit==2)nMultiple = 1;

	if( x0 == x2 && y0 == y1 && x1 == x3 && y2 == y3)
	{
		rectW = x1 - x0;
		rectH = y2 - y0;

		sprintf(pDmp, "(%d, %d)*(%d, %d)",
			tpagex+ (x0/nMultiple), tpagey + y0, rectW, rectH); 
	}
	else
	{
		sprintf(pDmp, "(%d, %d)-(%d, %d)-(%d, %d)-(%d, %d)",
			x0, y0, x1, y1, x2, y2, x3, y3); 
	}

}


int prim__()
{
	sprintf(g_PrimDesc, "UNK");
	return 1;
}
int prim00()
{
	sprintf(g_PrimDesc, "N/A");
	return 1;
}
int prim01()
{
	sprintf(g_PrimDesc, "clear cache");
	return 1;
}
int prim02()
{
	//$02     frame buffer rectangle draw
	// |1f-18|17-10|0f-08|07-00|
	//1|$02  |BGR              |command+color
	//2|Y          |X          |Topleft corner
	//3|H          |W          |Width & Height

	int r,g,b, x, y, w, h;

	r = g_PrimWord & 0xFF;
	g = (g_PrimWord >>  8) & 0xFF;
	b = (g_PrimWord >> 16) & 0xFF;

	x = g_PrimAddrL[1] & 0xFFFF;
	y = g_PrimAddrL[1] >> 16;
	w = g_PrimAddrL[2] & 0xFFFF;
	h = g_PrimAddrL[2] >> 16;

	sprintf(g_PrimDesc, "clear rect  (%d, %d)*(%d, %d) RGB(%2X, %2X, %2X) ", x,y,w,h,r,g,b);
	return 3;
}
int primE1()
{
	//$e1     draw mode setting
	// |1f-18|17-0b|0a |09 |08 07|06 05|04|03 02 01 00|
	//1|$e1  |     |dfe|dtd|tp   |abr  |ty|tx         | command +values

	sprintf(g_PrimDesc, "draw mode %06X "
		"TP(%d, %d)(bit:%d)", 
		g_PrimWord,
		GET_TPAGE_X(g_PrimWord),GET_TPAGE_Y(g_PrimWord),GET_TPAGE_BIT(g_PrimWord)	
		);
	return 1;
}
int primE2()
{
	//$e2     texture window setting
	// |1F-18|17-14|13-0F|0E-0A|09-05|04-00|
	//1|$E2        |twy  |twx  |twh  |tww  | command + value

	int twx,twy,tww,twh;
	tww = g_PrimWord & 0x1F;
	twh = (g_PrimWord >> 0x5)& 0x1F;
	twx = (g_PrimWord >> 0xa)& 0x1F;
	twy = (g_PrimWord >> 0xf)& 0x1F;
	sprintf(g_PrimDesc, "texture window %06X, (%d, %d)*(%d, %d)", g_PrimWord, twx, twy, tww, twh);
	return 1;
}
int primE3()
{
	//$e3     set drawing area top left
	// |1f-18|17-14|13-0a|09-00|
	//1|$e3  |     |Y    |X    |
	int x,y;
	x = g_PrimWord & 0x1FF;
	y = (g_PrimWord >> 0xa)& 0x1FF;
	sprintf(g_PrimDesc, "drawing area top left %06X, (%d, %d) ", g_PrimWord, x, y);
	return 1;
}
int primE4()
{
	//$e4     set drawing area bottom right
	// |1f-18|17-14|13-0a|09-00|
	//1|$e4  |     |Y    |X    |
	int x,y;
	x = g_PrimWord & 0x1FF;
	y = (g_PrimWord >> 0xa)& 0x1FF;
	sprintf(g_PrimDesc, "drawing area bottom right %06X, (%d, %d) ", g_PrimWord, x, y);
	return 1;
}
int primE5()
{
	//$e5     drawing offset
	// |1f-18|17-14|14-0b|0a-00|
	//1|$e5  |     |OffsY|OffsX|

	int x,y;
	x = g_PrimWord & 0x3FF;
	y = (g_PrimWord >> 0xb)& 0x3FF;
	sprintf(g_PrimDesc, "drawing offset %06X, (%d, %d) ", g_PrimWord, x, y);
	return 1;
}
int primE6()
{
	//$e6     mask setting
	// |1f-18|17-02|01   |00   |
	//1|$e6  |     |Mask2|Mask1|

	int m1,m2;
	m1 = g_PrimWord & 1;
	m2 = (g_PrimWord >> 1) & 1;
	sprintf(g_PrimDesc, "mask setting %06X, (%x, %x) ", g_PrimWord, m1, m2);
	return 1;
}
int prim80()
{
	//$80     move image in frame buffer
	// |1f-18|17-10|0f-08|07-00|
	//1|$80  |                0|command
	//2|sY         |sX         |Source coord.
	//3|dY         |dX         |Destination coord.
	//4|H          |W          |Height+Width of transfer

	int sY, sX, dX, dY, H, W;

	sX = g_PrimAddrL[1] & 0xFFFF;
	sY = g_PrimAddrL[1] >> 16;
	dX = g_PrimAddrL[2] & 0xFFFF;
	dY = g_PrimAddrL[2] >> 16;
	H = g_PrimAddrL[3] & 0xFFFF;
	W = g_PrimAddrL[3] >> 16;
	sprintf(g_PrimDesc, "move image (%d, %d)*(%d, %d) -> (%d, %d)", sX, sY, W, H, dX, dY);
	return 4;
}

int primA0()
{
	//$a0	send image to frame buffer
	// |1f-18|17-10|0f-08|07-00|
	//1|$A0  |                 |
	//2|Y          |X          |Destination coord.
	//3|H          |W          |Height+Width of transfer
	//4|pix1       |pix0       |image data
	//5..
	//?|pixn       |pixn-1     |

	//Transfers data from mainmemory to frame buffer
	//If the number of pixels to be sent is odd, an extra should be
	//sent. (32 bits per packet)

	int sY, sX, H, W, size;

	sX = g_PrimAddrL[1] & 0xFFFF;
	sY = g_PrimAddrL[1] >> 16;
	W = g_PrimAddrL[2] & 0xFFFF;
	H = g_PrimAddrL[2] >> 16;

	size = W*H*2;
	sprintf(g_PrimDesc, "load image (%d, %d)*(%d, %d) <- %x", sX, sY, W, H, g_PrimAddrB-psxM+12);

	if(agemo_flag_gpu_upload)
	{
		AgemoTrace("GPU Upload from $%X , $%X bytes", g_PrimAddrB-psxM+12, size*4);
		AgemoTrace("area X/Y(%d, %d) W/H(%d,%d)", sX, sY, W, H);
		__agemo_pause_cpu(1);
	}

	return 4;
}

int prim64()
{
	//$64     sprite
	// |1f-18|17-10|0f-08|07-00|
	//1|$64  |BGR              |command+color
	//2|y          |x          |
	//3|clut       |v    |u    |clut location, texture page y,x
	//4|h          |w          |

	//Because the SPRT primitive has no tpage parameter, 
	//the texture page of the current drawing environment
	//is used. You can change the texture page by inserting
	//a DR_TPAGE or DR_MODE primitive into the
	//primitive list before your SPRT primitive.

	SPRT *p;
	p = (SPRT *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "sprite (%d, %d)*(%d, %d) "
		"clut(%d, %d), UV(%d, %d) RGB(%2X, %2X, %2X)",
		p->x0, p->y0, p->w, p->h, 
		GET_CLUT_X(p->clut), GET_CLUT_Y(p->clut), 
		p->u0, p->v0, 
		p->r0, p->g0, p->b0);
	return 4;
}
int prim74()
{
	//$74     8*8 sprite

	// |1f-18|17-10|0f-08|07-00|
	//1|$74  |BGR              |command+color
	//2|y          |x          |
	//3|clut       |v    |u    |clut location, texture page y,x

	SPRT *p;
	p = (SPRT *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "sprite 8*8 (%d, %d) "
		"clut(%d, %d), UV(%d, %d) RGB(%2X, %2X, %2X)",
		p->x0, p->y0,
		GET_CLUT_X(p->clut), GET_CLUT_Y(p->clut), 
		p->u0, p->v0, 
		p->r0, p->g0, p->b0);
	return 3;
}

int prim7C()
{
	//$7C     16*16 sprite

	// |1f-18|17-10|0f-08|07-00|
	//1|$7c  |BGR              |command+color
	//2|y          |x          |
	//3|clut       |v    |u    |clut location, texture page y,x

	SPRT *p;
	p = (SPRT *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "sprite 16*16 (%d, %d) "
		"clut(%d, %d), UV(%d, %d) RGB(%2X, %2X, %2X)",
		p->x0, p->y0,
		GET_CLUT_X(p->clut), GET_CLUT_Y(p->clut), 
		p->u0, p->v0, 
		p->r0, p->g0, p->b0);
	return 3;
}


//polygon
int prim20()
{
 	//$20     Flat Triangle
	// |1f-18|17-10|0f-08|07-00|
	//1|$20  |BGR              |command+color
	//2|y0         |x0         |vertexes
	//3|y1         |x1         |
	//4|y2         |x2         |

	POLY_F3 *p;
	p = (POLY_F3 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "F3 (%d, %d)-(%d, %d)-(%d, %d) RGB(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, 
		p->r0, p->g0, p->b0);
	
	return 4;
}
int prim24()
{
	//$24     Flat Textured Triangle
	// |1f-18|17-10|0f-08|07-00|
	//1|$24  |BGR              |command+color
	//2|y0         |x0         |vertex 0
	//3|clut       |v0   |u0   |clutid+ texture coords vertext 0
	//4|y1         |x1         |
	//5|tpage      |v1   |u1   |
	//6|y2         |x2         |
	//7|           |v2   |u2   |

	POLY_FT3 *p;
	p = (POLY_FT3 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "FT3 (%d, %d)-(%d, %d)-(%d, %d) "
		"clut(%d, %d) TP(%d, %d)(bit:%d) (%d, %d)-(%d, %d)-(%d, %d) "
		"RGB(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, 
		GET_CLUT_X(p->clut), GET_CLUT_Y(p->clut), 
		GET_TPAGE_X(p->tpage),GET_TPAGE_Y(p->tpage),GET_TPAGE_BIT(p->tpage),
		p->u0, p->v0, p->u1, p->v1, p->u2, p->v2,
		p->r0, p->g0, p->b0);
	
	return 7;
}

int prim28()
{
	//$28     monchrome 4 point polygon
	// |1f-18|17-10|0f-08|07-00|
	//1|$28  |BGR              |command+color
	//2|y0         |x0         |vertexes
	//3|y1         |x1         |
	//4|y2         |x2         |
	//5|y3         |x3         |

	POLY_F4 *p;
	p = (POLY_F4 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "F4 (%d, %d)-(%d, %d)-(%d, %d)-(%d, %d) RGB(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, p->x3, p->y3, 
		p->r0, p->g0, p->b0);
	return 5;
}

int prim2C()
{
	//$2c     Flat Textured Quadrangle
	// |1f-18|17-10|0f-08|07-00|
	//1|$2c  |BGR              |command+color
	//2|y0         |x0         |vertex 0
	//3|clut       |v0   |u0   |clutid+ texture coords vertext 0
	//4|y1         |x1         |
	//5|tpage      |v1   |u1   |
	//6|y2         |x2         |
	//7|           |v2   |u2   |
	//8|y3         |x3         |
	//9|           |v3   |u3   |

	static char pDmp1[1024];
	static char pDmp2[1024];
	static char pDmp3[1024];
	POLY_FT4 *p;
	p = (POLY_FT4 *)(g_PrimAddrB - 4);


	DumpPoly4(pDmp1, p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, p->x3, p->y3); 
	DumpPoly4(pDmp2, p->u0, p->v0, p->u1, p->v1, p->u2, p->v2, p->u3, p->v3); 
	DumpUV   (pDmp3, p->tpage, p->u0, p->v0, p->u1, p->v1, p->u2, p->v2, p->u3, p->v3); 

	sprintf(g_PrimDesc, "FT4 %s "
		"clut(%d, %d) TP(%d, %d)(bit:%d) "
		"UV<%s>%s "
		"RGB(%2X, %2X, %2X)",
		pDmp1,
		GET_CLUT_X(p->clut), GET_CLUT_Y(p->clut),
		GET_TPAGE_X(p->tpage),GET_TPAGE_Y(p->tpage),GET_TPAGE_BIT(p->tpage),
		pDmp3,
		pDmp2,
		p->r0, p->g0, p->b0);

	return 9;
}

int prim30()
{
	//$30    Gouraud Triangle
	// |1f-18|17-10|0f-08|07-00|
	//1|$30  |BGR0             |command+color
	//2|y0         |x0         |vertexes
	//3|     |BGR1             |
	//4|y1         |x1         |
	//5|     |BGR2             |
	//6|y2         |x2         |

	POLY_G3 *p;
	p = (POLY_G3 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "G3 (%d, %d)-(%d, %d)-(%d, %d) RGB(%2X, %2X, %2X)(%2X, %2X, %2X)(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, 
		p->r0, p->g0, p->b0, p->r1, p->g1, p->b1, p->r2, p->g2, p->b2);
	
	return 6;
}
int prim34()
{
	//$34     Gouraud Textured Triangle
	// |1f-18|17-10|0f-08|07-00|
	//1|$34  |BGR0             |command+color
	//2|y0         |x0         |vertex 0
	//3|clut       |v0   |u0   |clutid+ texture coords vertex 0
	//4|     |BGR1             |
	//5|y1         |x1         |
	//6|tpage      |v1   |u1   |
	//7|     |BGR2             |
	//8|y2         |x2         |
	//9|           |v2   |u2   |

	POLY_GT3 *p;
	p = (POLY_GT3 *)(g_PrimAddrB - 4);
	
	sprintf(g_PrimDesc, "GT3 (%d, %d)-(%d, %d)-(%d, %d) "
		"clut(%d, %d) TP(%d, %d)(bit:%d) (%d,%d)-(%d,%d)-(%d,%d) RGB(%2X, %2X, %2X)(%2X, %2X, %2X)(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, 
		GET_CLUT_X(p->clut), GET_CLUT_Y(p->clut),
		GET_TPAGE_X(p->tpage),GET_TPAGE_Y(p->tpage),GET_TPAGE_BIT(p->tpage),
		p->u0, p->v0, p->u1, p->v1, p->u2, p->v2,
		p->r0, p->g0, p->b0, p->r1, p->g1, p->b1, p->r2, p->g2, p->b2);


	return 9;
}
int prim38()
{
	//$38     Gouraud Quadrangle
	// |1f-18|17-10|0f-08|07-00|
	//1|$38  |BGR0             |command+color
	//2|y0         |x0         |vertexes
	//3|     |BGR1             |
	//4|y1         |x1         |
	//5|     |BGR2             |
	//6|y2         |x2         |
	//7|     |BGR3             |
	//8|y3         |x3         |

	POLY_G4 *p;
	static char pDmp1[1024];

	p = (POLY_G4 *)(g_PrimAddrB - 4);

	DumpPoly4(pDmp1, p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, p->x3, p->y3); 

	sprintf(g_PrimDesc, "G4 %s RGB(%2X, %2X, %2X)(%2X, %2X, %2X)(%2X, %2X, %2X)(%2X, %2X, %2X)",
		pDmp1,
		p->r0, p->g0, p->b0, p->r1, p->g1, p->b1, p->r2, p->g2, p->b2, p->r3, p->g3, p->b3);
	
	return 8;
}
int prim3C()
{
	//$3c     Gouraud Textured Quadrangle
	// |1f-18|17-10|0f-08|07-00|
	//1|$3c  |BGR0             |command+color
	//2|y0         |x0         |vertex 0
	//3|clut       |v0   |u0   |clutid+ texture coords vertex 0
	//4|     |BGR1             |
	//5|y1         |x1         |
	//6|tpage      |v1   |u1   |texture page location
	//7|     |BGR2             |
	//8|y2         |x2         |
	//9|           |v2   |u2   |
	//a|     |BGR3             |
	//b|y3         |x3         |
	//c|           |v3   |u3   |

	POLY_GT4 *p;
	static char pDmp1[1024];
	static char pDmp2[1024];

	p = (POLY_GT4 *)(g_PrimAddrB - 4);

	DumpPoly4(pDmp1, p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, p->x3, p->y3); 
	DumpPoly4(pDmp2, p->u0, p->v0, p->u1, p->v1, p->u2, p->v2, p->u3, p->v3); 
	
	sprintf(g_PrimDesc, "GT4 %s "
		"clut(%d, %d) TP(%d, %d)(bit:%d) "
		"%s RGB(%2X, %2X, %2X)(%2X, %2X, %2X)(%2X, %2X, %2X)(%2X, %2X, %2X)",
		pDmp1,
		GET_CLUT_X(p->clut), GET_CLUT_Y(p->clut),
		GET_TPAGE_X(p->tpage),GET_TPAGE_Y(p->tpage),GET_TPAGE_BIT(p->tpage),
		pDmp2,
		p->r0, p->g0, p->b0, p->r1, p->g1, p->b1, p->r2, p->g2, p->b2, p->r3, p->g3, p->b3);

	return 0xC;
}

int prim40()
{
 	//$40     unconnected Flat Line
	// |1f-18|17-10|0f-08|07-00|
	//1|$40  |BGR              |command+color
	//2|y0         |x0         |vertex 0
	//3|y1         |x1         |vertex 1

	LINE_F2 *p;
	p = (LINE_F2 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "F2 (%d, %d)-(%d, %d) RGB(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, 
		p->r0, p->g0, p->b0);
	
	return 3;
}
int prim48()
{
	//48		3-connected Flat Line
	// |1f-18|17-10|0f-08|07-00|
	//1|$48  |BGR              |command+color
	//2|y0         |x0         |vertex 0
	//3|y1         |x1         |vertex 1
	//4|y2         |x2         |vertex 2
	//
	//.|yn         |xn         |vertex n
	//.|$55555555 Temination code.             !!!!!!!!!!!!!!!!!!!!

	LINE_F3 *p;
	p = (LINE_F3 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "F3 (%d, %d)-(%d, %d)-(%d, %d) RGB(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, p->x2, p->y2,
		p->r0, p->g0, p->b0);
	return 4+1;
}

int prim4C()
{
	//4C		4-connected Flat Line
	// |1f-18|17-10|0f-08|07-00|
	//1|$48  |BGR              |command+color
	//2|y0         |x0         |vertex 0
	//3|y1         |x1         |vertex 1
	//4|y2         |x2         |vertex 2
	//5|y3         |x3         |vertex 3

	//.|yn         |xn         |vertex n
	//.|$55555555 Temination code.
	LINE_F4 *p;
	p = (LINE_F4 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "F4 (%d, %d)-(%d, %d)-(%d, %d)-(%d, %d) RGB(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, p->x3, p->y3,
		p->r0, p->g0, p->b0);
	return 5;
}

int prim50()
{
 	//$50     unconnected Gouraud Line
	// |1f-18|17-10|0f-08|07-00|
	//1|$50  |BGR0             |command+color
	//2|y0         |x0         |
	//3|     |BGR1             |
	//4|y1         |x1         |

	LINE_G2 *p;
	p = (LINE_G2 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "G2 (%d, %d)-(%d, %d) RGB(%2X, %2X, %2X)(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, 
		p->r0, p->g0, p->b0, p->r1, p->g1, p->b1);
	
	return 4;
}

int prim58()
{
 	//$58		3-connected Gouraud Line
	// |1f-18|17-10|0f-08|07-00|
	//1|$50  |BGR0             |command+color
	//2|y0         |x0         |
	//3|     |BGR1             |
	//4|y1         |x1         |
	//5|     |BGR2             |
	//6|y2         |x2         |

	LINE_G3 *p;
	p = (LINE_G3 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "G3 (%d, %d)-(%d, %d)-(%d, %d) RGB(%2X, %2X, %2X)(%2X, %2X, %2X)(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, 
		p->r0, p->g0, p->b0, p->r1, p->g1, p->b1, p->r2, p->g2, p->b2);
	
	return 6;
}

int prim5C()
{
 	//$5C		4-connected Gouraud Line
	// |1f-18|17-10|0f-08|07-00|
	//1|$50  |BGR0             |command+color
	//2|y0         |x0         |
	//3|     |BGR1             |
	//4|y1         |x1         |
	//5|     |BGR2             |
	//6|y2         |x2         |
	//7|     |BGR3             |
	//8|y3         |x3         |

	LINE_G4 *p;
	p = (LINE_G4 *)(g_PrimAddrB - 4);

	sprintf(g_PrimDesc, "G4 (%d, %d)-(%d, %d)-(%d, %d)-(%d, %d) RGB(%2X, %2X, %2X)(%2X, %2X, %2X)(%2X, %2X, %2X)(%2X, %2X, %2X)",
		p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, p->x3, p->y3,
		p->r0, p->g0, p->b0, p->r1, p->g1, p->b1, p->r2, p->g2, p->b2, p->r3, p->g3, p->b3);
	
	return 8;
}

int prim60()
{
	//$60     rectangle
	// |1f-18|17-10|0f-08|07-00|
	//1|$60  |BGR              |command+color
	//2|y          |x          |
	//3|h          |w          |
	
	TILE *p;
	p = (TILE *)(g_PrimAddrB - 4);
	sprintf(g_PrimDesc, "RECT (%d, %d)*(%d, %d) RGB(%2X, %2X, %2X)",
		p->x0, p->y0, p->w, p->h, 
		p->r0, p->g0, p->b0);
	
	return 3;
}

int (*agemoPrim[0x100])() = 
{
	//  00      01      02      03      04       05     06      07
	prim00, prim01, prim02, prim__, prim__, prim__, prim__, prim__, //00-07
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //08-0F
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //10-17
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //18-1F
	prim20, prim20, prim20, prim20, prim24, prim24, prim24, prim24, //20-27
	prim28, prim28, prim28, prim28, prim2C, prim2C, prim2C, prim2C, //28-2F
	prim30, prim30, prim30, prim30, prim34, prim34, prim34, prim34, //30-37
	prim38, prim38, prim38, prim38, prim3C, prim3C, prim3C, prim3C, //38-3F
	prim40, prim40, prim40, prim40, prim__, prim__, prim__, prim__, //40-47
	prim48, prim48, prim48, prim48, prim4C, prim4C, prim4C, prim4C, //48-4F
	prim50, prim50, prim50, prim50, prim__, prim__, prim__, prim__, //50-57
	prim58, prim58, prim58, prim58, prim5C, prim5C, prim5C, prim5C, //58-5F
	prim60, prim60, prim60, prim60, prim64, prim64, prim64, prim64, //60-67
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //68-6F
	prim__, prim__, prim__, prim__, prim74, prim74, prim74, prim74, //70-77
	prim__, prim__, prim__, prim__, prim7C, prim7C, prim7C, prim7C, //78-7F
	prim80, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //80-87
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //88-8F
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //90-97
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //98-9F
	primA0, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //A0-A7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //A8-AF
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //B0-B7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //B8-BF
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //C0-C7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //C8-CF
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //D0-D7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //D8-DF
	prim__, primE1, primE2, primE3, primE4, primE5, primE6, prim__, //E0-E7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //E8-EF
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__, //F0-F7
	prim__, prim__, prim__, prim__, prim__, prim__, prim__, prim__  //F8-FF
};

void Agemo_OnGpuPackets(unsigned long *baseAddrL, unsigned long addr, int count)
{
	unsigned char * baseAddrB;
	unsigned char prim;
	int n;
	int nSpan;

	baseAddrB = (unsigned char*) baseAddrL;

	n = 0;

	do
	 {
		 //get primitive type
		 prim = baseAddrB[addr + n*4 + 3];
		 g_PrimAddrB = baseAddrB + addr + n*4;
		 g_PrimAddrL = baseAddrL + (addr>>2) + n;
		 g_PrimWord = *g_PrimAddrL;
		 g_PrimWord = (*g_PrimAddrL) & 0xFFFFFF;
		 nSpan = agemoPrim[prim]();
		 n += nSpan;

		 if(g_PrimDesc[0] == 0)
		 {
			 fprintf(gpuLog,"%06X:%02X - unk.", g_PrimAddrB-baseAddrB, prim);
			 UI_OnBtnDump();
			 exit(0);
			 return;
		 }
		 fprintf(gpuLog, "%06X:%02X - %s\n", g_PrimAddrB-baseAddrB, prim, g_PrimDesc);

	 }
	 while(n < count);
	 
}
