// VERSION 1.2

#include "globals.h"
#include "misc_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "animations.h"
#include "SDL.h"
#include "SDL_main.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if WIN32
#include <windows.h>
#endif
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

int glerrornum;
#define CHECKGLERROR if ((glerrornum = glGetError())) GL_PrintError(glerrornum, __FILE__, __LINE__);

typedef int GLint;
typedef unsigned int GLuint;
typedef int GLenum;
typedef int GLsizei;
typedef float GLfloat;
typedef double GLdouble;
typedef void GLvoid;
typedef float GLclampf;
typedef int GLbitfield;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;

typedef int GLintptrARB;
typedef int GLsizeiptrARB;


static int quit;

#define GL_NO_ERROR 				0x0
#define GL_INVALID_VALUE			0x0501
#define GL_INVALID_ENUM				0x0500
#define GL_INVALID_OPERATION			0x0502
#define GL_STACK_OVERFLOW			0x0503
#define GL_STACK_UNDERFLOW			0x0504
#define GL_OUT_OF_MEMORY			0x0505

void GL_PrintError(int errornumber, char *filename, int linenumber)
{
	switch(errornumber)
	{
#ifdef GL_INVALID_ENUM
	case GL_INVALID_ENUM:
		printf("GL_INVALID_ENUM at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_INVALID_VALUE
	case GL_INVALID_VALUE:
		printf("GL_INVALID_VALUE at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_INVALID_OPERATION
	case GL_INVALID_OPERATION:
		printf("GL_INVALID_OPERATION at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_STACK_OVERFLOW
	case GL_STACK_OVERFLOW:
		printf("GL_STACK_OVERFLOW at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_STACK_UNDERFLOW
	case GL_STACK_UNDERFLOW:
		printf("GL_STACK_UNDERFLOW at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_OUT_OF_MEMORY
	case GL_OUT_OF_MEMORY:
		printf("GL_OUT_OF_MEMORY at %s:%i\n", filename, linenumber);
		break;
#endif
#ifdef GL_TABLE_TOO_LARGE
    case GL_TABLE_TOO_LARGE:
		printf("GL_TABLE_TOO_LARGE at %s:%i\n", filename, linenumber);
		break;
#endif
	default:
		printf("GL UNKNOWN (%i) at %s:%i\n", errornumber, filename, linenumber);
		break;
	}
}


typedef struct _TargaHeader
{
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
}
TargaHeader;

unsigned char *LoadTGA (const char *filename, int *imagewidth, int *imageheight)
{
	int x, y, row_inc, image_width, image_height, red, green, blue, alpha, run, runlen, loadsize;
	unsigned char *pixbuf, *image_rgba, *f;
	const unsigned char *fin, *enddata;
	TargaHeader targa_header;

	*imagewidth = 0;
	*imageheight = 0;

	f = loadfile(filename, &loadsize);
	if (!f)
		return NULL;
	if (loadsize < 18+3)
	{
		free(f);
		printf("%s is too small to be valid\n", filename);
		return NULL;
	}
	targa_header.id_length = f[0];
	targa_header.colormap_type = f[1];
	targa_header.image_type = f[2];

	targa_header.colormap_index = f[3] + f[4] * 256;
	targa_header.colormap_length = f[5] + f[6] * 256;
	targa_header.colormap_size = f[7];
	targa_header.x_origin = f[8] + f[9] * 256;
	targa_header.y_origin = f[10] + f[11] * 256;
	targa_header.width = f[12] + f[13] * 256;
	targa_header.height = f[14] + f[15] * 256;
	targa_header.pixel_size = f[16];
	targa_header.attributes = f[17];

	if (targa_header.image_type != 2 && targa_header.image_type != 10)
	{
		free(f);
		printf("%s is is not type 2 or 10\n", filename);
		return NULL;
	}

	if (targa_header.colormap_type != 0	|| (targa_header.pixel_size != 32 && targa_header.pixel_size != 24))
	{
		free(f);
		printf("%s is not 24bit BGR or 32bit BGRA\n", filename);
		return NULL;
	}

	enddata = f + loadsize;

	image_width = targa_header.width;
	image_height = targa_header.height;

	image_rgba = malloc(image_width * image_height * 4);
	if (!image_rgba)
	{
		free(f);
		printf("failed to allocate memory for decoding %s\n", filename);
		return NULL;
	}

	*imagewidth = image_width;
	*imageheight = image_height;

	fin = f + 18;
	if (targa_header.id_length != 0)
		fin += targa_header.id_length;  // skip TARGA image comment

	// If bit 5 of attributes isn't set, the image has been stored from bottom to top
	if ((targa_header.attributes & 0x20) == 0)
	{
		pixbuf = image_rgba + (image_height - 1)*image_width*4;
		row_inc = -image_width*4*2;
	}
	else
	{
		pixbuf = image_rgba;
		row_inc = 0;
	}

	if (targa_header.image_type == 2)
	{
		// Uncompressed, RGB images
		if (targa_header.pixel_size == 24)
		{
			if (fin + image_width * image_height * 3 <= enddata)
			{
				for(y = 0;y < image_height;y++)
				{
					for(x = 0;x < image_width;x++)
					{
						*pixbuf++ = fin[2];
						*pixbuf++ = fin[1];
						*pixbuf++ = fin[0];
						*pixbuf++ = 255;
						fin += 3;
					}
					pixbuf += row_inc;
				}
			}
		}
		else
		{
			if (fin + image_width * image_height * 4 <= enddata)
			{
				for(y = 0;y < image_height;y++)
				{
					for(x = 0;x < image_width;x++)
					{
						*pixbuf++ = fin[2];
						*pixbuf++ = fin[1];
						*pixbuf++ = fin[0];
						*pixbuf++ = fin[3];
						fin += 4;
					}
					pixbuf += row_inc;
				}
			}
		}
	}
	else if (targa_header.image_type==10)
	{
		// Runlength encoded RGB images
		x = 0;
		y = 0;
		while (y < image_height && fin < enddata)
		{
			runlen = *fin++;
			if (runlen & 0x80)
			{
				// RLE compressed run
				runlen = 1 + (runlen & 0x7f);
				if (targa_header.pixel_size == 24)
				{
					if (fin + 3 > enddata)
						break;
					blue = *fin++;
					green = *fin++;
					red = *fin++;
					alpha = 255;
				}
				else
				{
					if (fin + 4 > enddata)
						break;
					blue = *fin++;
					green = *fin++;
					red = *fin++;
					alpha = *fin++;
				}

				while (runlen && y < image_height)
				{
					run = runlen;
					if (run > image_width - x)
						run = image_width - x;
					x += run;
					runlen -= run;
					while(run--)
					{
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alpha;
					}
					if (x == image_width)
					{
						// end of line, advance to next
						x = 0;
						y++;
						pixbuf += row_inc;
					}
				}
			}
			else
			{
				// RLE uncompressed run
				runlen = 1 + (runlen & 0x7f);
				while (runlen && y < image_height)
				{
					run = runlen;
					if (run > image_width - x)
						run = image_width - x;
					x += run;
					runlen -= run;
					if (targa_header.pixel_size == 24)
					{
						if (fin + run * 3 > enddata)
							break;
						while(run--)
						{
							*pixbuf++ = fin[2];
							*pixbuf++ = fin[1];
							*pixbuf++ = fin[0];
							*pixbuf++ = 255;
							fin += 3;
						}
					}
					else
					{
						if (fin + run * 4 > enddata)
							break;
						while(run--)
						{
							*pixbuf++ = fin[2];
							*pixbuf++ = fin[1];
							*pixbuf++ = fin[0];
							*pixbuf++ = fin[3];
							fin += 4;
						}
					}
					if (x == image_width)
					{
						// end of line, advance to next
						x = 0;
						y++;
						pixbuf += row_inc;
					}
				}
			}
		}
	}
	free(f);
	return image_rgba;
}


#define GL_PROJECTION				0x1701
#define GL_MODELVIEW				0x1700
#define GL_UNSIGNED_BYTE			0x1401
#define GL_MAX_TEXTURE_SIZE			0x0D33
#define GL_TEXTURE_2D				0x0DE1
#define GL_RGBA					0x1908
#define GL_QUADS				0x0007
#define GL_COLOR_BUFFER_BIT			0x00004000
#define GL_DEPTH_BUFFER_BIT			0x00000100
#define GL_UNSIGNED_SHORT			0x1403
#define GL_DEPTH_TEST				0x0B71
#define GL_CULL_FACE				0x0B44
#define GL_TEXTURE_MAG_FILTER			0x2800
#define GL_TEXTURE_MIN_FILTER			0x2801
#define GL_LINEAR				0x2601
#define GL_VERTEX_ARRAY				0x8074
#define GL_NORMAL_ARRAY				0x8075
#define GL_COLOR_ARRAY				0x8076
#define GL_TEXTURE_COORD_ARRAY			0x8078
#define GL_FLOAT				0x1406
#define GL_TRIANGLES				0x0004
#define GL_UNSIGNED_INT				0x1405
#define GL_LINES                          0x0001

void (GLAPIENTRY *glEnable)(GLenum cap);
void (GLAPIENTRY *glDisable)(GLenum cap);
void (GLAPIENTRY *glGetIntegerv)(GLenum pname, GLint *params);
const GLubyte *(GLAPIENTRY *glGetString)(GLenum name);
void (GLAPIENTRY *glOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
void (GLAPIENTRY *glFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
void (GLAPIENTRY *glGenTextures)(GLsizei n, GLuint *textures);
void (GLAPIENTRY *glBindTexture)(GLenum target, GLuint texture);
void (GLAPIENTRY *glTexImage2D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void (GLAPIENTRY *glTexParameteri)(GLenum target, GLenum pname, GLint param);
void (GLAPIENTRY *glColor4f)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void (GLAPIENTRY *glTexCoord2f)(GLfloat s, GLfloat t);
void (GLAPIENTRY *glVertex3f)(GLfloat x, GLfloat y, GLfloat z);
void (GLAPIENTRY *glBegin)(GLenum mode);
void (GLAPIENTRY *glEnd)(void);
GLenum (GLAPIENTRY *glGetError)(void);
void (GLAPIENTRY *glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void (GLAPIENTRY *glClear)(GLbitfield mask);
void (GLAPIENTRY *glMatrixMode)(GLenum mode);
void (GLAPIENTRY *glLoadIdentity)(void);
void (GLAPIENTRY *glRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void (GLAPIENTRY *glTranslatef)(GLfloat x, GLfloat y, GLfloat z);
void (GLAPIENTRY *glVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
void (GLAPIENTRY *glNormalPointer)(GLenum type, GLsizei stride, const GLvoid *ptr);
void (GLAPIENTRY *glColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
void (GLAPIENTRY *glTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
void (GLAPIENTRY *glDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void (GLAPIENTRY *glDrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

void (GLAPIENTRY *glBindBufferARB) (GLenum target, GLuint buffer);
void (GLAPIENTRY *glDeleteBuffersARB) (GLsizei n, const GLuint *buffers);
void (GLAPIENTRY *glGenBuffersARB) (GLsizei n, GLuint *buffers);
GLboolean (GLAPIENTRY *glIsBufferARB) (GLuint buffer);
GLvoid* (GLAPIENTRY *glMapBufferARB) (GLenum target, GLenum access);
GLboolean (GLAPIENTRY *glUnmapBufferARB) (GLenum target);
void (GLAPIENTRY *glBufferDataARB) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
void (GLAPIENTRY *glBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);

//void (GLAPIENTRY *glDisable)(GLenum cap);
void (GLAPIENTRY *glCullFace) (GLenum mode);
#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405

#define GL_EXTENSIONS                     0x1F03
#define GL_ARRAY_BUFFER_ARB               0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB       0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB       0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_STREAM_DRAW_ARB                0x88E0
#define GL_STREAM_READ_ARB                0x88E1
#define GL_STREAM_COPY_ARB                0x88E2
#define GL_STATIC_DRAW_ARB                0x88E4
#define GL_STATIC_READ_ARB                0x88E5
#define GL_STATIC_COPY_ARB                0x88E6
#define GL_DYNAMIC_DRAW_ARB               0x88E8
#define GL_DYNAMIC_READ_ARB               0x88E9
#define GL_DYNAMIC_COPY_ARB               0x88EA
#define GL_READ_ONLY_ARB                  0x88B8
#define GL_WRITE_ONLY_ARB                 0x88B9
#define GL_READ_WRITE_ARB                 0x88BA
#define GL_BUFFER_SIZE_ARB                0x8764
#define GL_BUFFER_USAGE_ARB               0x8765
#define GL_BUFFER_ACCESS_ARB              0x88BB
#define GL_BUFFER_MAPPED_ARB              0x88BC
#define GL_BUFFER_MAP_POINTER_ARB         0x88BD


void resampleimage(unsigned char *inpixels, int inwidth, int inheight, unsigned char *outpixels, int outwidth, int outheight)
{
	unsigned char *inrow, *inpix;
	int x, y, xf, xfstep;
	xfstep = (int) (inwidth * 65536.0f / outwidth);
	for (y = 0;y < outheight;y++)
	{
		inrow = inpixels + ((y * inheight / outheight) * inwidth * 4);
		for (x = 0, xf = 0;x < outwidth;x++, xf += xfstep)
		{
			inpix = inrow + (xf >> 16) * 4;
			outpixels[0] = inpix[0];
			outpixels[1] = inpix[1];
			outpixels[2] = inpix[2];
			outpixels[3] = inpix[3];
			outpixels += 4;
		}
	}
}

GLuint maxtexturesize;

#define IMAGETEXTURE_HASHINDICES 1024
typedef struct imagetexture_s
{
	struct imagetexture_s *next;
	char *name;
	GLuint texnum;
}
imagetexture_t;

imagetexture_t *imagetexturehash[IMAGETEXTURE_HASHINDICES];

void initimagetextures(void)
{
	int i;
	for (i = 0;i < IMAGETEXTURE_HASHINDICES;i++)
		imagetexturehash[i] = NULL;
}

int textureforimage(const char *name)
{
	int i, hashindex, pixels_width, pixels_height, width, height, alpha;
	unsigned char *pixels, *texturepixels;
	imagetexture_t *image;
	char nametga[1024];
	hashindex = 0;
	for (i = 0;name[i];i++)
		hashindex += name[i];
	hashindex = (hashindex + i) % IMAGETEXTURE_HASHINDICES;
	for (image = imagetexturehash[hashindex];image;image = image->next)
		if (!strcmp(image->name, name))
			return image->texnum;
	image = malloc(sizeof(imagetexture_t));
	image->name = malloc(strlen(name) + 1);
	strcpy(image->name, name);
	image->texnum = 0;
	image->next = imagetexturehash[hashindex];
	imagetexturehash[hashindex] = image;

	pixels = LoadTGA(name, &pixels_width, &pixels_height);
	if (!pixels)
	{
		sprintf(nametga, "%s.tga", name);
		pixels = LoadTGA(nametga, &pixels_width, &pixels_height);
		if (!pixels)
			printf("failed both %s and %s\n", name, nametga);
	}
	if (pixels)
	{
		for (width = 1;width < pixels_width && width < (int)maxtexturesize;width *= 2);
		for (height = 1;height < pixels_height && height < (int)maxtexturesize;height *= 2);
		texturepixels = malloc(width * height * 4);
		resampleimage(pixels, pixels_width, pixels_height, texturepixels, width, height);
		alpha = 0;
		for (i = 3;i < width * height * 4;i += 4)
			if (texturepixels[i] < 255)
				alpha = 1;
		CHECKGLERROR
		glGenTextures(1, &image->texnum);CHECKGLERROR
		glBindTexture(GL_TEXTURE_2D, image->texnum);CHECKGLERROR
		glTexImage2D(GL_TEXTURE_2D, 0, alpha ? 4 : 3, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texturepixels);CHECKGLERROR
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);CHECKGLERROR
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);CHECKGLERROR
		free(texturepixels);
		free(pixels);
	}
	return image->texnum;
}

void bindimagetexture(const char *name)
{
	CHECKGLERROR
	glBindTexture(GL_TEXTURE_2D, textureforimage(name));
	CHECKGLERROR
}

#define MAX_TEXTUREUNITS 16

int textureunits = 1;

int varraysize;

typedef struct {
	float	v[3];
} vertex_t;

typedef struct {
	float	v[2];
} texcoord_t;

vertex_t *varray_vertex;
vertex_t *varray_normal;
texcoord_t *varray_texcoord;

int vbo_enable = 0;
int vbo_ext = 0;
const char *ext_string = NULL;
int ext_vbo = 0;
int ext_drawrangeelements = 0;

unsigned int vbo_bufs[3] = { 0, 0, 0};

void R_Mesh_Init(void)
{
	varraysize = 0;
	varray_vertex = NULL;
	varray_normal = NULL;
	varray_texcoord = NULL;
}

void R_Mesh_ResizeCheck(int numverts)
{
	if (varraysize < numverts)
	{
		if (ext_vbo)
			glDeleteBuffersARB(3, vbo_bufs);

		varraysize = numverts;
		if (varray_vertex)
			free(varray_vertex);
		if (varray_normal)
			free(varray_normal);
		if (varray_texcoord)
			free(varray_texcoord);
		varray_vertex = calloc(varraysize, sizeof(vertex_t));
		varray_normal = calloc(varraysize, sizeof(vertex_t));
		varray_texcoord = calloc(varraysize, sizeof(texcoord_t));

		glEnable(GL_VERTEX_ARRAY);
		glEnable(GL_NORMAL_ARRAY);
		glEnable(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_COLOR_ARRAY);
	}
}


void R_DrawMesh( unsigned int num_verts, unsigned int num_tris, unsigned int *indices, const char *shadername )
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT); //SKM winding is for front culling

	if (vbo_enable)
	{
		if (!vbo_bufs[0])
		{
			glGenBuffersARB(3, vbo_bufs);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_bufs[0]);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(vertex_t) * varraysize, NULL, GL_DYNAMIC_DRAW_ARB);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_bufs[1]);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(vertex_t) * varraysize, NULL, GL_DYNAMIC_DRAW_ARB);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_bufs[2]);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(texcoord_t) * varraysize, NULL, GL_DYNAMIC_DRAW_ARB);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		}
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_bufs[0]);
		glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(vertex_t) * num_verts, varray_vertex);
		glVertexPointer(3, GL_FLOAT, 0, 0);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_bufs[1]);
		glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(vertex_t) * num_verts, varray_normal);
		glNormalPointer(GL_FLOAT, 0, 0);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_bufs[2]);
		glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sizeof(texcoord_t) * num_verts, varray_texcoord);
		glTexCoordPointer(2, GL_FLOAT, 0, 0);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	else
	{
		glVertexPointer(3, GL_FLOAT, 0, varray_vertex);
		glNormalPointer(GL_FLOAT, 0, varray_normal);
		glTexCoordPointer(2, GL_FLOAT, 0, varray_texcoord);
	}
	glColor4f(1,1,1,1);
	bindimagetexture(shadername);
	if (ext_drawrangeelements)
		glDrawRangeElements(GL_TRIANGLES, 0, num_verts, num_tris * 3, GL_UNSIGNED_INT, indices);
	else
		glDrawElements(GL_TRIANGLES, num_tris * 3, GL_UNSIGNED_INT, indices);
}

void R_DrawLine(float origin[3], float origin2[3], float color[3], int depthtest)
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	if( depthtest ) glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);

    glColor4f(color[RED],color[GREEN],color[BLUE], 1.0f);
	glBegin(GL_LINES);
		glVertex3f( origin[0] , origin[1], origin[2]  );
		glVertex3f( origin2[0] , origin2[1] , origin2[2] );
	glEnd();
}

void R_DrawAxis( void )
{
	float origin1[3];
	float origin2[3];
	float color[3] = { 0.6f, 0.6f, 0.6f };
	float axis_size = 6;
	float front_axis_size = 18;

	origin1[0] = 0;
	origin1[1] = 0;
	origin1[2] = axis_size;	//up
	origin2[0] = 0;
	origin2[1] = 0;
	origin2[2] = -axis_size;
	R_DrawLine( origin1, origin2, color, 0 );

	origin1[0] = 0;
	origin1[1] = axis_size;	//right
	origin1[2] = 0;
	origin2[0] = 0;
	origin2[1] = -axis_size;
	origin2[2] = 0;
	R_DrawLine( origin1, origin2, color, 0 );

	origin1[0] = front_axis_size;
	origin1[1] = 0;
	origin1[2] = 0;
	origin2[0] = -axis_size;
	origin2[1] = 0;
	origin2[2] = 0;
	R_DrawLine( origin1, origin2, color, 0 );

}

SDL_Surface *initvideo(int width, int height, int bpp, int fullscreen)
{
	SDL_Surface *surface;
	if (SDL_GL_LoadLibrary (NULL))
	{
		printf("Unable to load GL library\n");
		SDL_Quit();
		exit(1);
	}
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
	surface = SDL_SetVideoMode(640, 480, 16, SDL_OPENGL | (fullscreen ? (SDL_FULLSCREEN | SDL_DOUBLEBUF) : 0));
	if (!surface)
		return NULL;

	glEnable = SDL_GL_GetProcAddress("glEnable");
	glDisable = SDL_GL_GetProcAddress("glDisable");
	glGetIntegerv = SDL_GL_GetProcAddress("glGetIntegerv");
	glGetString = SDL_GL_GetProcAddress("glGetString");
	glOrtho = SDL_GL_GetProcAddress("glOrtho");
	glFrustum = SDL_GL_GetProcAddress("glFrustum");
	glGenTextures = SDL_GL_GetProcAddress("glGenTextures");
	glBindTexture = SDL_GL_GetProcAddress("glBindTexture");
	glTexImage2D = SDL_GL_GetProcAddress("glTexImage2D");
	glTexParameteri = SDL_GL_GetProcAddress("glTexParameteri");
	glColor4f = SDL_GL_GetProcAddress("glColor4f");
	glTexCoord2f = SDL_GL_GetProcAddress("glTexCoord2f");
	glVertex3f = SDL_GL_GetProcAddress("glVertex3f");
	glBegin = SDL_GL_GetProcAddress("glBegin");
	glEnd = SDL_GL_GetProcAddress("glEnd");
	glGetError = SDL_GL_GetProcAddress("glGetError");
	glClearColor = SDL_GL_GetProcAddress("glClearColor");
	glClear = SDL_GL_GetProcAddress("glClear");
	glMatrixMode = SDL_GL_GetProcAddress("glMatrixMode");
	glLoadIdentity = SDL_GL_GetProcAddress("glLoadIdentity");
	glRotatef = SDL_GL_GetProcAddress("glRotatef");
	glTranslatef = SDL_GL_GetProcAddress("glTranslatef");
	glVertexPointer = SDL_GL_GetProcAddress("glVertexPointer");
	glNormalPointer = SDL_GL_GetProcAddress("glNormalPointer");
	glColorPointer = SDL_GL_GetProcAddress("glColorPointer");
	glTexCoordPointer = SDL_GL_GetProcAddress("glTexCoordPointer");
	glDrawElements = SDL_GL_GetProcAddress("glDrawElements");
	glDrawRangeElements = SDL_GL_GetProcAddress("glDrawRangeElements");

	glCullFace = SDL_GL_GetProcAddress("glCullFace");

	glBindBufferARB = SDL_GL_GetProcAddress("glBindBufferARB");
	glDeleteBuffersARB = SDL_GL_GetProcAddress("glDeleteBuffersARB");
	glGenBuffersARB = SDL_GL_GetProcAddress("glGenBuffersARB");
	glIsBufferARB = SDL_GL_GetProcAddress("glIsBufferARB");
	glMapBufferARB = SDL_GL_GetProcAddress("glMapBufferARB");
	glUnmapBufferARB = SDL_GL_GetProcAddress("glUnmapBufferARB");
	glBufferDataARB = SDL_GL_GetProcAddress("glBufferDataARB");
	glBufferSubDataARB = SDL_GL_GetProcAddress("glBufferSubDataARB");

	ext_string = glGetString(GL_EXTENSIONS);
	ext_drawrangeelements = strstr(ext_string, "GL_EXT_draw_range_elements") != NULL;
	ext_vbo = strstr(ext_string, "GL_ARB_vertex_buffer_object") != NULL;

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexturesize);
	return surface;
}

void drawstring(const char *string, float x, float y, float scalex, float scaley)
{
	int num;
	float bases, baset, scales, scalet;
	scales = 1.0f / 16.0f;
	scalet = 1.0f / 16.0f;
	glBegin(GL_QUADS);
	while (*string)
	{
		num = *string++;
		if (num != ' ')
		{
			bases = ((num & 15) * scales);
			baset = ((num >> 4) * scalet);
			glTexCoord2f(bases         , baset         );glVertex3f(x         , y         , 10);
			glTexCoord2f(bases         , baset + scalet);glVertex3f(x         , y + scaley, 10);
			glTexCoord2f(bases + scales, baset + scalet);glVertex3f(x + scalex, y + scaley, 10);
			glTexCoord2f(bases + scales, baset         );glVertex3f(x + scalex, y         , 10);
		}
		x += scalex;
	}
	glEnd();
}


typedef struct
{
	float	origin[3];
	float	angles[3];
} cam_t;
cam_t cam;

void clamp(float *v)
{
    int i;
    for (i = 0; i < 3; i ++) {
		while( v[i] > 360 ) v[i] -= 360;
		while( v[i] < -360 ) v[i] += 360;
	}
}


int		SKM_LoadModel( char* filename );
void	SKM_Draw( int showmodel, int showskeleton, int curframe, int oldframe, float lerp );
void	SKM_freemodel(void);


void viewer_main(char *filename, int width, int height, int bpp, int fullscreen)
{
	char	caption[1024];
	float	xmax, ymax, zNear, zFar;
	int		currenttime;
	int		playback;
	SDL_Event event;
	SDL_Surface *surface;
	int sceneframerate;
	char	tempstring[256];
	int		fps = 0, fpsframecount = 0;
	double	fpsbasetime = 0;
	int		frame = 0, oldframe = 0;
	char	*animationname;
	double	prevframetime = 0, nextframetime = 0;
	double	frametime = 0;
	float	frontlerp;
	int		showskeleton = 1, showhelp = 0, showmodel = 1, showaxis = 1;
	int		mousex, mousey, mousemovex, mousemovey, buttonmask; //mouse input
	float	sensitivity = 0.5f;


	glerrornum = 0;
	quit = 0;

	InitSwapFunctions();
	if( !SKM_LoadModel(filename) )
	{
		printf("unable to load %s\n", filename);
		return;
	}
	
	printf("Initializing SDL.\n");

	/* Initialize defaults, Video and Audio */
	if ((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1))
	{
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		SDL_Quit();
		exit(-1);
	}
	animationname = SMViewer_NextFrame( &frame, &oldframe );//set up the first frame

	surface = initvideo(width, height, bpp, fullscreen);

	initimagetextures();
//	skmprecacheimages();

	//glClearColor(0,0,0,0);
	printf("SDL initialized.\n");

	printf("using an SDL opengl %dx%dx%dbpp surface.\n", surface->w, surface->h, surface->format->BitsPerPixel);

	sprintf(caption, "skmviewer: %s", filename);
	SDL_WM_SetCaption(caption, NULL);

	SDL_EnableUNICODE(1);

	playback = 1;
	currenttime = SDL_GetTicks();
	sceneframerate = 24;
	cam.origin[2] = 64;
	cam.angles[1] = -90;
	cam.angles[0] = -90;clamp(cam.angles);

	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_LEFT:
					animationname = SMViewer_NextAnimation( &frame, &oldframe, -1 );
					break;
				case SDLK_RIGHT:
					animationname = SMViewer_NextAnimation( &frame, &oldframe, 1 );
					break;
				case SDLK_UP:
					if( !playback ) animationname = SMViewer_NextFrame( &frame, &oldframe );
					break;
				case SDLK_DOWN:
					if( !playback ) animationname = SMViewer_PrevFrame( &frame, &oldframe );
					break;
				case SDLK_PAGEUP:
					break;
				case SDLK_PAGEDOWN:
					break;
				case SDLK_COMMA:
					animationname = SMViewer_NextAnimation( &frame, &oldframe, -1 );
					break;
				case SDLK_PERIOD:
					animationname = SMViewer_NextAnimation( &frame, &oldframe, 1 );
					break;
				case SDLK_KP_PLUS:
					if(sceneframerate < 200)
						sceneframerate++;
					break;
				case SDLK_KP_MINUS:
					if(sceneframerate > 1)
						sceneframerate--;
					break;
				case SDLK_PLUS:
					if(sceneframerate < 200)
						sceneframerate++;
					break;
				case SDLK_MINUS:
					if(sceneframerate > 1)
						sceneframerate--;
					break;
				case SDLK_HOME:
					break;
				case SDLK_END:
					break;
				case SDLK_0:
				case SDLK_1:
				case SDLK_2:
				case SDLK_3:
				case SDLK_4:
				case SDLK_5:
				case SDLK_6:
				case SDLK_7:
				case SDLK_8:
				case SDLK_9:
					// keep last two characters
					sceneframerate = (sceneframerate % 10) * 10 + (event.key.keysym.sym - SDLK_0);
					if( sceneframerate < 1 ) sceneframerate = 1;
					break;
				case SDLK_SPACE:
					playback = !playback;
					break;
				case SDLK_ESCAPE:
					quit = 1;
					break;
				default:
					if (event.key.keysym.unicode == 's')
					{
						showskeleton = !showskeleton;
					}
					if (event.key.keysym.unicode == 'm')
					{
						showmodel = !showmodel;
					}
					if (event.key.keysym.unicode == 'a')
					{
						showaxis = !showaxis;
					}
					if (event.key.keysym.unicode == 'h')
					{
						showhelp = !showhelp;
					}
					if (event.key.keysym.unicode == 'v')
					{
						if (ext_vbo)
						{
							vbo_enable = !vbo_enable;
							if (vbo_enable)
								printf("vbo enabled\n");
							else
								printf("vbo disabled\n");
						}
						else
							printf("vbo toggle needs GL_ARB_vertex_buffer_object extension\n");
					}
					break;
				}
				break;
			case SDL_KEYUP:
				break;
			case SDL_QUIT:
				quit = 1;
				break;
			default:
				break;
			}
		}
		if (quit)
			break;
		
		//get mouse input
		SDL_GetMouseState( &mousex, &mousey ); //not used right now
		buttonmask = SDL_GetRelativeMouseState( &mousemovex, &mousemovey );
		if( buttonmask & SDL_BUTTON_LMASK ) {
			cam.angles[1] += mousemovex * sensitivity;
			cam.angles[0] += mousemovey * sensitivity;
			clamp (cam.angles);
		} else if( buttonmask & SDL_BUTTON_MMASK ) {
			cam.origin[0] += -(mousemovex * sensitivity);
			cam.origin[1] += mousemovey * sensitivity;
			clamp (cam.angles);
		} else if( buttonmask & SDL_BUTTON_RMASK ) {
			cam.origin[2] += -(mousemovey * sensitivity);
			clamp (cam.angles);
		}

		currenttime = SDL_GetTicks();
		if (playback) {
			if (currenttime < nextframetime)		//interpolate
			{
				frontlerp = (float)((currenttime - prevframetime)/(nextframetime - prevframetime));
				if (frontlerp > 1)
					frontlerp = 1;
				else if (frontlerp < 0)
					frontlerp = 0;
			} else {
				animationname = SMViewer_NextFrame( &frame, &oldframe );
				frametime = (double)( 1000.0/(double)sceneframerate );
				prevframetime = currenttime;
				nextframetime = frametime + currenttime;
				frontlerp = 0.0f;
			}
		}


		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		zNear = 1;
		zFar = 16000;//zFar = 1 + dpm->allradius * 2;//JALFIXME
		xmax = zNear;
		ymax = (float)((double)xmax * (double) height / (double) width);
		glMatrixMode(GL_PROJECTION);CHECKGLERROR
		glLoadIdentity();CHECKGLERROR
		glFrustum(-xmax, xmax, -ymax, ymax, zNear, zFar);CHECKGLERROR
		glMatrixMode(GL_MODELVIEW);CHECKGLERROR
		glLoadIdentity();

		glTranslatef (-cam.origin[0], -cam.origin[1], -cam.origin[2]);
		glRotatef(cam.angles[0], 1.0f, 0.0f, 0.0f);
		glRotatef(cam.angles[2], 0.0f, 1.0f, 0.0f);
		glRotatef(cam.angles[1], 0.0f, 0.0f, 1.0f);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		
		SKM_Draw( showmodel, showskeleton, frame, oldframe, frontlerp );
		if( showaxis ) R_DrawAxis();

		//i = SDL_GetTicks();
		//printf("%dms per frame\n", i - currenttime);

		glMatrixMode(GL_PROJECTION);CHECKGLERROR
		glLoadIdentity();CHECKGLERROR
		glOrtho(0, 640, 480, 0, -100, 100);CHECKGLERROR
		glMatrixMode(GL_MODELVIEW);CHECKGLERROR
		glLoadIdentity();
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		//glDisable(GL_CULL_FACE);
		bindimagetexture("lhfont.tga");
		glColor4f(1,1,1,1);
		fpsframecount++;
		if (currenttime >= fpsbasetime + 1000)
		{
			fps = (int) ((double) fpsframecount * 1000.0 / ((double) currenttime - (double) fpsbasetime));
			fpsbasetime = currenttime;
			fpsframecount = 0;
		}
		sprintf(tempstring, "H: help");
		drawstring(tempstring, 0, 0, 8, 8);
		if( showhelp )
		{
			sprintf(tempstring, "S: toogle skeleton");
			drawstring(tempstring, 0, 16, 8, 8);
			sprintf(tempstring, "M: toogle model");
			drawstring(tempstring, 0, 24, 8, 8);
			sprintf(tempstring, "A: toogle axis");
			drawstring(tempstring, 0, 32, 8, 8);
			sprintf(tempstring, "left/right arrows: prev/next animation");
			drawstring(tempstring, 0, 40, 8, 8);
			sprintf(tempstring, "up/down arrows: prev/next frame (when paused)");
			drawstring(tempstring, 0, 48, 8, 8);
			sprintf(tempstring, "space: pause");
			drawstring(tempstring, 0, 56, 8, 8);
			sprintf(tempstring, "+,-: change animation playrate");
			drawstring(tempstring, 0, 64, 8, 8);
			sprintf(tempstring, "0-9: type in animation playrate");
			drawstring(tempstring, 0, 72, 8, 8);
			sprintf(tempstring, "V: VBO (%s)", vbo_enable ? "enabled " : "disabled");
			drawstring(tempstring, 0, 80, 8, 8);
			sprintf(tempstring, "escape: quit");
			drawstring(tempstring, 0, 88, 8, 8);
		}
		sprintf(tempstring, "fps%5i  playrate %02d  frame:%04i  %s", fps, sceneframerate, frame, animationname);
		drawstring(tempstring, 0, 480 - 8, 8, 8);

		// note: SDL_GL_SwapBuffers does a glFinish for us
		SDL_GL_SwapBuffers();

		//SDL_Delay(1);
	}

	SKM_freemodel();

	// close the screen surface
	SDL_QuitSubSystem (SDL_INIT_VIDEO);

	printf("Quiting SDL.\n");

	/* Shutdown all subsystems */
	SDL_Quit();

	printf("Quiting....\n");
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("usage: skmviewer <filename.dpm>\n");
		return 1;
	}
	viewer_main(argv[1], 640, 480, 16, 0);
	return 0;
}
