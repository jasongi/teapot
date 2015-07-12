/*$T indentinput.c GC 1.140 11/04/13 02:20:13 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
/*  
    Copyright (C) 2013 Jason Giancono (jasongiancono@gmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "cg200.h"

/*
 * WINDOW_WIDTH: The starting width of the window in pixels WINDOW_HEIGHT: The
 * starting height of the window in pixels
 */
#define WINDOW_WIDTH	600
#define WINDOW_HEIGHT	600

/* Color defines for the setColor function */
#define GREY	0
#define RED		1
#define GREEN	2
#define BLUE	3
#define CYAN	4
#define MAGENTA 5
#define YELLOW	6
#define BLACK	7
#define GREY2	8
#define TABLE2	8

/*
 * Magic number of degrees the teapot needs to be away from it's desired cup
 * before slowdown
 */
#define SLOWMAGIC	200

/* stores the textures */
bmpread_t		teapottex[7];

/* multiplier for Level of Detail */
int				splines = 7;

/* raw points for the Fork mesh */
float			forkmatrix[][12] = FORK;

/* animation clock */
int				ani_clock = 0;

/* true when animation is playing */
int				ani_play = 0;

/* randomly chosen from 1-4 as to which cup the teapot stops at */
int				ani_direction = 0;

/* animation speed (plays best around 1-3) */
int				speed = 5;

/* Level of detail (can be 1-4) */
int				LOD = 2;

/* Shade mode */
static GLenum	shademode = GL_SMOOTH;

/* texture array */
GLuint			texture[7];

/* used in the drawCheck function */
static int		useRGB = 1;
static int		useLighting = 1;
static int		useQuads = 1;

/* fog settings */
GLuint			fogMode[] = { GL_EXP, GL_EXP2, GL_LINEAR };
GLuint			fogfilter = 2;
GLfloat			fogColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };

/* predefined colors */
static float	materialColor[10][4] =
{
	{ 0.8, 0.8, 0.8, 1.0 },
	{ 0.8, 0.0, 0.0, 1.0 },
	{ 0.0, 0.8, 0.0, 1.0 },
	{ 0.0, 0.0, 0.8, 1.0 },
	{ 0.0, 0.8, 0.8, 1.0 },
	{ 0.8, 0.0, 0.8, 1.0 },
	{ 0.8, 0.8, 0.0, 1.0 },
	{ 0.0, 0.0, 0.0, 0.6 },
	{ 0.9, 0.7, 0.7, 1.0 },
	{ 0.9, 0.4, 0.4, 1.0 },
};

/* these will be the finishes of the models */
Finish			finPot, finFork, finCup, finTable, finDefault;

/* some glocal variables used in transformation */
float			zoom;
float			scale, alpha, beta;
int				inc;
int				inc2;
GLUquadricObj	*quadratic;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void

/* loads a texture into memory from its raw rgb data */
makeTexture(bmpread_t t, int i)
{
	glGenTextures(1, &(texture[i]));
	glBindTexture(GL_TEXTURE_2D, texture[i]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, t.width, t.height, 0, GL_RGB, GL_UNSIGNED_BYTE, t.rgb_data);
}

/*
 =======================================================================================================================
    sets the material for the next object to be drawn
 =======================================================================================================================
 */
void setMaterial(Finish f)
{
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &(f.ambient.r));
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &(f.diffuse.r));
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &(f.specular.r));
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &(f.emission.r));
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, f.shininess);
}

/*
 =======================================================================================================================
    void init() This function is called once at the beginning of program execution allowing initialisation of variables
    etc.
 =======================================================================================================================
 */
void init()
{

	/* set the finishes */
	finPot = (Finish)
	{
		{
			0.2, 0.2, 0.2, 1.0
		},
		{
			0.8, 0.8, 0.8, 1.0
		},
		{
			1.0, 1.0, 1.0, 1.0
		},
		{
			0.7, 0.7, 0.7, 1.0
		},
		1.0
	};
	finDefault = (Finish)
	{
		{
			0.2, 0.2, 0.2, 1.0
		},
		{
			0.8, 0.8, 0.8, 1.0
		},
		{
			0.0, 0.0, 0.0, 1.0
		},
		{
			0.0, 0.0, 0.0, 1.0
		},
		0.0
	};
	finTable = (Finish)
	{
		{
			0.2, 0.2, 0.2, 1.0
		},
		{
			0.8, 0.8, 0.8, 1.0
		},
		{
			0.0, 0.0, 0.0, 1.0
		},
		{
			0.0, 0.0, 0.0, 1.0
		},
		1.0
	};
	finFork = (Finish)
	{
		{
			0.2, 0.2, 0.2, 1.0
		},
		{
			0.8, 0.8, 0.8, 1.0
		},
		{
			0.5, 0.5, 0.5, 1.0
		},
		{
			0.0, 0.0, 0.0, 1.0
		},
		1.5
	};
	finCup = (Finish)
	{
		{
			0.2, 0.2, 0.2, 1.0
		},
		{
			0.6, 0.6, 0.6, 1.0
		},
		{
			0.2, 0.2, 0.2, 1.0
		},
		{
			0.2, 0.2, 0.2, 1.0
		},
		1.5
	};

	/* enable some standard things */
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	quadratic = gluNewQuadric();
	gluQuadricNormals(quadratic, GLU_SMOOTH);

	/* set initial rotation and scale values */
	alpha = -(float) 20.0;
	beta = (float) 20.0;
	scale = (float) 1.0;
	zoom = 1.0;
	inc = 0;
	inc2 = 0;

	/*
	 * read in some textures using libbmpread (much better than the one on blackboard)
	 * source: https://github.com/chazomaticus/libbmpread
	 */
	if(bmpread("b.bmp", 4, &(teapottex[0])) == 0)
	{
		printf("failed loading texture");
		perror("error");
	}

	makeTexture(teapottex[0], 0);
	if(bmpread("g.bmp", 4, &(teapottex[1])) == 0)
	{
		printf("failed loading texture");
		perror("error");
	}

	makeTexture(teapottex[1], 1);
	if(bmpread("n.bmp", 4, &(teapottex[2])) == 0)
	{
		printf("failed loading texture");
		perror("error");
	}

	makeTexture(teapottex[2], 2);
}

/*
 =======================================================================================================================
    Draws a rectangular prism
 =======================================================================================================================
 */
void drawPrism(float x, float z, float y, float height, float width, float length)
{

	/* x face */
	glNormal3f(-1, 0, 0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x, y, z + width);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x, y + height, z + width);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x, y + height, z);

	/* z face */
	glNormal3f(0, 0, 1);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z + width);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x + length, y, z + width);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x + length, y + height, z + width);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x, y + height, z + width);

	/* x face */
	glNormal3f(1, 0, 0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x + length, y, z + width);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x + length, y, z);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x + length, y + height, z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x + length, y + height, z + width);

	/* z face */
	glNormal3f(0, 0, -1);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x + length, y, z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x, y, z);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x, y + height, z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x + length, y + height, z);

	/* top face */
	glNormal3f(0, 1, 0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y + height, z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x, y + height, z + width);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x + length, y + height, z + width);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x + length, y + height, z);

	/* bottom face */
	glNormal3f(0, -1, 0);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(x, y, z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(x, y, z + width);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(x + length, y, z + width);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(x + length, y, z);
}

/*
 =======================================================================================================================
    draws a mesh, reads a 2d array where the first 3 floats are the normal and the next 9 are the 3 points for the
    triangle. Used to draw the fork
 =======================================================================================================================
 */
void drawMesh(int numMesh, float mesh[][12])
{
	glBegin(GL_TRIANGLES);

	/*~~~*/
	int ii;
	/*~~~*/

	for(ii = 0; ii < numMesh - 1; ii++)
	{
		{
			glNormal3f(mesh[ii][0], mesh[ii][1], mesh[ii][2]);

			/*~~~*/
			int jj;
			/*~~~*/

			for(jj = 1; jj < 4; jj++) glVertex3f(mesh[ii][jj * 3 + 0], mesh[ii][jj * 3 + 1], mesh[ii][jj * 3 + 2]);
			fflush(stdout);
		}
	}

	glEnd();
}

/*
 =======================================================================================================================
    draws text on the screen. Fog was messing with it so I had to disable
 =======================================================================================================================
 */
void bitmap_output(int x, int y, char *string, void *font)
{
	if(fogfilter <= 2) glDisable(GL_FOG);
	glColor4f((float) 1.0, (float) 1.0, (float) 1.0, 1.0);

	/*~~~~~~~*/
	int len, i;
	/*~~~~~~~*/

	glRasterPos2f(x, y);
	len = (int) strlen(string);
	for(i = 0; i < len; i++)
	{
		glutBitmapCharacter(font, string[i]);
		if(string[i] == '\n')
		{
			y += 15;
			glRasterPos2f(x, y);
		}
	}

	if(fogfilter <= 2) glEnable(GL_FOG);
}

/*
 =======================================================================================================================
    sets the color of the next object to draw
 =======================================================================================================================
 */
setColor(int c)
{
	glColor4fv(&materialColor[c][0]);
	if(useLighting)
	{
		{
			if(useRGB)
			{
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &materialColor[c][0]);
			}
			else
			{
				glMaterialfv(GL_FRONT_AND_BACK, GL_COLOR_INDEXES, &materialColor[c][0]);
			}
		}
	}
	else
	{
		if(useRGB)
		{
			glColor4fv(&materialColor[c][0]);
		}
		else
		{
			glIndexf(materialColor[c][1]);
		}
	}
}

/*
 =======================================================================================================================
    draws a checkered rectangle
 =======================================================================================================================
 */
static void drawCheck(int w, int h, int evenColor, int oddColor)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	static int		initialized = 0;
	static int		usedLighting = 0;
	static GLuint	checklist = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(!initialized || (usedLighting != useLighting))
	{
		{
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
			static float	square_normal[4] = { 0.0, 0.0, 1.0, 0.0 };
			static float	square[4][4];
			int				i, j;
			/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

			if(!checklist)
			{
				checklist = glGenLists(1);
			}

			glNewList(checklist, GL_COMPILE_AND_EXECUTE);
			if(useQuads)
			{
				glNormal3fv(square_normal);
				glBegin(GL_QUADS);
			}

			for(j = 0; j < h; ++j)
			{
				{
					for(i = 0; i < w; ++i)
					{
						{
							square[0][0] = -1.0 + 2.0 / w * i;
							square[0][1] = -1.0 + 2.0 / h * (j + 1);
							square[0][2] = 0.0;
							square[0][3] = 1.0;
							square[1][0] = -1.0 + 2.0 / w * i;
							square[1][1] = -1.0 + 2.0 / h * j;
							square[1][2] = 0.0;
							square[1][3] = 1.0;
							square[2][0] = -1.0 + 2.0 / w * (i + 1);
							square[2][1] = -1.0 + 2.0 / h * j;
							square[2][2] = 0.0;
							square[2][3] = 1.0;
							square[3][0] = -1.0 + 2.0 / w * (i + 1);
							square[3][1] = -1.0 + 2.0 / h * (j + 1);
							square[3][2] = 0.0;
							square[3][3] = 1.0;
							if((i & 1) ^ (j & 1))
							{
								setColor(oddColor);
							}
							else
							{
								setColor(evenColor);
							}

							if(!useQuads)
							{
								glBegin(GL_POLYGON);
							}

							glVertex4fv(&square[0][0]);
							glVertex4fv(&square[1][0]);
							glVertex4fv(&square[2][0]);
							glVertex4fv(&square[3][0]);
							if(!useQuads)
							{
								glEnd();
							}
						}
					}
				}
			}

			if(useQuads)
			{
				glEnd();
			}

			glEndList();
			initialized = 1;
			usedLighting = useLighting;
		}
	}
	else
	{
		glCallList(checklist);
	}
}

/*
 =======================================================================================================================
    draws a chair with one of two textures
 =======================================================================================================================
 */
void chair()
{
	/*~~~~~~~~~~~~~~~~~~~~~~*/
	static int	texChoose = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~*/

	setMaterial(finDefault);
	if(texChoose % 2)
		glBindTexture(GL_TEXTURE_2D, texture[0]);
	else
		glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);

	/*~~~~~~~~~~~~~~~~~~~~~*/
	float	legheight = 0.75;
	float	dist = 0.5;
	float	legfat = 0.1;
	float	seat = 0.1;
	float	seatback = 0.9;
	/*~~~~~~~~~~~~~~~~~~~~~*/

	drawPrism(0.0, 0.0, 0.0, legheight, legfat, legfat);
	drawPrism(dist, 0.0, 0.0, legheight, legfat, legfat);
	drawPrism(0.0, dist, 0.0, legheight, legfat, legfat);
	drawPrism(dist, dist, 0.0, legheight, legfat, legfat);
	drawPrism(0, 0, legheight, seat, dist + legfat, dist + legfat);
	drawPrism(0, 0, legheight + seat, seatback, dist + legfat, legfat);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	texChoose++;
}

/*
 =======================================================================================================================
    draws a cup and fork
 =======================================================================================================================
 */
void cups()
{
	glPushMatrix();
	glRotatef(270, 1.0, 0, 0);
	glScalef(0.2, 0.2, 0.2);
	glTranslatef(-1.2, 1.3, 0);
	setColor(GREY);
	setMaterial(finFork);
	drawMesh(FORKLENGTH, forkmatrix);
	glPopMatrix();
	glPushMatrix();
	setMaterial(finCup);
	setColor(GREY2);

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	float	ss = 0.45;
	float	teaouterdiabtm = ss * 0.30;
	float	teainnerdiabtm = ss * 0.25;
	float	teaouterdiatop = ss * 0.40;
	float	teainnerdiatop = ss * 0.35;
	float	gap = ss * 0.05;
	float	teaheight = ss * 0.5;
	float	tableh = 0;
	float	tablex = 0;
	float	tablez = 0.4;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	glRotatef(270, 1.0, 0, 0);
	glTranslatef(tablex, tablez, tableh);
	gluCylinder(quadratic, teaouterdiabtm, teaouterdiatop, teaheight, splines * LOD, splines * LOD);
	glPushMatrix();
	glTranslatef(0.0, 0.0, gap);
	gluCylinder(quadratic, teainnerdiabtm, teainnerdiatop, teaheight - gap, splines * LOD, splines * LOD);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0, 0, teaheight);
	gluDisk(quadratic, teainnerdiatop, teaouterdiatop, splines * LOD, splines * LOD);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0., 0.0, 0.0);
	gluDisk(quadratic, 0.00, teaouterdiabtm, splines * LOD, splines * LOD);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0., 0.0, gap);
	gluDisk(quadratic, 0.00, teainnerdiabtm, splines * LOD, splines * LOD);
	glPopMatrix();
	glPopMatrix();
}

/*
 =======================================================================================================================
    set the fog up and turn it on
 =======================================================================================================================
 */
void fog()
{
	if(fogfilter <= 2)
	{
		{
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);		/* We'll Clear To The Color Of The Fog ( Modified ) */
			glFogi(GL_FOG_MODE, fogMode[fogfilter]);	/* Fog Mode */
			glFogfv(GL_FOG_COLOR, fogColor);			/* Set Fog Color */
			glFogf(GL_FOG_DENSITY, 0.35f);			/* How Dense Will The Fog Be */
			glHint(GL_FOG_HINT, GL_DONT_CARE);		/* Fog Hint Value */
			glFogf(GL_FOG_START, 2.0f);				/* Fog Start Depth */
			glFogf(GL_FOG_END, 8.0f - zoom * 1.5);	/* Fog End Depth */
			glEnable(GL_FOG);	/* Enables GL_FOG */
		}
	}
	else
		glDisable(GL_FOG);
}

/*
 =======================================================================================================================
    Display function. Where the magic happens and everything is drawn.
 =======================================================================================================================
 */
void display(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	/* adjust for zoom value */
	gluPerspective(50 * zoom, (float) glutGet(GLUT_WINDOW_WIDTH) / (float) glutGet(GLUT_WINDOW_HEIGHT), 0.001, 40.0);
	glMatrixMode(GL_MODELVIEW);

	/* do rotation things */
	alpha = alpha + (float) inc;
	if(alpha > (float) 360.0) alpha -= (float) 360.0;
	beta = beta + (float) inc2;
	if(beta > (float) 360.0) beta -= (float) 360.0;

	/* normalise any normals which aren't in unit form */
	glEnable(GL_NORMALIZE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* draw the fog */
	fog();
	glPushMatrix();

	/* translate whole scene so we can actually see it */
	glTranslatef((float) 0.0, -(float) 0.5, -(float) 5);

	/* rotate by rotation values */
	glRotatef(beta, (float) 1.0, (float) 0.0, (float) 0.0);
	glRotatef(-alpha, (float) 0.0, (float) 1.0, (float) 0.0);

	/* it was a bit big... */
	glScalef(0.50, 0.50, 0.50);

	/* set shade mode */
	glShadeModel(shademode);
	glPushMatrix();

	/* draw the floor */
	glScalef(scale * 5, scale * 5, scale * 5);
	glRotatef(180, 0, 1.0, 1.0);
	drawCheck(20, 20, BLUE, YELLOW);	/* draw ground */
	glPopMatrix();

	/* set up the lights */
	glTranslatef(0.3, 0, 1.25);
	glEnable(GL_LIGHTING);

	/* this is a positioned light */
	glEnable(GL_LIGHT0);

	/* this is a directed light */
	glEnable(GL_LIGHT1);

	/* this is a spotlight (not initiated til later) */
	glEnable(GL_LIGHT2);

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/* set the ambient light */
	GLfloat ambientColor[] = { 0.2, 0.2, 0.2, 1.0 };
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	GLfloat lightColor0[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat lightPos0[] = { 0.0f, 1.5f, -1.0f, 0.0f };
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	GLfloat lightColor1[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat lightPos1[] = { 1.0f, 2.0f, -2.0f, 0.0f };
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1);
	glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);

	/* draw the table */
	glPushMatrix();
	glTranslatef(-0.3, 0, -1.25);
	glRotatef(180, 0, 1.0, 1.0);

	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	float	cylbase = 0.60;
	float	cyltop = 0.15;
	float	cylheight = 1.0;
	float	tableheight = 0.10;
	float	tablecirc = 0.90;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	setMaterial(finTable);
	setColor(TABLE2);
	gluCylinder(quadratic, cylbase, cyltop, cylheight, splines * LOD, splines * LOD);
	glTranslatef(0, 0, cylheight);
	gluDisk(quadratic, 0.0, cyltop, splines * LOD, splines * LOD);
	gluCylinder(quadratic, tablecirc, tablecirc, tableheight, splines * LOD, splines * LOD);
	gluDisk(quadratic, 0.0, tablecirc, splines * LOD, splines * LOD);
	glTranslatef(0, 0, tableheight);
	gluDisk(quadratic, 0.0, tablecirc, splines * LOD, splines * LOD);
	setMaterial(finDefault);
	glPopMatrix();
	glPushMatrix();

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/* initiate the spotlight */
	GLfloat light_ambient2[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat light_diffuse2[] = { 0.0, 0.3, 0, 1.0 };
	GLfloat light_specular2[] = { 0.0, 0.3, 0, 1.0 };
	GLfloat lightPos2[] = { 0.0f, 3.0f, 0.0f, 0.0f };
	GLfloat spot_direction[] = { 0, -1.0, 0 };
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	glLightfv(GL_LIGHT2, GL_AMBIENT, light_ambient2);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse2);
	glLightfv(GL_LIGHT2, GL_SPECULAR, light_specular2);
	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 30.0);
	glLightfv(GL_LIGHT2, GL_POSITION, lightPos2);
	glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, spot_direction);

	/* draw the chairs and rotate them around the table */
	glRotatef(270, 0.0, 1.0, 0.0);
	glTranslatef(-1.5, 0, 1.5);
	glPushMatrix();
	glRotatef(90, 0, 1.0, 0.0);
	chair();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.3, 0, -1.25);
	glRotatef(90, 0, 1.0, 0.0);
	glTranslatef(-0.3, 0, 1.25);
	glRotatef(90, 0, 1.0, 0.0);
	chair();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.3, 0, -1.25);
	glRotatef(180, 0, 1.0, 0.0);
	glTranslatef(-0.3, 0, 1.25);
	glRotatef(90, 0, 1.0, 0.0);
	chair();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.3, 0, -1.25);
	glRotatef(270, 0, 1.0, 0.0);
	glTranslatef(-0.3, 0, 1.25);
	glRotatef(90, 0, 1.0, 0.0);
	chair();
	glPopMatrix();

	/* draw the cups/forks and rotate around the table */
	glPushMatrix();
	glTranslatef(0.35, 1.1, -0.25);
	cups();
	glPushMatrix();
	glTranslatef(-0.05, 0, -1.0);
	glRotatef(90, 0, 1.0, 0);
	glTranslatef(0.05, 0, 1.0);
	cups();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(-0.05, 0, -1.0);
	glRotatef(180, 0, 1.0, 0);
	glTranslatef(0.05, 0, 1.0);
	cups();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(-0.05, 0, -1.0);
	glRotatef(270, 0, 1.0, 0);
	glTranslatef(0.05, 0, 1.0);
	cups();
	glPopMatrix();
	glPopMatrix();

	/* draw the teapot */
	glPushMatrix();

	/* put on table */
	glTranslatef(0.3, 1.3, -1.25);

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/* trans values for animation */
	static int		rotateval = 1;
	static int		rotatevalx = 0;
	static float	transvaly = 0;
	static float	transvalx = 0;
	/* clock values for animation */
	static int		beginslow = 0;
	static int		slowclock = 0;
	static int		pourclock = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	/* only animate when playing */
	if(ani_play)
	{
		{
			/*~~~*/
			int ii;
			/*~~~*/

			for(ii = 0; ii < speed; ii++)
			{
				{

					/* section where teapot slows */
					if((beginslow))
					{
						{
							if(((20 - slowclock / 10) > 0))
							{
								rotateval = rotateval + (20 - slowclock / 10);
								slowclock++;
							}
							else
							{
								{

									/* section where teapot is pouring */
									if(pourclock < 90)
									{
										{
											rotatevalx = pourclock / 2;
											transvaly = ((double) pourclock) / (double) 200;
											transvalx = ((double) pourclock) / (double) 400;
										}
									}
									else if(pourclock < 135)
									{	/* teapot in mid air */
										rotatevalx = 45;
										transvaly = 90.0 / 200;
										transvalx = 90.0 / 400;
									}
									else if(pourclock < 225)
									{
										{	/* teapot is being set down */
											rotatevalx = 45 - (pourclock - 135) / 2;
											transvaly = 90.0 / 200 - ((double) pourclock - 135) / (double) 200;
											transvalx = 90.0 / 400 - ((double) pourclock - 135) / (double) 400;
										}
									}
									else if(pourclock < 300)
									{		/* teapot is back on table */
										rotatevalx = 0;
										transvaly = 0;
										transvalx = 0;
									}
									else
									{
										{	/* reset variables for next play */
											ani_play = 0;
											ani_clock = 0;
											rotateval = 1;
											ani_direction = 0;
											slowclock = 0;
											pourclock = -speed;
											beginslow = 0;
											rotatevalx = 0;
											transvaly = 0;
											transvalx = 0;
										}
									}

									pourclock++;
								}
							}
						}
					}
					else if(ani_clock < 200)
					{			/* teapot is speeding up it's spin */
						rotateval = rotateval + ani_clock / 10;
					}
					else if(ani_clock < 500)
					{
						{		/* teapot is at constant spin */
							rotateval = rotateval + 20;
							if(ani_clock == 300)
							{	/* choose the cup it will stop at */
								ani_direction = rand() % 4 + 1;
							}
						}
					}

					if(rotateval > 360) rotateval -= 360;
					if
					(
						(!beginslow)
					&&	(ani_direction != 0)
					&&	(
							(
								(rotateval >= ani_direction * 90 - SLOWMAGIC - 20)
							&&	(rotateval <= ani_direction * 90 - SLOWMAGIC)
							)
						||	(
									(rotateval >= ani_direction * 90 - SLOWMAGIC + 340)
								&&	(rotateval <= ani_direction * 90 - SLOWMAGIC + 360)
								)
						)
					)
					{			/* choose opportune moment to slow down */
						beginslow = 1;
						rotateval = ani_direction * 90 - SLOWMAGIC + 360;
						if(rotateval > 360) rotateval -= 360;
					}

					ani_clock++;
				}
			}
		}	/* apply transformations */
	}

	glRotatef(rotateval, 0, 1.0, 0);
	glTranslatef(transvalx, transvaly, 0);
	glRotatef(-rotatevalx, 0, 0, 1.0);

	/* apply texture */
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_TEXTURE_2D);

	/* apply finish */
	setMaterial(finPot);
	glutSolidTeapot(0.2);
	setMaterial(finDefault);
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
	glPopMatrix();
	glPopMatrix();

	/* 3D stuff is done */
	glFlush();

	/* draw text overlay */
	glDisable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	speedstring[20];
	char	str[500] = "<Z> and <z>: zoom in/out\n<X> or <x>: Start/Stop X rotation\n<Y> or <y> Start/Stop Y Rotation\n<A> or <a>: Start animation\n<F> or <f>: Speed up animation\n<S> or <s>: Slow down animation\n<T> or <t>: Pause animation\n<C> or <c>: Resume the animation\n<p>: switch the rendering to the flat shaded polygonization.\n<P> switch the rendering to the smooth shaded polygonization.\n<q> or <Q>: change fog mode\nspeed: ";
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	sprintf(speedstring, "%d", speed);
	strcat(str, speedstring);
	strcat(str, "\nLOD: ");
	sprintf(speedstring, "%d", LOD);
	strcat(str, speedstring);
	strcat(str, "\nFog Mode: ");
	switch(fogfilter)
	{
		{
		case 0: strcat(str, "GL_EXP"); break;
		case 1: strcat(str, "GL_EXP2"); break;
		case 2: strcat(str, "GL_LINEAR"); break;
		case 3: strcat(str, "NO FOG"); break;
		}
	}

	bitmap_output(30, 30, str, GLUT_BITMAP_HELVETICA_12);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	/* draw transparent box for text */
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f((float) 0, (float) 0, (float) 0, (float) 0.6);
	glRecti(30 - 10, 30 - 20, 40 + 350, 30 + 250);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glFlush();
	glutSwapBuffers();
}

/*
 =======================================================================================================================
    resize the window
 =======================================================================================================================
 */
void reshape(int width, int height)
{
	glViewport(0, 0, (GLsizei) width, (GLsizei) height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(50 * zoom, (float) width / (float) height, 0.001, 40.0);
	glMatrixMode(GL_MODELVIEW);
}

/*
 =======================================================================================================================
    void input(unsigned char key, int x, int y) A callback function for when the user presses a key. Pressing the 'r'
    key will either stop or start the rotation of the drawn object.
 =======================================================================================================================
 */
void input(unsigned char key, int x, int y)
{
	switch(key)
	{
		{
		case 'y':
		case 'Y':
			{

				/* rotate along y */
				if(inc == 1)
					inc = 0;
				else
					inc = 1;
				break;
			}

		case 'x':
		case 'X':
			{

				/* rotate along x */
				if(inc2 == 1)
					inc2 = 0;
				else
					inc2 = 1;
				break;
			}

		case 'z':	/* zoom out and change LOD if needed */
			{
				zoom = zoom + zoom * 0.1;
				if(zoom > 2) zoom = 2;
				if(zoom <= 0.5)
					LOD = 4;
				else if(zoom <= 1.0)
				{
					LOD = 3;
				}
				else if(zoom <= 1.5)
				{
					LOD = 2;
				}
				else if(zoom <= 2.0)
				{
					LOD = 1;
				}
				break;
			}

		case 'Z':	/* zoom in and change LOD if needed */
			{
				zoom = zoom - zoom * 0.1;
				if(zoom <= 0.5)
					LOD = 4;
				else if(zoom <= 1.0)
				{
					LOD = 3;
				}
				else if(zoom <= 1.5)
				{
					LOD = 2;
				}
				else if(zoom <= 2.0)
				{
					LOD = 1;
				}
				break;
			}

		case 'a':	/* begin animation */

		case 'A':
			{
				ani_play = 1;
				break;
			}

		case 'f':
		case 'F':
			{

				/* faster animate */
				speed++;
				break;
			}

		case 's':
		case 'S':
			{

				/* slower animate */
				speed--;
				if(speed < 1) speed = 1;
				break;
			}

		case 't':
		case 'T':
			{

				/* pause animate */
				ani_play = 0;
				break;
			}

		case 'c':
		case 'C':
			{

				/* resume animate */
				ani_play = 1;
				break;
			}

		case 'p':
			{		/* flat shading */
				shademode = GL_FLAT;
				break;
			}

		case 'P':
			{		/* smooth shading */
				shademode = GL_SMOOTH;
				break;
			}

		case 'q':
		case 'Q':
			{		/* change fog mode */
				fogfilter++;
				if(fogfilter > 3) fogfilter = 0;
			}
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void nextFrame(int x)
{	/* set fps */
	glutPostRedisplay();
	glutTimerFunc(20, nextFrame, 0);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int main(int argc, char **argv)
{

	/*~~~~~~*/
	/* seed the rand function */
	time_t	t;
	/*~~~~~~*/

	srand(time(0));

	/* initialise glut and a window */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(100, 100);

	/* create a window and initialise the program */
	glutCreateWindow("Spin the Teapot: Can you guess where it lands next?");
	init();

	/* set callback functions */
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(input);
	nextFrame(0);
	glutMainLoop();
	return 0;
}
