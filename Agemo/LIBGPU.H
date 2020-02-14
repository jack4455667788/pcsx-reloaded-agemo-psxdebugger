/* $PSLibId: Run-time Library Release 4.6$ */
#ifndef _LIBGPU_H_
#define _LIBGPU_H_
/*
 *  (C) Copyright 1993-1995 Sony Corporation,Tokyo,Japan. All Rights Reserved
 *
 * 		libgpu.h: Graphic Primitive Structures Database
 *
 * Primitive list:
 *
 *      Name     |Size*1|Shade  |Vertex |Texture| Function
 *      ---------+------+-------+-------+-------+------------------------
 *      POLY_F3  | 5	|Flat   |   3   |OFF    | Flat Triangle
 *      POLY_FT3 | 8	|Flat   |   3   |ON     | Flat Textured Triangle
 *      POLY_G3  | 7	|Gouraud|   3   |OFF    | Gouraud Triangle
 *      POLY_GT3 |10	|Gouraud|   3   |ON     | Gouraud Textured Triangle
 *      POLY_F4  | 6	|Flat   |   4   |OFF    | Flat Quadrangle
 *      POLY_FT4 |10	|Flat   |   4   |ON     | Flat Textured Quadrangle
 *      POLY_G4  | 9	|Gouraud|   4   |OFF    | Gouraud Quadrangle
 *      POLY_GT4 |13	|Gouraud|   4   |ON     | Gouraud Textured Quadrangle
 *      ---------+------+-------+-------+-------+------------------------
 *      LINE_F2  | 4	|Flat   |   2   | -     | unconnected Flat Line 
 *      LINE_G2  | 5	|Gouraud|   2   | -     | unconnected Gouraud Line 
 *      LINE_F3  | 6	|Flat	|   3	| -     | 3-connected Flat Line
 *      LINE_G3  | 8	|Gouraud|   3	| -     | 3-connected Gouraud Line
 *      LINE_F4  | 7	|Flat	|   4	| -    	| 4-connected Flat Line
 *      LINE_G4  |10	|Gouraud|   4	| -    	| 4-connected Gouraud Line
 *      ---------+------+-------+-------+-------+------------------------
 *      SPRT	 | 5	|Flat	|   1   |ON     | free size Sprite
 *      SPRT_16	 | 4	|Flat	|   1   |ON     | 16x16 Sprite
 *      SPRT_8	 | 4	|Flat	|   1   |ON     | 8x8 Sprite
 *      ---------+------+-------+-------+-------+------------------------
 *      TILE	 | 4	|Flat	|   1   |OFF    | free size Sprite
 *      TILE_16	 | 3	|Flat	|   1   |OFF    | 16x16 Sprite
 *      TILE_8	 | 3	|Flat	|   1   |OFF    | 8x8 Sprite
 *      TILE_1	 | 3	|Flat	|   1   |OFF    | 1x1 Sprite
 *      ---------+------+-------+-------+-------+------------------------
 *      DR_TWIN	 | 3	|   -	|   -   | -     | Texture Window
 *      DR_AREA	 | 3	|   -	|   -   | -     | Drawing Area
 *      DR_OFFSET| 3	|   -	|   -   | -     | Drawing Offset
 *      DR_MODE  | 3	|   -	|   -   | -     | Drawing Mode
 *      DR_ENV   |16	|   -	|   -	| -     | Drawing Environment
 *      DR_MOVE  | 6	|   -	|   -	| -     | MoveImage
 *      DR_LOAD  |17	|   -	|   -	| -     | LoadImage
 *      DR_TPAGE | 2    |   -   |   -   | -     | Drawing TPage
 *      DR_STP   | 3    |   -   |   -   | -     | Drawing STP
 *
 *	*1: in long-word
 *
 * Texture Attributes:
 *	abr: ambient rate
 *		abr	0	1	2	3
 *		-------------------------------------
 *		Front	0.5	1.0	0.5	-1.0
 *		Back	0.5	1.0	1.0	 1.0
 *
 *	tp: texture mode
 *		 tp	0	1	2	
 *		 -----------------------------
 *		 depth	4bit	8bit	16bit
 *		 color	CLUT	CLUT	DIRECT
 */

//changed by agemo
/*
 *	Externals
 */
extern	int (*GPU_printf)(char *fmt, ...);	/* printf() object */
	
/*
 * Dump Primivie Parameters
 */
#define dumpRECT(r)	\
	GPU_printf("(%d,%d)-(%d,%d)\n", (r)->x,(r)->y,(r)->w,(r)->h)

#define dumpWH(p)	GPU_printf("(%d,%d)\n", (p)->w,  (p)->h ) 
#define dumpXY0(p)	GPU_printf("(%d,%d)\n", (p)->x0, (p)->y0) 
#define dumpUV0(p)	GPU_printf("(%d,%d)\n", (p)->u0, (p)->v0) 

#define dumpXY2(p)							\
	GPU_printf("(%d,%d)-(%d,%d)\n",					\
	(p)->x0, (p)->y0, (p)->x1, (p)->y1)

#define dumpXY3(p)							\
	GPU_printf("(%d,%d)-(%d,%d)-(%d,%d)\n",				\
	(p)->x0, (p)->y0, (p)->x1, (p)->y1,				\
	(p)->x2, (p)->y2)

#define dumpUV3(p)							\
	GPU_printf("(%d,%d)-(%d,%d)-(%d,%d)\n",				\
	(p)->u0, (p)->v0, (p)->u1, (p)->v1,				\
	(p)->u2, (p)->v2)

#define dumpXY4(p)							\
	GPU_printf("(%d,%d)-(%d,%d)-(%d,%d)-(%d,%d)\n",			\
	(p)->x0, (p)->y0, (p)->x1, (p)->y1,				\
	(p)->x2, (p)->y2, (p)->x3, (p)->y3)

#define dumpUV4(p)							\
	GPU_printf("(%d,%d)-(%d,%d)-(%d,%d)-(%d,%d)\n",			\
	(p)->u0, (p)->v0, (p)->u1, (p)->v1,				\
	(p)->u2, (p)->v2, (p)->u3, (p)->v3)			

#define dumpRGB0(p)							\
	GPU_printf("(%3d,%3d,%3d)\n", (p)->r0, (p)->g0, (p)->b0) 	
		   
#define dumpRGB1(p)							\
	GPU_printf("(%3d,%3d,%3d)\n", (p)->r1, (p)->g1, (p)->b1)	
		   
#define dumpRGB2(p)							\
	GPU_printf("(%3d,%3d,%3d)\n", (p)->r2, (p)->g2, (p)->b2) 
		   
#define dumpRGB3(p)							\
	GPU_printf("(%3d,%3d,%3d)\n", (p)->r3, (p)->g3, (p)->b3) 	

#define dumpTPage(tpage)						\
	GPU_printf("tpage: (%d,%d,%d,%d)\n",				\
			   ((tpage)>>7)&0x003,((tpage)>>5)&0x003,	\
			   ((tpage)<<6)&0x7c0,				\
			   (((tpage)<<4)&0x100)+(((tpage)>>2)&0x200))

#define dumpClut(clut) \
	GPU_printf("clut: (%d,%d)\n", (clut&0x3f)<<4, (clut>>6))

//CLUT-ID  Specifies the location of the CLUT data. Data is 16bits.
//F-6      Y coordinate 0-511
//5-0      X coordinate X/16
#define GET_CLUT_X(clut) (clut & 0x3f)<<4
#define GET_CLUT_Y(clut) (clut>>6)

//Texture Page information, 16 bit 
//|F E D C B A 9|8 7|6 5|4 |3 2 1 0|
//|0            |tp |abr|ty|tx     |
#define GET_TPAGE_X(tp) (tp & 0xf)<<6
#define GET_TPAGE_Y(tp) (tp & 0x10)<<4
#define GET_TPAGE_BIT(tp) (tp>>7) & 3
	
/*	Primitive 	Lentgh		Code				*/
/*--------------------------------------------------------------------	*/
/*									*/
#define SIZE_PolyF3	4//setlen(p, 4),  setcode(p, 0x20)
#define SIZE_PolyFT3	7//setlen(p, 7),  setcode(p, 0x24)
#define SIZE_PolyG3	6//)	setlen(p, 6),  setcode(p, 0x30)
#define SIZE_PolyGT3	9//(p)	setlen(p, 9),  setcode(p, 0x34)
#define SIZE_PolyF4	5//(p)	setlen(p, 5),  setcode(p, 0x28)
#define SIZE_PolyFT4	9//(p)	setlen(p, 9),  setcode(p, 0x2c)
#define SIZE_PolyG4	8//(p)	setlen(p, 8),  setcode(p, 0x38)
#define SIZE_PolyGT4	12//(p)	setlen(p, 12), setcode(p, 0x3c)

#define SIZE_Sprt8	3//(p)	setlen(p, 3),  setcode(p, 0x74)
#define SIZE_Sprt16	3//(p)	setlen(p, 3),  setcode(p, 0x7c)
#define SIZE_Sprt		4//(p)	setlen(p, 4),  setcode(p, 0x64)

#define SIZE_Tile1	2//(p)	setlen(p, 2),  setcode(p, 0x68)
#define SIZE_Tile8	2//(p)	setlen(p, 2),  setcode(p, 0x70)
#define SIZE_Tile16	2//(p)	setlen(p, 2),  setcode(p, 0x78)
#define SIZE_Tile		3//(p)	setlen(p, 3),  setcode(p, 0x60)
#define SIZE_LineF2	3//(p)	setlen(p, 3),  setcode(p, 0x40)
#define SIZE_LineG2	4//(p)	setlen(p, 4),  setcode(p, 0x50)
#define SIZE_LineF3	5//(p)	setlen(p, 5),  setcode(p, 0x48),(p)->pad = 0x55555555
#define SIZE_LineG3	7//(p)	setlen(p, 7),  setcode(p, 0x58),(p)->pad = 0x55555555, 		(p)->p2 = 0
#define SIZE_LineF4	6//(p)	setlen(p, 6),  setcode(p, 0x4c),(p)->pad = 0x55555555
#define SIZE_LineG4	9//(p)	setlen(p, 9),  setcode(p, 0x5c),(p)->pad = 0x55555555, 		(p)->p2 = 0, (p)->p3 = 0
	
/*
 * Rectangle:
 */
typedef struct {
	short x, y;		/* offset point on VRAM */
	short w, h;		/* width and height */
} PSX_RECT;

typedef struct {
	int x, y;		/* offset point on VRAM */
	int w, h;		/* width and height */
} PSX_RECT32;

/*
 * Environment 
 */
typedef struct {
	u_long	tag;
	u_long	code[15];
} DR_ENV;				/* Packed Drawing Environment */
	       
typedef struct {
	PSX_RECT	clip;		/* clip area */
	short	ofs[2];		/* drawing offset */
	PSX_RECT	tw;		/* texture window */
	u_short tpage;		/* texture page */	
	u_char	dtd;		/* dither flag (0:off, 1:on) */
	u_char	dfe;		/* flag to draw on display area (0:off 1:on) */
	u_char	isbg;		/* enable to auto-clear */
	u_char	r0, g0, b0;	/* initital background color */
	DR_ENV	dr_env;		/* reserved */
} DRAWENV;
	       
typedef struct {
	PSX_RECT	disp;		/* display area */
	PSX_RECT	screen;		/* display start point */
	u_char	isinter;	/* interlace 0: off 1: on */
	u_char	isrgb24;	/* RGB24 bit mode */
	u_char	pad0, pad1;	/* reserved */
} DISPENV;

/*
 * Polygon Primitive Definitions
 */
typedef struct {
	unsigned	addr: 24;
	unsigned 	len:   8;
	u_char		r0, g0, b0, code;
} P_TAG;
	
typedef struct {
	u_char	r0, g0, b0, code;
} P_CODE;
	
typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, y0;
	short	x1,	y1;
	short	x2,	y2;
} POLY_F3;				/* Flat Triangle */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, y0;
	short	x1,	y1;
	short	x2,	y2;
	short	x3,	y3;
} POLY_F4;				/* Flat Quadrangle */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, y0;
	u_char	u0, v0;	u_short	clut;
	short	x1,	y1;
	u_char	u1, v1;	u_short	tpage;
	short	x2,	y2;
	u_char	u2, v2;	u_short	pad1;
} POLY_FT3;				/* Flat Textured Triangle */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	u0, v0;	u_short	clut;
	short	x1,	y1;
	u_char	u1, v1;	u_short	tpage;
	short	x2,	y2;
	u_char	u2, v2;	u_short	pad1;
	short	x3,	y3;
	u_char	u3, v3;	u_short	pad2;
} POLY_FT4;				/* Flat Textured Quadrangle */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	r1, g1, b1, pad1;
	short	x1,	y1;
	u_char	r2, g2, b2, pad2;
	short	x2,	y2;
} POLY_G3;				/* Gouraud Triangle */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	r1, g1, b1, pad1;
	short	x1,	y1;
	u_char	r2, g2, b2, pad2;
	short	x2,	y2;
	u_char	r3, g3, b3, pad3;
	short	x3,	y3;
} POLY_G4;				/* Gouraud Quadrangle */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	u0, v0;	u_short	clut;
	u_char	r1, g1, b1, p1;
	short	x1,	y1;
	u_char	u1, v1;	u_short	tpage;
	u_char	r2, g2, b2, p2;
	short	x2,	y2;
	u_char	u2, v2;	u_short	pad2;
} POLY_GT3;				/* Gouraud Textured Triangle */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	u0, v0;	u_short	clut;
	u_char	r1, g1, b1, p1;
	short	x1,	y1;
	u_char	u1, v1;	u_short	tpage;
	u_char	r2, g2, b2, p2;
	short	x2,	y2;
	u_char	u2, v2;	u_short	pad2;
	u_char	r3, g3, b3, p3;
	short	x3,	y3;
	u_char	u3, v3;	u_short	pad3;
} POLY_GT4;				/* Gouraud Textured Quadrangle */

/*
 * Line Primitive Definitions
 */
typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	short	x1,	y1;
} LINE_F2;				/* Unconnected Flat Line */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	r1, g1, b1, p1;
	short	x1,	y1;
} LINE_G2;				/* Unconnected Gouraud Line */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	short	x1,	y1;
	short	x2,	y2;
	u_long	pad;
} LINE_F3;				/* 2 connected Flat Line */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	r1, g1, b1, p1;
	short	x1,	y1;
	u_char	r2, g2, b2, p2;
	short	x2,	y2;
	u_long	pad;
} LINE_G3;				/* 2 connected Gouraud Line */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	short	x1,	y1;
	short	x2,	y2;
	short	x3,	y3;
	u_long	pad;
} LINE_F4;				/* 3 connected Flat Line Quadrangle */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	r1, g1, b1, p1;
	short	x1,	y1;
	u_char	r2, g2, b2, p2;
	short	x2,	y2;
	u_char	r3, g3, b3, p3;
	short	x3,	y3;
	u_long	pad;
} LINE_G4;				/* 3 connected Gouraud Line */

/*
 * Sprite Primitive Definitions
 */
typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	u0, v0;	u_short	clut;
	short	w,	h;
} SPRT;					/* free size Sprite */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	u0, v0;	u_short	clut;
} SPRT_16;				/* 16x16 Sprite */
	       
typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	u_char	u0, v0;	u_short	clut;
} SPRT_8;				/* 8x8 Sprite */
	       
/*
 * Tile Primitive Definitions
 */
typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
	short	w,	h;
} TILE;					/* free size Tile */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
} TILE_16;				/* 16x16 Tile */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
} TILE_8;				/* 8x8 Tile */

typedef struct {
	u_long	tag;
	u_char	r0, g0, b0, code;
	short	x0, 	y0;
} TILE_1;				/* 1x1 Tile */

/*
 *  Special Primitive Definitions
 */
typedef struct {
	u_long	tag;
	u_long	code[2];
} DR_MODE;				/* Drawing Mode */

typedef struct {
	u_long	tag;
	u_long	code[2];
} DR_TWIN;				/* Texture Window */
	       
typedef struct {
	u_long	tag;
	u_long	code[2];
} DR_AREA;				/* Drawing Area */
	       
typedef struct {
	u_long	tag;
	u_long	code[2];
} DR_OFFSET;				/* Drawing Offset */
	       
typedef struct {			/* MoveImage */
	u_long	tag;
	u_long	code[5];
} DR_MOVE;

typedef struct {			/* LoadImage */
	u_long	tag;
	u_long	code[3];
	u_long	p[13];
} DR_LOAD;

typedef	struct {
	u_long	tag;
	u_long	code[1];
} DR_TPAGE;				/* Drawing TPage */

typedef struct {
        u_long  tag;
        u_long  code[2];
} DR_STP;                               /* Drawing STP */

/*
 *	Font Stream Parameters
 */
#define FNT_MAX_ID	8	/* max number of stream ID */
#define FNT_MAX_SPRT	1024	/* max number of sprites in all streams */

/*
 *	Multi-purpose Sony-TMD primitive
 */
typedef struct {		/* long word type 3D vector */
	long	vx, vy;
	long	vz, pad;
} VECTOR;
	
typedef struct {		/* short word type 3D vector */	
	short	vx, vy;
	short	vz, pad;
} SVECTOR;
	       
typedef struct {		/* color type vector */	
	u_char	r, g, b, cd;
} CVECTOR;
	       
typedef struct {		/* 2D short vector */
	short vx, vy;
} DVECTOR;


typedef struct {
	u_long	id;	
	u_char	r0, g0, b0, p0;		/* Color of vertex 0 */
	u_char	r1, g1, b1, p1;		/* Color of vertex 1 */
	u_char	r2, g2, b2, p2;		/* Color of vertex 2 */
	u_char	r3, g3, b3, p3;		/* Color of vertex 3 */
	u_short	tpage, clut;		/* texture page ID, clut ID */
	u_char	u0, v0, u1, v1;		/* texture corner point */
	u_char	u2, v2, u3, v3;
	
	/* independent vertex model */
	SVECTOR	x0, x1, x2, x3;		/* 3D corner point */
	SVECTOR	n0, n1, n2, n3;		/* 3D corner normal vector */
	
	/* Common vertex model */
	SVECTOR	*v_ofs;			/* offset to vertex database */
	SVECTOR	*n_ofs;			/* offset to normal database */
	
	u_short	vert0, vert1; 		/* index of vertex */
	u_short	vert2, vert3;		
	u_short	norm0, norm1; 		/* index of normal */
	u_short	norm2, norm3;

	
} TMD_PRIM;

/*
 *	Multi-purpose TIM image
 */
typedef struct {
	u_long  mode;		/* pixel mode */
	PSX_RECT	*crect;		/* CLUT rectangle on frame buffer */
	u_long	*caddr;		/* CLUT address on main memory */
	PSX_RECT	*prect;		/* texture image rectangle on frame buffer */
	u_long	*paddr;		/* texture image address on main memory */
} TIM_IMAGE;
	       
#endif /* _LIBGPU_H_ */
