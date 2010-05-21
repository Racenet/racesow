
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <direct.h>

#define SKMODEL_VERSION 2.01

#ifndef M_PI
#define M_PI 3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#if _MSC_VER
#pragma warning (disable : 4244)
#endif

#define MAX_NAME		64
#define MAX_FRAMES		65536
#define MAX_TRIS		65536
#define MAX_VERTS		(MAX_TRIS * 3)
#define MAX_BONES		256
#define MAX_SHADERS		256
#define MAX_FILESIZE	16777216
#define MAX_ATTACHMENTS MAX_BONES

// model format type
#define SKMFORMAT_HEADER	"SKM1"
#define SKMFORMAT_VERSION	2		// hierarchical skeletal pose

// model format related flags
#define SKMBONEFLAG_ATTACH 1

#define MAX_PATH		1024

#define POINT_EPSILON		0.1
#define NORMAL_EPSILON		0.01
#define TEXCOORD_EPSILON	0.1

char outputdir_name[MAX_PATH];
char output_name[MAX_PATH];

char header_name[MAX_PATH];
char model_name[MAX_PATH];
char pose_name[MAX_PATH];

char scene_name[MAX_PATH];

char textures_path[MAX_PATH];

char model_name_uppercase[MAX_PATH];
char scene_name_uppercase[MAX_PATH];

FILE *headerfile = NULL;

double modelorigin[3];
double modelorigin[3] = {0.0, 0.0, 0.0}, modelscale = 1.0;
double modelrotate_pitch = 0.0, modelrotate_yaw = 0.0, modelrotate_roll = 0.0;
int cleanbones;

void stringtouppercase(char *in, char *out)
{
	// cleanup name
	while (*in)
	{
		*out = *in++;

		// force lowercase
		if (*out >= 'a' && *out <= 'z')
			*out += 'A' - 'a';
		out++;
	}

	*out++ = 0;
}


void cleancopyname(char *out, char *in, int size)
{
	char *end = out + size - 1;

	// cleanup name
	while (out < end)
	{
		*out = *in++;
		if (!*out)
			break;

		// force lowercase
		if (*out >= 'A' && *out <= 'Z')
			*out += 'a' - 'A';

		// convert backslash to slash
		if (*out == '\\')
			*out = '/';

		out++;
	}

	end++;

	while (out < end)
		*out++ = 0; // pad with nulls
}

void chopextension(char *text)
{
	char *temp;

	if (!*text)
		return;

	temp = text;
	while (*temp)
	{
		if (*temp == '\\')
			*temp = '/';
		temp++;
	}

	temp = text + strlen(text) - 1;
	while (temp >= text)
	{
		if (*temp == '.') // found an extension
		{
			// clear extension
			*temp++ = 0;
			while (*temp)
				*temp++ = 0;
			break;
		}

		if (*temp == '/') // no extension but hit path
			break;

		temp--;
	}
}

void *readfile(char *filename, int *filesize)
{
	FILE *file;
	void *mem;
	unsigned long size;

	if (!filename[0])
	{
		printf ("readfile: tried to open empty filename\n");
		return NULL;
	}

	if (!(file = fopen (filename, "rb")))
		return NULL;

	fseek (file, 0, SEEK_END);
	if (!(size = ftell(file)))
	{
		fclose (file);
		return NULL;
	}

	if (!(mem = malloc (size + 1)))
	{
		fclose (file);
		return NULL;
	}

	((unsigned char *)mem)[size] = 0;	// 0 byte added on the end
	fseek (file, 0, SEEK_SET);
	if (fread (mem, 1, size, file) < size)
	{
		fclose (file);
		free (mem);
		return NULL;
	}

	fclose (file);
	if (filesize)						// can be passed NULL...
		*filesize = size;

	return mem;
}

void writefile(char *filename, void *buffer, int size)
{
	int size1;
	FILE *file;

	file = fopen (filename, "wb");
	if (!file)
	{
		printf ("unable to open file \"%s\" for writing\n", filename);
		return;
	}

	size1 = fwrite (buffer, 1, size, file);
	fclose (file);

	if (size1 < size)
	{
		printf ("unable to write file \"%s\"\n", filename);
		return;
	}
}

char *scriptbytes, *scriptend;
int scriptsize;

unsigned char *tokenpos;

int getline(unsigned char *line)
{
	unsigned char *out = line;

	if (!line)
		return 0;

	while (*tokenpos == '\r' || *tokenpos == '\n')
		tokenpos++;

	if (*tokenpos == 0)
	{
		*out++ = 0;
		return 0;
	}

	while (*tokenpos && *tokenpos != '\r' && *tokenpos != '\n')
		*out++ = *tokenpos++;

	*out++ = 0;

	return out - line;
}


//JAL:HL2SMD[start]
#define MAX_TOKEN_CHARS 1024
char	com_token[MAX_TOKEN_CHARS];
char *COM_ParseExt(char **data_p/*, int nl*/)
{
	int		c;
	int		len;
	char	*data;
	data = *data_p;
	len = 0;
	com_token[0] = 0;
	if (!data)
	{
		*data_p = NULL;
		return "";
	}
	// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return "";
		}
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		data += 2;
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	// skip /* */ comments
	if (c == '/' && data[1] == '*')
	{
		data += 2;
		while (1)
		{
			if (!*data)
				break;
			if (*data != '*' || *(data+1) != '/')
				data++;
			else
			{
				data += 2;
				break;
			}
		}
		goto skipwhite;
	}
	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				if (len == MAX_TOKEN_CHARS)
				{
					//Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
					len = 0;
				}
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}
	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);
	if (len == MAX_TOKEN_CHARS)
	{
		//Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;
	*data_p = data;
	return com_token;
}
//JAL:HL2SMD[end]

typedef struct bonepose_s
{
	double q[4];
	double origin[3];
} bonepose_t;

typedef struct bone_s
{
	char name[MAX_NAME];
	int parent; // parent of this bone
	int flags;
	int users; // used to determine if a bone is used to avoid saving out unnecessary bones
	int defined;
} bone_t;

typedef struct frame_s
{
	int defined;
	char name[MAX_NAME];
	double mins[3], maxs[3], yawradius, allradius; // clipping
	int numbones;
	bonepose_t *bones;
} frame_t;

//JAL:HL2SMD[start]
#define MAX_INFLUENCES 16

typedef struct tripoint_s
{
	int shadernum;
	//int bonenum;
	double texcoord[2];
	//double origin[3];
	//double normal[3];
	int		numinfluences;
	double	influenceorigin[MAX_INFLUENCES][3];
	double	influencenormal[MAX_INFLUENCES][3];
	int		influencebone[MAX_INFLUENCES];
	float	influenceweight[MAX_INFLUENCES];
} tripoint;
//JAL:HL2SMD[end]

typedef struct triangle_s
{
	int shadernum;
	int v[3];
} triangle;

typedef struct attachment_s
{
	char name[MAX_NAME];
	char parentname[MAX_NAME];
	bonepose_t matrix;
} attachment;

int numattachments = 0;
attachment attachments[MAX_ATTACHMENTS];

int numframes = 0;
frame_t frames[MAX_FRAMES];
int numbones = 0;
bone_t bones[MAX_BONES]; // master bone list
int numshaders = 0;
char shaders[MAX_SHADERS][MAX_PATH];
int numtriangles = 0;
triangle triangles[MAX_TRIS];
int numverts = 0;
tripoint vertices[MAX_VERTS];

// these are used while processing things
bonepose_t bonematrix[MAX_BONES];
char *modelfile;
int vertremap[MAX_VERTS];

double quat_normalize(double q[4])
{
	double length;

	length = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	if (length != 0)
	{
		double ilength = 1.0 / sqrt (length);
		q[0] *= ilength;
		q[1] *= ilength;
		q[2] *= ilength;
		q[3] *= ilength;
	}
	return length;
}

void quat_multiply(double q1[4], double q2[4], double out[4])
{
	out[3] = q1[3]*q2[3] - q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2];
	out[0] = q1[3]*q2[0] + q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1];
	out[1] = q1[3]*q2[1] + q1[1]*q2[3] + q1[2]*q2[0] - q1[0]*q2[2];
	out[2] = q1[3]*q2[2] + q1[2]*q2[3] + q1[0]*q2[1] - q1[1]*q2[0];
}

void quat_transform(double q[4], double v[3], double out[3])
{
	double wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	x2 = q[0] + q[0]; y2 = q[1] + q[1]; z2 = q[2] + q[2];
	xx = q[0] * x2; xy = q[0] * y2; xz = q[0] * z2;
	yy = q[1] * y2; yz = q[1] * z2; zz = q[2] * z2;
	wx = q[3] * x2; wy = q[3] * y2; wz = q[3] * z2;

	out[0] = (1.0f - yy - zz) * v[0] + (xy - wz) * v[1] + (xz + wy) * v[2];
	out[1] = (xy + wz) * v[0] + (1.0f - xx - zz) * v[1] + (yz - wx) * v[2];
	out[2] = (xz - wy) * v[0] + (yz + wx) * v[1] + (1.0f - xx - yy) * v[2];
}

void matrix_to_quat( double m[3][3], double q[4] )
{
	double tr, s;

	tr = m[0][0] + m[1][1] + m[2][2];
	if( tr > 0.00001 ) {
		s = (double)sqrt( tr + 1.0f );
		q[3] = (double)(s * 0.5); s = (double)(0.5 / s);
		q[0] = (m[2][1] - m[1][2]) * s;
		q[1] = (m[0][2] - m[2][0]) * s;
		q[2] = (m[1][0] - m[0][1]) * s;
	} else {
		int i, j, k;

		i = 0;
		if (m[1][1] > m[0][0]) i = 1;
		if (m[2][2] > m[i][i]) i = 2;
		j = (i + 1) % 3;
		k = (i + 2) % 3;

		s = (double)sqrt( m[i][i] - (m[j][j] + m[k][k]) + 1.0 );

		q[i] = (double)(s * 0.5); if( s != 0.0 ) s = (double)(0.5 / s);
		q[j] = (m[j][i] + m[i][j]) * s;
		q[k] = (m[k][i] + m[i][k]) * s;
		q[3] = (m[k][j] - m[j][k]) * s;
	}

	quat_normalize( q );
}

void matrix_copy( double m1[3][3], double m2[3][3] )
{
	int i, j;
	for( i = 0; i < 3; i++ )
		for( j = 0; j < 3; j++ )
			m2[i][j] = m1[i][j];
}
void matrix_multiply( double m1[3][3], double m2[3][3], double out[3][3] )
{
	out[0][0] = m1[0][0]*m2[0][0] + m1[0][1]*m2[1][0] + m1[0][2]*m2[2][0];
	out[0][1] = m1[0][0]*m2[0][1] + m1[0][1]*m2[1][1] + m1[0][2]*m2[2][1];
	out[0][2] = m1[0][0]*m2[0][2] + m1[0][1]*m2[1][2] + m1[0][2]*m2[2][2];
	out[1][0] = m1[1][0]*m2[0][0] + m1[1][1]*m2[1][0] +	m1[1][2]*m2[2][0];
	out[1][1] = m1[1][0]*m2[0][1] + m1[1][1]*m2[1][1] + m1[1][2]*m2[2][1];
	out[1][2] = m1[1][0]*m2[0][2] + m1[1][1]*m2[1][2] +	m1[1][2]*m2[2][2];
	out[2][0] = m1[2][0]*m2[0][0] + m1[2][1]*m2[1][0] +	m1[2][2]*m2[2][0];
	out[2][1] = m1[2][0]*m2[0][1] + m1[2][1]*m2[1][1] +	m1[2][2]*m2[2][1];
	out[2][2] = m1[2][0]*m2[0][2] + m1[2][1]*m2[1][2] +	m1[2][2]*m2[2][2];
}

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F
void matrix_rotate( double m[3][3], double angle, double x, double y, double z )
{
	double t[3][3], b[3][3];
	double c = cos( DEG2RAD(angle) );
	double s = sin( DEG2RAD(angle) );
	double mc = 1 - c, t1, t2;
	
	t[0][0] = (x * x * mc) + c;
	t[1][1] = (y * y * mc) + c;
	t[2][2] = (z * z * mc) + c;

	t1 = y * x * mc;
	t2 = z * s;
	t[0][1] = t1 + t2;
	t[1][0] = t1 - t2;

	t1 = x * z * mc;
	t2 = y * s;
	t[0][2] = t1 - t2;
	t[2][0] = t1 + t2;

	t1 = y * z * mc;
	t2 = x * s;
	t[1][2] = t1 + t2;
	t[2][1] = t1 - t2;

	matrix_copy( m, b );
	matrix_multiply( b, t, m );
}

double wrapangles(double f)
{
	while (f < M_PI)
		f += M_PI * 2;
	while (f >= M_PI)
		f -= M_PI * 2;

	return f;
}

bonepose_t computebonematrix(double x, double y, double z, double a, double b, double c)
{
	bonepose_t out;
	double sr, sp, sy, cr, cp, cy;
	double m[3][3], tr, s;

	sy = sin (c);
	cy = cos (c);
	sp = sin (b);
	cp = cos (b);
	sr = sin (a);
	cr = cos (a);

	m[0][0] = cp*cy;
	m[1][0] = cp*sy;
	m[2][0] = -sp;
	m[0][1] = sr*sp*cy+cr*-sy;
	m[1][1] = sr*sp*sy+cr*cy;
	m[2][1] = sr*cp;
	m[0][2] = (cr*sp*cy+-sr*-sy);
	m[1][2] = (cr*sp*sy+-sr*cy);
	m[2][2] = cr*cp;

	tr = m[0][0] + m[1][1] + m[2][2];
	if( tr >  0.00000001 ) {
		s = sqrt( tr + 1.0 );
		out.q[3] = s * 0.5; s = 0.5 / s;
		out.q[0] = (m[2][1] - m[1][2]) * s;
		out.q[1] = (m[0][2] - m[2][0]) * s;
		out.q[2] = (m[1][0] - m[0][1]) * s;
	} else {
		int i, j, k;

		i = 0;
		if (m[1][1] > m[0][0]) i = 1;
		if (m[2][2] > m[i][i]) i = 2;
		j = (i + 1) % 3;
		k = (i + 2) % 3;

		s = sqrt( m[i][i] - (m[j][j] + m[k][k]) + 1.0 );

		out.q[i] = s * 0.5; if( s != 0.0 ) s = 0.5 / s;
		out.q[j] = (m[j][i] + m[i][j]) * s;
		out.q[k] = (m[k][i] + m[i][k]) * s;
		out.q[3] = (m[k][j] - m[j][k]) * s;
	}

	quat_normalize( out.q );

	out.origin[0] = x;
	out.origin[1] = y;
	out.origin[2] = z;

	return out;
}

bonepose_t concattransform(bonepose_t in1, bonepose_t in2)
{
	bonepose_t out;

	quat_multiply( in1.q, in2.q, out.q );
	quat_transform( in1.q, in2.origin, out.origin );
	out.origin[0] += in1.origin[0]; out.origin[1] += in1.origin[1]; out.origin[2] += in1.origin[2];

	return out;
}

void transform(double in[3], bonepose_t b, double out[3])
{
	quat_transform( b.q, in, out );
	out[0] += b.origin[0]; out[1] += b.origin[1]; out[2] += b.origin[2];
}

void inversetransform(double in[3], bonepose_t b, double out[3])
{
	double temp[3];
	double btemp[4];

	temp[0] = in[0] - b.origin[0]; temp[1] = in[1] - b.origin[1]; temp[2] = in[2] - b.origin[2];
	btemp[0] = -b.q[0]; btemp[1] = -b.q[1]; btemp[2] = -b.q[2]; btemp[3] = b.q[3];
	quat_transform( btemp, temp, out );
}

void inverserotate(double in[3], bonepose_t b, double out[3])
{
	double btemp[4];

	btemp[0] = -b.q[0]; btemp[1] = -b.q[1]; btemp[2] = -b.q[2]; btemp[3] = b.q[3];
	quat_transform( btemp, in, out );
}

int parsenodes(void)
{
	int num, parent;
	unsigned char line[1024], name[1024];
	char *tok, *string;//JAL:HL2SMD

	memset(bones, 0, sizeof(bones));
	numbones = 0;

	while (getline(line))
	{
		if (!strcmp(line, "end"))
			break;
//JAL:HL2SMD[start]
		//parse this line read by tokens
		string = line;

		//get bone number
		tok = COM_ParseExt(&string);
		if( !tok ) {
			printf("error in nodes, expecting bone number in line:%s\n", line);
			return 0;
		}
		num = atoi( tok );

		//get bone name
		tok = COM_ParseExt(&string);
		if( !tok ) {
			printf("error in nodes, expecting bone name in line:%s\n", line);
			return 0;
		}
		cleancopyname(name, tok, MAX_NAME);//printf( "bone name: %s\n", name );

		//get parent number
		tok = COM_ParseExt(&string);
		if( !tok ) {
			printf("error in nodes, expecting parent number in line:%s\n", line);
			return 0;
		}
		parent = atoi( tok );
//JAL:HL2SMD[end]
		if (num < 0 || num >= MAX_BONES)
		{
			printf("invalid bone number %i\n", num);
			return 0;
		}
		if (parent >= num)
		{
			printf("bone's parent >= bone's number\n");
			return 0;
		}
		if (parent < -1)
		{
			printf("bone's parent < -1\n");
			return 0;
		}
		if (parent >= 0 && !bones[parent].defined)
		{
			printf("bone's parent bone has not been defined\n");
			return 0;
		}
		memcpy(bones[num].name, name, MAX_NAME);
		bones[num].defined = 1;
		bones[num].parent = parent;
		if (num >= numbones)
			numbones = num + 1;
	}
	return 1;
}

int parseskeleton(void)
{
	unsigned char line[1024], temp[1024];
	int i, frame, num;
	double x, y, z, a, b, c;
	int baseframe;
	char *tok, *string;//JAL:HL2SMD

	baseframe = numframes;
	frame = baseframe;

	while (getline (line))
	{
//JAL:HL2SMD[start]
		if (!strcmp(line, "end"))
			break;

		//parse this line read by tokens
		string = line;

		//get opening line token
		tok = COM_ParseExt(&string);
		if (!tok) {
			printf("error in parseskeleton, script line:%s\n", line);
			return 0;
		}

		if (!strcmp(tok, "time"))
		{
			//get the time value
			tok = COM_ParseExt(&string);
			if (!tok) {
				printf("error in parseskeleton, expecting time value in line:%s\n", line);
				return 0;
			}
			i = atoi( tok );
			if (i < 0)
			{
				printf("invalid time %i\n", i);
				return 0;
			}

			frame = baseframe + i;
			if (frame >= MAX_FRAMES)
			{
				printf("only %i frames supported currently\n", MAX_FRAMES);
				return 0;
			}
			if (frames[frame].defined)
			{
				printf("warning: duplicate frame\n");
				free(frames[frame].bones);
			}
			sprintf(temp, "%s_%i", scene_name, i);
			if (strlen(temp) > 31)
			{
				printf("error: frame name \"%s\" is longer than 31 characters\n", temp);
				return 0;
			}
			cleancopyname(frames[frame].name, temp, MAX_NAME);

			frames[frame].numbones = numbones + numattachments + 1;
			frames[frame].bones = malloc(frames[frame].numbones * sizeof(bonepose_t));
			memset(frames[frame].bones, 0, frames[frame].numbones * sizeof(bonepose_t));
			frames[frame].bones[frames[frame].numbones - 1].q[3] = 1;
			frames[frame].defined = 1;
			if (numframes < frame + 1)
				numframes = frame + 1;
		}
		else
		{
			//the token was bone number
			num = atoi( tok );

			if (num < 0 || num >= numbones)
			{
				printf("error: invalid bone number: %i\n", num);
				return 0;
			}
			if (!bones[num].defined)
			{
				printf("error: bone %i not defined\n", num);
				return 0;
			}

			//get x, y, z tokens
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parseskeleton, expecting 'x' value in line:%s\n", line);
				return 0;
			}
			x = atof( tok );

			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parseskeleton, expecting 'y' value in line:%s\n", line);
				return 0;
			}
			y = atof( tok );

			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parseskeleton, expecting 'z' value in line:%s\n", line);
				return 0;
			}
			z = atof( tok );

			//get a, b, c tokens
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parseskeleton, expecting 'a' value in line:%s\n", line);
				return 0;
			}
			a = atof( tok );

			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parseskeleton, expecting 'b' value in line:%s\n", line);
				return 0;
			}
			b = atof( tok );

			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parseskeleton, expecting 'c' value in line:%s\n", line);
				return 0;
			}
			c = atof( tok );

//			if (num < 0 || num >= numbones)
//			{
//				printf("error: invalid bone number: %i\n", num);
//				return 0;
//			}
//			if (!bones[num].defined)
//			{
//				printf("error: bone %i not defined\n", num);
//				return 0;
//			}

			// root bones need to be offset
			if (bones[num].parent < 0)
			{
				// Vic: added '* modelscale' (thanks to Riot)
				x = (x - modelorigin[0]) * modelscale;
				y = (y - modelorigin[1]) * modelscale;
				z = (z - modelorigin[2]) * modelscale;
			}
			// LordHavoc: compute matrix
			frames[frame].bones[num] = computebonematrix (x, y, z, a, b, c);
		}
//JAL:HL2SMD[end]
	}

	for (frame = 0; frame < numframes; frame++)
	{
		if (!frames[frame].defined)
		{
			if (frame < 1)
			{
				printf("error: no first frame\n");
				return 0;
			}
			if (!frames[frame - 1].defined)
			{
				printf("error: no previous frame to duplicate\n");
				return 0;
			}
			sprintf(temp, "%s_%i", scene_name, frame - baseframe);
			if (strlen(temp) > 31)
			{
				printf("error: frame name \"%s\" is longer than 31 characters\n", temp);
				return 0;
			}
			printf("frame %s missing, duplicating previous frame %s with new name %s\n", temp, frames[frame - 1].name, temp);
			frames[frame].defined = 1;
			cleancopyname(frames[frame].name, temp, MAX_NAME);
			frames[frame].numbones = numbones + numattachments + 1;
			frames[frame].bones = malloc(frames[frame].numbones * sizeof(bonepose_t));
			memcpy(frames[frame].bones, frames[frame - 1].bones, frames[frame].numbones * sizeof(bonepose_t));
			frames[frame].bones[frames[frame].numbones - 1].q[3] = 1;
			printf("duplicate frame named %s\n", frames[frame].name);
		}
		if (frame >= baseframe && headerfile)
			fprintf(headerfile, "#define MODEL_%s_%s_%i %i\n", model_name_uppercase, scene_name_uppercase, frame - baseframe, frame);
	}
	if (headerfile)
	{
		fprintf(headerfile, "#define MODEL_%s_%s_START %i\n", model_name_uppercase, scene_name_uppercase, baseframe);
		fprintf(headerfile, "#define MODEL_%s_%s_END %i\n", model_name_uppercase, scene_name_uppercase, numframes);
		fprintf(headerfile, "#define MODEL_%s_%s_LENGTH %i\n", model_name_uppercase, scene_name_uppercase, numframes - baseframe);
		fprintf(headerfile, "\n");
	}
	return 1;
}

int freeframes(void)
{
	int i;

	for (i = 0; i < numframes; i++)
	{
		if (frames[i].defined && frames[i].bones)
			free(frames[i].bones);
	}

	numframes = 0;
	return 1;
}

int initframes(void)
{
	memset(frames, 0, sizeof(frames));
	return 1;
}

//JAL:HL2SMD[start]
int parsetriangles(void)
{
	unsigned char line[1024], cleanline[MAX_PATH];
	int current = 0, i;
	double org[3], normal[3];
	double d;
	int vbonenum;
	double	vtexcoord[2];
	
	int		numinfluences;
	int		temp_numbone[MAX_INFLUENCES];
	float	temp_influence[MAX_INFLUENCES];
	double	temp_origin[MAX_INFLUENCES][3];
	double	temp_normal[MAX_INFLUENCES][3];
	int	j;
	char	*tok, *string;

	numtriangles = 0;
	numshaders = 0;

	for (i = 0; i < numbones; i++)
	{
		if (!bones[i].defined)
			continue;
		if (bones[i].parent >= 0)
			bonematrix[i] = concattransform(bonematrix[bones[i].parent], frames[0].bones[i]);
		else
			bonematrix[i] = frames[0].bones[i];
	}
	while (getline(line))
	{
		if (!strcmp(line, "end"))
			break;

		if (current == 0)
		{
			char pathandshadername[MAX_NAME];

			// Vic: make sure shader name is valid (thanks to defcon-X)
			//cleancopyname(cleanline, line, MAX_NAME);

			pathandshadername[0] = 0;
			if( textures_path[0] )
				strcat(pathandshadername, textures_path);
			if( line[0] )
				strcat(pathandshadername, line);

			cleancopyname(cleanline, pathandshadername, MAX_PATH);

			for (i = 0; i < numshaders; i++)
				if (!strcmp (shaders[i], cleanline))
					break;

			triangles[numtriangles].shadernum = i;

			if (i == numshaders)
			{
				if( i == MAX_SHADERS ) {
					printf("MAX_SHADERS reached\n");
					return 0;
				}
				strcpy(shaders[i], cleanline);
				numshaders++;
			}

			current++;
		}
		else
		{
			//parse this line read by tokens
			string = line;
			org[0] = 0;org[1] = 0;org[2] = 0;
			normal[0] = 0;normal[1] = 0;normal[2] = 0;
			vtexcoord[0] = 0;vtexcoord[1] = 0;
			
			//get bonenum token
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parsetriangles, expecting 'bonenum', script line:%s\n", line);
				return 0;
			}
			vbonenum = atoi( tok );

			//get org[0] token
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parsetriangles, expecting 'org[0]', script line:%s\n", line);
				return 0;
			}
			org[0] = atof( tok );

			//get org[1] token
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parsetriangles, expecting 'org[1]', script line:%s\n", line);
				return 0;
			}
			org[1] = atof( tok );

			//get org[2] token
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parsetriangles, expecting 'org[2]', script line:%s\n", line);
				return 0;
			}
			org[2] = atof( tok );

			//get normal[0] token
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parsetriangles, expecting 'normal[0]', script line:%s\n", line);
				return 0;
			}
			normal[0] = atof( tok );

			//get normal[1] token
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parsetriangles, expecting 'normal[1]', script line:%s\n", line);
				return 0;
			}
			normal[1] = atof( tok );

			//get normal[2] token
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parsetriangles, expecting 'normal[2]', script line:%s\n", line);
				return 0;
			}
			normal[2] = atof( tok );

			//get vtexcoord[0] token
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parsetriangles, expecting 'vtexcoord[0]', script line:%s\n", line);
				return 0;
			}
			vtexcoord[0] = atof( tok );

			//get vtexcoord[1] token
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') {
				printf("error in parsetriangles, expecting 'vtexcoord[1]', script line:%s\n", line);
				return 0;
			}
			vtexcoord[1] = atof( tok );

			// are there more words (HalfLife2) or not (HalfLife1)?
			tok = COM_ParseExt(&string);
			if (!tok || tok[0] <= ' ') { 
				// one influence (HalfLife1)
				numinfluences = 1;
				temp_numbone[0] = vbonenum;
				temp_influence[0] = 1.0f;
			} 
			else 
			{ 
				// multiple influences found (HalfLife2)
				int c;

				numinfluences = atoi( tok );
				if( !numinfluences ) 
				{
					printf("error in parsetriangles, expecting 'numinfluences', script line:%s\n", line);
					return 0;
				}

				//read by pairs, bone number and influence
				for( c = 0; c < numinfluences; c++ )
				{
					//get bone number
					tok = COM_ParseExt(&string);//printf("tok: \"%s\"\n", tok);
					if( !tok || tok[0] <= ' ' ) {
						printf("invalid vertex influence \"%s\"\n", line);
						return 0;
					}
					temp_numbone[c] = atoi(tok);
					if(temp_numbone[c] < 0 || temp_numbone[c] >= numbones )
					{
						printf("invalid vertex influence (invalid bone number) \"%s\"\n", line);
						return 0;
					}
					//get influence weight
					tok = COM_ParseExt(&string);//printf("tok: \"%s\"\n", tok);
					if( !tok || tok[0] <= ' ' ) {
						printf("invalid vertex influence \"%s\"\n", line);
						return 0;
					}
					temp_influence[c] = atof(tok);
					if( temp_influence[c] < 0.0f ) 
					{
						printf("invalid vertex influence weight, ignored \"%s\"\n", line);
						return 0;
					} 
					else if( temp_influence[c] > 1.0f )
						temp_influence[c] = 1.0f;
				}
			}

			//validate linked bones
			if( numinfluences < 1 )
			{
				printf("vertex with no influence found in triangle data\n");
				return 0;
			}
			for( i=0; i<numinfluences; i++ ) 
			{
				if (temp_numbone[i] < 0 || temp_numbone[i] >= MAX_BONES )
				{
					printf("invalid bone number %i in triangle data\n", temp_numbone[i]);
					return 0;
				}
				if (!bones[temp_numbone[i]].defined)
				{
					printf("bone %i in triangle data is not defined\n", temp_numbone[i]);
					return 0;
				}
			}

			// apply model scaling and offset
			org[0] = (org[0] - modelorigin[0]) * modelscale;
			org[1] = (org[1] - modelorigin[1]) * modelscale;
			org[2] = (org[2] - modelorigin[2]) * modelscale;
			
			// untransform the origin and normal
			for( j=0; j<numinfluences; j++ ) 
			{
				inversetransform(org, bonematrix[temp_numbone[j]], temp_origin[j]);
				inverserotate(normal, bonematrix[temp_numbone[j]], temp_normal[j]);
				
				// normalize
				d = sqrt( temp_normal[j][0] * temp_normal[j][0] + temp_normal[j][1] * temp_normal[j][1] + temp_normal[j][2] * temp_normal[j][2] );
				
				// Vic: check if normal is valid
				if (d)
				{
					d = 1.0 / d;
					temp_normal[j][0] *= d;
					temp_normal[j][1] *= d;
					temp_normal[j][2] *= d;
					
					// round off minor errors in the normal
					if (fabs(temp_normal[j][0]) < 0.001)
						temp_normal[j][0] = 0;
					if (fabs(temp_normal[j][1]) < 0.001)
						temp_normal[j][1] = 0;
					if (fabs(temp_normal[j][2]) < 0.001)
						temp_normal[j][2] = 0;
					
					// Vic: normalize again
					d = 1 / sqrt(temp_normal[j][0] * temp_normal[j][0] + temp_normal[j][1] * temp_normal[j][1] + temp_normal[j][2] * temp_normal[j][2]);
					temp_normal[j][0] *= d;
					temp_normal[j][1] *= d;
					temp_normal[j][2] *= d;
				}
				else
				{
					printf("invalid normal in bone %i\n", temp_numbone[j]);
				}
			}

			// add vertex to list if unique (jalfixme?, it's only checking first influence)
			for (i = 0; i < numverts; i++)
				if (vertices[i].shadernum == triangles[numtriangles].shadernum
				 && vertices[i].numinfluences == numinfluences
				 && vertices[i].influencebone[0] == temp_numbone[0]
				 && fabs(vertices[i].influenceorigin[0][0] - temp_origin[0][0]) < POINT_EPSILON
				 && fabs(vertices[i].influenceorigin[0][1] - temp_origin[0][1]) < POINT_EPSILON
				 && fabs(vertices[i].influenceorigin[0][2] - temp_origin[0][2]) < POINT_EPSILON
				 && fabs(vertices[i].influencenormal[0][0] - temp_normal[0][0]) < NORMAL_EPSILON
				 && fabs(vertices[i].influencenormal[0][1] - temp_normal[0][1]) < NORMAL_EPSILON
				 && fabs(vertices[i].influencenormal[0][2] - temp_normal[0][2]) < NORMAL_EPSILON
				 && vertices[i].texcoord[0] == vtexcoord[0] 
				 && vertices[i].texcoord[1] == vtexcoord[1])	//JAL: uvcoords bugfix
				 //&& fabs(vertices[i].texcoord[0] - vtexcoord[0]) < TEXCOORD_EPSILON 
				 //&& fabs(vertices[i].texcoord[1] - vtexcoord[1]) < TEXCOORD_EPSILON)
					break;

			triangles[numtriangles].v[current - 1] = i;

			if (i >= numverts)
			{
				numverts++;
				vertices[i].shadernum = triangles[numtriangles].shadernum;
				vertices[i].texcoord[0] = vtexcoord[0];
				vertices[i].texcoord[1] = vtexcoord[1];
				vertices[i].numinfluences = numinfluences;

				for( j=0; j < vertices[i].numinfluences; j++ ) 
				{
					vertices[i].influenceorigin[j][0] = temp_origin[j][0];
					vertices[i].influenceorigin[j][1] = temp_origin[j][1];
					vertices[i].influenceorigin[j][2] = temp_origin[j][2];
					vertices[i].influencenormal[j][0] = temp_normal[j][0];
					vertices[i].influencenormal[j][1] = temp_normal[j][1];
					vertices[i].influencenormal[j][2] = temp_normal[j][2];
					vertices[i].influencebone[j] = temp_numbone[j];
					vertices[i].influenceweight[j] = temp_influence[j];
				}
				
			}

			current++;
			if (current >= 4)
			{
				current = 0;
				numtriangles++;
			}
		}
	}

	return 1;
}
//JAL:HL2SMD[end]

int parsemodelfile(void)
{
	int i;
	char line[1024], command[256];

	tokenpos = modelfile;
	while (getline(line))
	{
		sscanf(line, "%s %i", command, &i);

		if (!strcmp(command, "version"))
		{
			if (i != 1)
			{
				printf("file is version %d, only version 1 is supported\n", i);
				return 0;
			}
		}
		else if (!strcmp(command, "nodes"))
		{
			if (!parsenodes())
				return 0;
		}
		else if (!strcmp(command, "skeleton"))
		{
			if (!parseskeleton())
				return 0;
		}
		else if (!strcmp(command, "triangles"))
		{
			if (!parsetriangles())
				return 0;
		}
		else
		{
			printf ("unknown command \"%s\"\n", line);
			return 0;
		}
	}

	return 1;
}

int addattachments(void)
{
	int i, j;

	for (i = 0; i < numattachments; i++)
	{
		bones[numbones].defined = 1;
		bones[numbones].parent = -1;
		bones[numbones].flags = SKMBONEFLAG_ATTACH;

		for (j = 0; j < numbones; j++)
			if (!strcmp (bones[j].name, attachments[i].parentname))
				bones[numbones].parent = j;

		if (bones[numbones].parent < 0)
			printf ("warning: unable to find bone \"%s\" for attachment \"%s\", using root instead\n", attachments[i].parentname, attachments[i].name);

		cleancopyname(bones[numbones].name, attachments[i].name, MAX_NAME);

		// we have to duplicate the attachment in every frame
		for (j = 0; j < numframes; j++)
			frames[j].bones[numbones] = attachments[i].matrix;

		numbones++;
	}

	numattachments = 0;

	return 1;
}

int cleanupbones(void)
{
	int i, j;
	int oldnumbones;
	int remap[MAX_BONES];

	// figure out which bones are used
	for (i = 0; i < numbones; i++)
	{
		if (!bones[i].defined)
			continue;

		bones[i].users = 0;
		if (bones[i].flags & SKMBONEFLAG_ATTACH)
			bones[i].users++;
	}
//JAL:HL2SMD[start]
	for (i = 0; i < numverts; i++) {
		for( j = 0; j < vertices[i].numinfluences; j++ )
			bones[vertices[i].influencebone[j]].users++;
	}
//JAL:HL2SMD[end]
	for (i = 0; i < numbones; i++)
		if (bones[i].defined && bones[i].users && bones[i].parent >= 0)
			bones[bones[i].parent].users++;

	// now calculate the remapping table for whichever ones should remain
	oldnumbones = numbones;
	numbones = 0;
	for (i = 0; i < oldnumbones; i++)
	{
		if (bones[i].defined && bones[i].users)
			remap[i] = numbones++;
		else
		{
			remap[i] = -1;
		}
	}

	// shuffle bone data around to eliminate gaps
	for (i = 0; i < oldnumbones; i++)
		if (bones[i].parent >= 0)
			bones[i].parent = remap[bones[i].parent];

	for (i = 0; i < oldnumbones; i++)
		if (remap[i] >= 0 && remap[i] != i)
			bones[remap[i]] = bones[i];

	for (i = 0; i < numframes; i++)
	{
		if (!frames[i].defined)
			continue;

		for (j = 0; j < oldnumbones; j++)
		{
			if (remap[j] >= 0 && remap[j] != j)
				frames[i].bones[remap[j]] = frames[i].bones[j];
		}
	}
//JAL:HL2SMD[start]
	// remap vertex references
	for (i = 0; i < numverts; i++) {
		for( j=0; j<vertices[i].numinfluences; j++ )
			vertices[i].influencebone[j] = remap[vertices[i].influencebone[j]];
	}
//JAL:HL2SMD[end]
	return 1;
}

int cleanupshadernames(void)
{
	int i;

	for (i = 0; i < numshaders; i++)
		chopextension(shaders[i]);

	return 1;
}

void fixrootbones(void)
{
	int i, j;
	bonepose_t rootposequat;
	double rootpose[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
	double angles[3];

	angles[0] = modelrotate_pitch;
	angles[1] = modelrotate_yaw;
	angles[2] = modelrotate_roll;
	matrix_rotate( rootpose, angles[0], 0.0f, 1.0f, 0.0f );
	matrix_rotate( rootpose, angles[1], 0.0f, 0.0f, 1.0f );
	matrix_rotate( rootpose, angles[2], 1.0f, 0.0f, 0.0f );

	rootposequat.origin[0] = rootposequat.origin[1] = rootposequat.origin[2] = 0.0;
	matrix_to_quat( rootpose, rootposequat.q );
	
	for (j = 0;j < numbones;j++)
	{
		if (bones[j].parent < 0)
		{
			// a root bone
			for (i = 0;i < numframes;i++)
				frames[i].bones[j] = concattransform(rootposequat, frames[i].bones[j]);
		}
	}
}

char *token;

void inittokens(char *script)
{
	token = script;
}

char tokenbuffer[1024];

char *gettoken(void)
{
	char *out;

	if (!token)			// Vic (thanks to defcon-X)
		return NULL;

	out = tokenbuffer;
	while (*token && *token <= ' ' && *token != '\n')
		token++;

	if (!*token)
		return NULL;

	switch (*token)
	{
		case '\"':
			token++;
			while (*token && *token != '\r' && *token != '\n' && *token != '\"')
				*out++ = *token++;
			*out++ = 0;

			if (*token == '\"')
				token++;
			else
				printf("warning: unterminated quoted string\n");

			return tokenbuffer;

		case '(':
		case ')':
		case '{':
		case '}':
		case '[':
		case ']':
		case '\n':
			tokenbuffer[0] = *token++;
			tokenbuffer[1] = 0;
			return tokenbuffer;

		default:
			while (*token && *token > ' ' && *token != '(' && *token != ')' && *token != '{' && *token != '}' && *token != '[' && *token != ']' && *token != '\"')
				*out++ = *token++;

			*out++ = 0;
			return tokenbuffer;
	}
}

typedef struct sccommand_s
{
	char *name;
	int (*code)(void);
} sccommand;

int isdouble(char *c)
{
	if (!c)		// Vic
		return 0;

	while (*c)
	{
		switch (*c)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
			case 'e':
			case 'E':
			case '-':
			case '+':
				break;
			default:
				return 0;
		}

		c++;
	}

	return 1;
}

int isfilename(char *c)
{
	if (!c)		// Vic
		return 0;

	while (*c)
	{
		if (*c < ' ')
			return 0;
		c++;
	}

	return 1;
}

int sc_attachment(void)
{
	int i;
	char *c;
	double origin[3], angles[3];

	if (numattachments >= MAX_ATTACHMENTS)
	{
		printf ("ran out of attachment slots\n");
		return 0;
	}

	c = gettoken();
	if (!isfilename(c))
		return 0;

	cleancopyname(attachments[numattachments].name, c, MAX_NAME);

	c = gettoken();
	if (!isfilename(c))
		return 0;

	cleancopyname(attachments[numattachments].parentname, c, MAX_NAME);

	for (i = 0; i < 6; i++)
	{
		c = gettoken();
		if (!isdouble(c))
			return 0;

		if (i < 3)
			origin[i] = atof(c);
		else
			angles[i - 3] = atof(c) * (M_PI / 180.0);
	}

	attachments[numattachments].matrix = computebonematrix(origin[0], origin[1], origin[2], angles[0], angles[1], angles[2]);

	numattachments++;
	return 1;
}

int sc_outputdir(void)
{
	char *c = gettoken();

	if (!isfilename(c))
		return 0;

	strcpy(outputdir_name, c);
	chopextension(outputdir_name);

	if (strlen(outputdir_name) && outputdir_name[strlen(outputdir_name) - 1] != '/')
		strcat(outputdir_name, "/");

	_mkdir( outputdir_name );

	return 1;
}

int sc_texturepath(void)
{
	char *c = gettoken();

	if (!isfilename(c))
		return 0;

	strcpy(textures_path, c);
	chopextension(textures_path);

	if (strlen(textures_path) && textures_path[strlen(textures_path) - 1] != '/')
		strcat(textures_path, "/");

	return 1;
}

int sc_model(void)
{
	char *c = gettoken();

	if (!isfilename(c))
		return 0;

	strcpy(model_name, c);
	chopextension(model_name);
	stringtouppercase(model_name, model_name_uppercase);

	sprintf(header_name, "%s%s.h", outputdir_name, model_name);
	sprintf(output_name, "%s%s.skm", outputdir_name, model_name);

	return 1;
}

int sc_export(void)
{
	char *c = gettoken();
	char temp[MAX_PATH];

	if (!isfilename(c))
		return 0;

	strcpy(temp, c);
	chopextension(temp);

	sprintf(pose_name, "%s%s.skp", outputdir_name, temp);

	return 1;
}

int sc_origin(void)
{
	int i;
	char *c;
	for( i = 0; i < 3; i++ )
	{
		c = gettoken();
		if (!c)
			return 0;
		if (!isdouble(c))
			return 0;
		modelorigin[i] = atof(c);
	}
	return 1;
}

int sc_rotate_pitch(void)
{
	char *c = gettoken();
	if (!c)
		return 0;
	if (!isdouble(c))
		return 0;
	modelrotate_pitch = atof(c);
	return 1;
}

int sc_rotate_yaw(void)
{
	char *c = gettoken();
	if (!c)
		return 0;
	if (!isdouble(c))
		return 0;
	modelrotate_yaw = atof(c);
	return 1;
}

int sc_rotate_roll(void)
{
	char *c = gettoken();
	if (!c)
		return 0;
	if (!isdouble(c))
		return 0;
	modelrotate_roll = atof(c);
	return 1;
}

int sc_scale(void)
{
	char *c = gettoken();
	if (!c)
		return 0;
	if (!isdouble(c))
		return 0;
	modelscale = atof (c);
	return 1;
}

int sc_cleanbones(void)
{
	cleanbones = 1;
	return 1;
}

int sc_scene(void)
{
	char *c = gettoken();
	if (!c)
		return 0;
	if (!isfilename(c))
		return 0;

	modelfile = readfile(c, NULL);
	if (!modelfile)
		return 0;

	cleancopyname(scene_name, c, MAX_NAME);
	chopextension(scene_name);
	stringtouppercase(scene_name, scene_name_uppercase);
	printf("parsing scene %s\n", scene_name);

	if (!headerfile)
	{
		headerfile = fopen(header_name, "wb");

		if (headerfile)
		{
			fprintf(headerfile, "/*\n");
			fprintf(headerfile, "Generated header file for %s\n", model_name);
			fprintf(headerfile, "This file contains frame number definitions for use in code referencing the model, to make code more readable and maintainable.\n");
			fprintf(headerfile, "*/\n");
			fprintf(headerfile, "\n");
			fprintf(headerfile, "#ifndef MODEL_%s_H\n", model_name_uppercase);
			fprintf(headerfile, "#define MODEL_%s_H\n", model_name_uppercase);
			fprintf(headerfile, "\n");
		}
	}

	if (!parsemodelfile())
		return 0;

	free(modelfile);
	return 1;
}

int sc_comment(void)
{
	while (gettoken()[0] != '\n');
	return 1;
}

int sc_nothing(void)
{
	return 1;
}

sccommand sc_commands[] =
{
	{"attachment", sc_attachment},
	{"outputdir", sc_outputdir},
	{"texturepath", sc_texturepath},
	{"model", sc_model},
	{"origin", sc_origin},
	{"rotatepitch", sc_rotate_pitch},
	{"rotateyaw", sc_rotate_yaw},
	{"rotateroll", sc_rotate_roll},
	{"scale", sc_scale},
	{"scene", sc_scene},
	{"export", sc_export},
	{"cleanbones", sc_cleanbones},
	{"\n", sc_nothing},
	{"", NULL}
};

int processcommand(char *command)
{
	int r;
	sccommand *c;
	c = sc_commands;

	// Vic: allow this style of comments
	// #commment
	if (command[0] == '#')
	{
		sc_comment();
		return 1;
	}

	while (c->name[0])
	{
		if (!strcmp(c->name, command))
		{
			printf("executing command %s\n", command);
			r = c->code();
			if (!r)
				printf("error processing script\n");
			return r;
		}

		c++;
	}

	printf("command %s not recognized\n", command);
	return 0;
}

int convertmodel(void);

void processscript(void)
{
	char *c;

	inittokens(scriptbytes);
	numframes = 0;
	numbones = 0;
	numshaders = 0;
	numtriangles = 0;
	cleanbones = 0;
	textures_path[0] = 0;

	initframes();

	while ((c = gettoken()))
		if (c[0] > ' ')
			if (!processcommand(c))
				return;

	if (headerfile)
	{
		fprintf (headerfile, "#endif /*MODEL_%s_H*/\n", model_name_uppercase);
		fclose (headerfile);
	}

	if (!addattachments())
	{
		freeframes();
		return;
	}

	if( cleanbones ) {
		if (!cleanupbones())
		{
			freeframes();
			return;
		}
	}

	if (!cleanupshadernames())
	{
		freeframes();
		return;
	}

	fixrootbones();
	if (!convertmodel())
	{
		freeframes();
		return;
	}

	freeframes();
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf ("usage: %s scriptname.txt\n", argv[0]);
		return 0;
	}

	printf ("SKMODEL Converter version: %.2f\n", SKMODEL_VERSION );

	scriptbytes = readfile (argv[1], &scriptsize);
	if (!scriptbytes)
	{
		printf ("unable to read script file\n");
		return 0;
	}

	scriptend = scriptbytes + scriptsize;
	processscript();

#if (_MSC_VER && _DEBUG)
	printf ("press ENTER\n");
	getchar ();
#endif

	return 0;
}

unsigned char *output;
unsigned char outputbuffer[MAX_FILESIZE];

void putstring(char *in, int length)
{
	while (*in && length)
	{
		*output++ = *in++;
		length--;
	}

	// pad with nulls
	while (length--)
		*output++ = 0;
}

void putnulls(int num)
{
	while (num--)
		*output++ = 0;
}

void putlong(int num)
{
	*output++ = ((num >>  0) & 0xFF);
	*output++ = ((num >>  8) & 0xFF);
	*output++ = ((num >> 16) & 0xFF);
	*output++ = ((num >> 24) & 0xFF);
}

void putfloat(double num)
{
	union
	{
		float f;
		int i;
	}
	n;

	n.f = num;

	// this matches for both positive and negative 0, thus setting it to positive 0
	if (n.f == 0)
		n.f = 0;

	putlong(n.i);
}

void putinit(void)
{
	output = outputbuffer;
}

int putgetposition(void)
{
	return (int)output - (int)outputbuffer;
}

void putsetposition(int n)
{
	output = (unsigned char *)(n + (int)outputbuffer);
}

typedef struct lump_s
{
	int start, length;
} lump_t;

char m_triused[MAX_TRIS];
int m_numtriangles;
triangle m_triangles[MAX_TRIS];
int m_striptris[MAX_TRIS];

// returns number of triangles in tristrip, similar to
// what Quake does
int strip_length(int starttri, int startvert)
{
	int			m1, m2;
	int			j;
	triangle	*last, *check;
	int			k, stripcount;

	m_triused[starttri] = 2;
	last = &triangles[starttri];
	m_striptris[0] = starttri;
	stripcount = 1;

	m1 = last->v[(startvert+2)%3];
	m2 = last->v[(startvert+1)%3];

	// look for a matching triangle
nexttri:
	for (j = starttri + 1, check = &triangles[starttri+1]; j < m_numtriangles; j++, check++)
	{
		for (k = 0; k < 3; k++)
		{
			if (check->v[k] != m1)
				continue;
			if (check->v[(k+1)%3] != m2)
				continue;

			// if we can't use this triangle, this tristrip is done
			if (m_triused[j])
				goto done;

			// the new edge
			if (stripcount & 1)
				m2 = check->v[ (k+2)%3 ];
			else
				m1 = check->v[ (k+2)%3 ];

			m_striptris[stripcount++] = j;
			m_triused[j] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for (j = starttri + 1; j < m_numtriangles; j++)
		if (m_triused[j] == 2)
			m_triused[j] = 0;

	return stripcount;
}

void build_tris(void)
{
	int		i, j;
	int		startv;
	int		len, bestlen;
	int		besttris[MAX_TRIS];

	memset (m_triused, 0, sizeof(m_triused));
	for (i = 0; i < m_numtriangles; i++)
	{
		if (m_triused[i])
			continue;

		bestlen = 0;
		for (startv = 0; startv < 3; startv++)
		{
			len = strip_length (i, startv);
			if (len > bestlen)
			{
				bestlen = len;
				for (j = 0; j < bestlen; j++)
					besttris[j] = m_striptris[j];
			}
		}

		for (j = 0; j < bestlen; j++)
			m_triused[besttris[j]] = 1;

		for (j = 0; j < bestlen; j++)
		{
			putlong (m_triangles[besttris[j]].v[0]);
			putlong (m_triangles[besttris[j]].v[1]);
			putlong (m_triangles[besttris[j]].v[2]);
		}
	}
}

int convertmodel(void)
{
	int i, j, k, nverts, ntris;
	int pos_filesize, pos_lumps, pos_frames, pos_bones, pos_meshes, pos_verts, pos_texcoords, pos_index, pos_references;
	int filesize, restoreposition;
	int numreferences, references[MAX_BONES];
	char meshname[MAX_NAME];

	memset(meshname, 0, sizeof(meshname));

	putinit();

	// ID string
	putstring(SKMFORMAT_HEADER, strlen(SKMFORMAT_HEADER));

	// model type
	putlong(SKMFORMAT_VERSION);

	// filesize
	pos_filesize = putgetposition();
	putlong(0);

	// numbers of things
	putlong(numbones);
	putlong(numshaders);

	// offsets to things
	pos_lumps = putgetposition();
	putlong(0);

	// store the meshes
	pos_meshes = putgetposition();

	// skip over the mesh structs, they will be filled in later
	putsetposition(pos_meshes + numshaders * (MAX_NAME*2 + 28));

	// store the data referenced by meshes
	for (i = 0; i < numshaders; i++)
	{
		pos_verts = putgetposition();
		nverts = 0;
		m_numtriangles = 0;

		memset (references, 0, sizeof(references));
		for (j = 0; j < numverts; j++)
		{
			if (vertices[j].shadernum == i)
			{
				vertremap[j] = nverts++;

//JAL:HL2SMD[start]
				putlong(vertices[j].numinfluences); // how many bones for this vertex (always 1 for smd)
				for( k=0; k<vertices[j].numinfluences; k++ ) {
					putfloat(vertices[j].influenceorigin[k][0] * vertices[j].influenceweight[k]);
					putfloat(vertices[j].influenceorigin[k][1] * vertices[j].influenceweight[k]);
					putfloat(vertices[j].influenceorigin[k][2] * vertices[j].influenceweight[k]);
					putfloat(vertices[j].influenceweight[k]); // influence of the bone on the vertex
					putfloat(vertices[j].influencenormal[k][0] * vertices[j].influenceweight[k]);
					putfloat(vertices[j].influencenormal[k][1] * vertices[j].influenceweight[k]);
					putfloat(vertices[j].influencenormal[k][2] * vertices[j].influenceweight[k]);
					putlong(vertices[j].influencebone[k]); // number of the bone

					references[vertices[j].influencebone[k]]++;
				}
//JAL:HL2SMD[end]
			}
			else
				vertremap[j] = -1;
		}

		pos_texcoords = putgetposition();
		for (j = 0; j < numverts; j++)
		{
			if (vertices[j].shadernum == i)
			{
				// OpenGL wants bottom to top texcoords
				putfloat(vertices[j].texcoord[0]);
				putfloat(1.0f - vertices[j].texcoord[1]);
			}
		}

		pos_index = putgetposition();
		ntris = 0;
		for (j = 0; j < numtriangles; j++)
		{
			if (triangles[j].shadernum == i)
			{
				m_triangles[m_numtriangles].v[0] = vertremap[triangles[j].v[0]];
				m_triangles[m_numtriangles].v[1] = vertremap[triangles[j].v[2]];
				m_triangles[m_numtriangles].v[2] = vertremap[triangles[j].v[1]];
				m_numtriangles++;
				ntris++;
			}
		}

		build_tris();
/*
		// put all referenced bones' parents into reference list
		for (j = 0; j < numbones; j++)
		{
			if (references[j])
			{
				k = bones[j].parent;
				while ((k != -1) && !(references[bones[k].parent]))
				{
					references[bones[k].parent]++;
					k = bones[k].parent;
				}
			}
		}
*/
		pos_references = putgetposition();
		for (j = 0, numreferences = 0; j < numbones; j++)
		{
			if (references[j])
			{
				putlong(j);
				numreferences++;
			}
		}

		// now we actually write the mesh header
		restoreposition = putgetposition();
		putsetposition(pos_meshes + i * (MAX_NAME*2 + 28));
		putstring(shaders[i], MAX_NAME);

		sprintf(meshname, "%i", i);
		putstring(meshname, MAX_NAME);

		putlong(nverts);
		putlong(ntris);
		putlong(numreferences);
		putlong(pos_verts);
		putlong(pos_texcoords);
		putlong(pos_index);
		putlong(pos_references);
		putsetposition(restoreposition);
	}

	filesize = putgetposition();
	putsetposition(pos_lumps);
	putlong(pos_meshes);
	putsetposition(pos_filesize);
	putlong(filesize);
	putsetposition(filesize);

	// print model stats
	printf("model stats:\n");
	printf("%i vertices %i triangles %i bones %i shaders %i frames\n", numverts, numtriangles, numbones, numshaders, numframes);
	printf("renderlist:\n");

	for (i = 0; i < numshaders; i++)
	{
		nverts = 0;
		for (j = 0; j < numverts; j++)
			if (vertices[j].shadernum == i)
				nverts++;

		ntris = 0;
		for (j = 0; j < numtriangles; j++)
			if (triangles[j].shadernum == i)
				ntris++;

		printf("%5i tris%6i verts : %s\n", ntris, nverts, shaders[i]);
	}

	printf("file size: %5ik\n", (filesize + 1023) >> 10);
	writefile(output_name, outputbuffer, filesize);
	printf("wrote file %s\n", output_name);

	//jal: write the .skin file.
	{
		FILE *skinfile = NULL;
		char skinfile_name[MAX_PATH];
		
		sprintf(skinfile_name, "%s%s.skin", outputdir_name, model_name);
		
		if (!skinfile)
		{
			skinfile = fopen(skinfile_name, "wb");
			
			fprintf(skinfile, "//Skin file generated by skmodel\n");
			fprintf(skinfile, "//this converter generates the mesh names by their mesh number\n");
			for (i = 0; i < numshaders; i++)
				fprintf(skinfile, "%i,%s\n", i, shaders[i]);
		}

		fclose(skinfile);
		printf("wrote file %s\n", skinfile_name);
	}

	// store the data referenced by frames
	if (pose_name[0])
	{
		int pos_framebones;

		putinit();

		// ID string
		putstring(SKMFORMAT_HEADER, strlen(SKMFORMAT_HEADER));

		// model type
		putlong(SKMFORMAT_VERSION);

		// filesize
		pos_filesize = putgetposition();
		putlong(0);

		putlong(numbones);
		putlong(numframes);

		// offsets to things
		pos_lumps = putgetposition();
		putlong(0);
		putlong(0);

		// store the bones
		pos_bones = putgetposition();
		for (i = 0; i < numbones; i++)
		{
			putstring(bones[i].name, MAX_NAME);
			putlong(bones[i].parent);
			putlong(bones[i].flags);
		}

		// store the frames
		pos_frames = putgetposition();
		// skip over the frame structs, they will be filled in later
		putsetposition(pos_frames + numframes * (MAX_NAME + 4));

		for (i = 0; i < numframes; i++)
		{
			pos_framebones = putgetposition();
			for (j = 0; j < numbones; j++)
			{
				for (k = 0; k < 4; k++)
					putfloat(frames[i].bones[j].q[k]);
				for (k = 0; k < 3; k++)
					putfloat(frames[i].bones[j].origin[k]);
			}

			// now we actually write the frame header
			restoreposition = putgetposition();
			putsetposition(pos_frames + i * (MAX_NAME + 4));
			putstring(frames[i].name, MAX_NAME);
			putlong(pos_framebones);
			putsetposition(restoreposition);
		}

		filesize = putgetposition();
		putsetposition(pos_lumps);
		putlong(pos_bones);
		putlong(pos_frames);
		putsetposition(pos_filesize);
		putlong(filesize);
		putsetposition(filesize);

		printf("\n");

		printf("file size: %5ik\n", (filesize + 1023) >> 10);
		writefile(pose_name, outputbuffer, filesize);
		printf("wrote file %s\n", pose_name);
	}

	return 1;
}

