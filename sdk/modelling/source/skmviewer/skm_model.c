
#include "globals.h"
#include "misc_utils.h"
#include "skm_model.h"
#include <math.h>
#include "quaternions.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//rendering info
typedef struct {
	float	v[3];
} vertex_t;

typedef struct {
	float	v[2];
} texcoord_t;
extern vertex_t *varray_vertex;
extern vertex_t *varray_normal;
extern texcoord_t *varray_texcoord;
int textureforimage(const char *name);
void R_Mesh_ResizeCheck(int numverts);
void R_DrawMesh( unsigned int numverts, unsigned int numtris, unsigned int *indices, const char *shadername );
void R_DrawLine(float origin[3], float origin2[3], float color[3], int depthtest);

void SKM_GenerateAnimationScenes( mskmodel_t *model );


mskmodel_t *modelSKM; //tmp model holder

void Str_StripExtension ( const char *in, char *out)
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}
void Str_FixedExtension (char *filename, const char *extension)
{
	Str_StripExtension( filename, filename );
	sprintf(filename, "%s.%s", filename, extension);
}


void SKM_freemodel(void) 
{ 
	int i, j;

	//SKP mallocs
	for( i = 0; i < (int)modelSKM->numframes; i++ )
		free( modelSKM->frames[i].boneposes );

	free( modelSKM->frames );
	free( modelSKM->bones );

	//SKM mallocs
	for( i = 0; i < (int)modelSKM->nummeshes; i++ ) 
	{
		for( j = 0; j < (int)modelSKM->meshes[i].numverts; j++ )
			free( modelSKM->meshes[i].vertexes[j].verts );

		free( modelSKM->meshes[i].vertexes );
		free( modelSKM->meshes[i].stcoords );
		free( modelSKM->meshes[i].indexes );
		free( modelSKM->meshes[i].references );
	}

	free( modelSKM->meshes );
	free( modelSKM );
}

//========================
//Save Skeletal Poses file
//========================
void SKM_saveSKPfile(mskmodel_t *model)
{
	int		i, j, k;
	int		pos_filesize;
	int		pos_offsets;
	int		pos_bones;
	int		pos_frames;
	int		pos_framebones;
	int		restoreposition;
	int		filesize;
	
	// create filehandle
	putinit (SKM_MAX_FILESIZE);

	// write header
	//---------------

	// ID string
	putstring (SKMHEADER, strlen(SKMHEADER));
	
	// model type
	putlong (SKM_MODELTYPE);
	
	// filesize
	pos_filesize = putgetposition ();
	putlong (0);
	
	putlong (model->numbones);
	putlong (model->numframes);
	
	// offsets to things
	pos_offsets = putgetposition ();
	putlong (0);
	putlong (0);


	// write bones
	//---------------
	pos_bones = putgetposition ();

	for (i = 0; i < (int)model->numbones; i++)
	{
		putstring (model->bones[i].name, SKM_MAX_NAME);
		putlong (model->bones[i].parent);
		putlong (model->bones[i].flags);
	}
	
	// write frames
	//---------------
	pos_frames = putgetposition ();

	// skip over the frame structs, they will be filled in later
	putsetposition (pos_frames + model->numframes * (SKM_MAX_NAME + 4));
	
	for (i = 0; i < (int)model->numframes; i++)
	{
		pos_framebones = putgetposition ();
		for (j = 0; j < (int)model->numbones; j++)
		{
			for (k = 0; k < 4; k++)
				putfloat (model->frames[i].boneposes[j].quat[k]);
			for (k = 0; k < 3; k++)
				putfloat (model->frames[i].boneposes[j].origin[k]);
		}
		
		// now we actually write the frame header
		restoreposition = putgetposition ();
		putsetposition (pos_frames + i * (SKM_MAX_NAME + 4));
		putstring (model->frames[i].name, SKM_MAX_NAME);
		putlong (pos_framebones);
		putsetposition (restoreposition);
	}


	// Finish: fill header data
	//---------------
	filesize = putgetposition ();
	putsetposition (pos_offsets);
	putlong (pos_bones);
	putlong (pos_frames);
	putsetposition (pos_filesize);
	putlong (filesize);
	putsetposition (filesize);

	// write the file
	putwritefile ("data/out.skp", filesize);
	printf ("wrote SKP file\n");
}

//========================
//Save Skeletal Model file
//========================
void SKM_saveSKMfile(mskmodel_t *model)
{
	int i, j;
	int	k, l;
	int pos_filesize;
	int	pos_ofs_meshes;
	int	pos_meshes;
	int	pos_verts;
	int	pos_texcoords;
	int	pos_index;
	int	pos_references;
	int filesize;
	int restoreposition;
	mskvertex_t		*poutskmvert;
	mskbonevert_t	*poutbonevert;
	vec2			*poutstcoord;
	index_t			*pouttris;
	unsigned int	*poutreferences;


	// create filehandle
	putinit (SKM_MAX_FILESIZE);

	// write header
	//---------------

	// ID string
	putstring (SKMHEADER, strlen(SKMHEADER));

	// model type
	putlong (SKM_MODELTYPE);

	// filesize
	pos_filesize = putgetposition ();
	putlong (0);

	// numbers of things
	putlong (model->numbones);
	putlong (model->nummeshes);

	// offsets to things
	pos_ofs_meshes = putgetposition ();
	putlong (0);

	// store the meshes
	pos_meshes = putgetposition ();

	// skip over the mesh structs, they will be filled in later
	putsetposition (pos_meshes + model->nummeshes * (SKM_MAX_NAME*2 + 28));

	//mesh cycle
	for (i = 0; i < (int)model->nummeshes; i++)
	{
		//write vertices (ofs_verts)
		//---------------
		pos_verts = putgetposition ();

		poutskmvert = model->meshes[i].vertexes;
		for( j = 0; j < (int)model->meshes[i].numverts; j++, poutskmvert++ ) 
		{
			putlong(poutskmvert->numbones);
			poutbonevert = model->meshes[i].vertexes[j].verts;
		
			for( l = 0; l < (int)poutskmvert->numbones; l++, poutbonevert++ ) 
			{
				for( k = 0; k < 3; k++ )
					putfloat(poutbonevert->origin[k]);
				
				putfloat(poutbonevert->influence);

				for( k = 0; k < 3; k++ )
					putfloat(poutbonevert->normal[k]);
				
				putlong(poutbonevert->bonenum);
			}
		}

		//write stcoords (ofs_stcoords)
		//---------------
		pos_texcoords = putgetposition ();

		poutstcoord = model->meshes[i].stcoords;

		for( j = 0; j < (int)model->meshes[i].numverts; j++ ) {
			putfloat(poutstcoord[j][0]);
			putfloat(poutstcoord[j][1]);
		}

		//write indices (ofs_indices)
		//---------------
		pos_index = putgetposition ();

		pouttris = model->meshes[i].indexes;

		for( j = 0; j < (int)model->meshes[i].numtris; j++, pouttris += 3 ) {
			putlong(pouttris[0]);
			putlong(pouttris[1]);
			putlong(pouttris[2]);
		}

		//write references (ofs_references)
		//---------------
		pos_references = putgetposition ();

		poutreferences = model->meshes[i].references;

		for( j = 0; j < (int)model->meshes[i].numreferences; j++, poutreferences++ )
			putlong(*poutreferences);


		//write mesh header
		//---------------
		restoreposition = putgetposition ();
		putsetposition (pos_meshes + i * (SKM_MAX_NAME*2 + 28));
		putstring (model->meshes[i].shadername, SKM_MAX_NAME);
		putstring (model->meshes[i].name, SKM_MAX_NAME);

		putlong (model->meshes[i].numverts);
		putlong (model->meshes[i].numtris);
		putlong (model->meshes[i].numreferences);
		putlong (pos_verts);
		putlong (pos_texcoords);
		putlong (pos_index);
		putlong (pos_references);
		putsetposition (restoreposition);

	}

	//finish: SKM file header
	filesize = putgetposition ();
	putsetposition (pos_ofs_meshes);
	putlong (pos_meshes);
	putsetposition (pos_filesize);
	putlong (filesize);
	putsetposition (filesize);	//restore position

	putwritefile ("data/out.skm", filesize);
	printf ("wrote SKM file\n");
}

//========================
//Load Skeletal Poses file
//========================
int SKM_ReadSKPfile(char *name, mskmodel_t	*poutmodel)
{
	unsigned char	*buffer = NULL;
	char			filename[1024];

	int			filesize = 0;
	int			i, j, k;
	dskpheader_t	*pinmodel;
	dskpbone_t		*pinbone;
	dskpframe_t		*pinframe;
	dskpbonepose_t	*pinbonepose;
	mskbone_t		*poutbone;
	mskframe_t		*poutframe;
	mskbonepose_t	*poutbonepose;

	strcpy( filename, name );
	Str_FixedExtension( filename, "skp" );
	buffer = loadfile( filename, &filesize );
	if( !buffer )
		return 0;

	pinmodel = ( dskpheader_t * )buffer;
	printf ( "----------------------\n");
	printf ( "%s pinmodel type: %i\n", filename, pinmodel->type);
	printf ( "%s pinmodel filesize: %i (MAX: %i)\n", filename, pinmodel->filesize, SKM_MAX_FILESIZE);
	printf ( "%s pinmodel numbones: %i\n", filename, pinmodel->num_bones);
	if( LittleLong (pinmodel->type) != SKM_MODELTYPE )
		printf ( "%s has wrong type number (%i should be %i)\n",
		filename, LittleLong (pinmodel->type), SKM_MODELTYPE );
	if( LittleLong (pinmodel->type) != (int)pinmodel->type || 
		LittleLong (pinmodel->type) > SKM_MAX_BONES) {
		printf ( "%s num_bones: Invalid value\n", filename );
	}

	poutmodel->numframes = LittleLong ( pinmodel->num_frames );
	printf("numframes:pout:%i pin:%i\n", poutmodel->numframes, LittleLong ( pinmodel->num_frames ));
	if( poutmodel->numframes <= 0 )
		printf("%s has no frames", filename );
	else if ( poutmodel->numframes > SKM_MAX_FRAMES )
		printf("%s has too many frames", filename );

	//copy numbones (at QF it is set up at loading the model)

	poutmodel->numbones = LittleLong ( pinmodel->num_bones );
	printf("NumBones(SKP): %i\n",(int)poutmodel->numbones);

	pinbone = ( dskpbone_t * )( ( unsigned char * )pinmodel + LittleLong ( pinmodel->ofs_bones ) );
	poutbone = poutmodel->bones = malloc ( sizeof(mskbone_t) * poutmodel->numbones );

	for( i = 0; i < (int)poutmodel->numbones; i++, pinbone++, poutbone++ ) 
	{
		strncpy( poutbone->name, pinbone->name, SKM_MAX_NAME );
		poutbone->flags = LittleLong ( pinbone->flags );
		poutbone->parent = LittleLong ( pinbone->parent );
	}

	pinframe = ( dskpframe_t * )( ( unsigned char * )pinmodel + LittleLong ( pinmodel->ofs_frames ) );
	poutframe = poutmodel->frames = malloc ( sizeof(mskframe_t) * poutmodel->numframes );

	for( i = 0; i < (int)poutmodel->numframes; i++, pinframe++, poutframe++ ) {
		poutbone = poutmodel->bones;

		// frames names are ignored at QFusion, but not here
		strncpy( poutframe->name, pinframe->name, SKM_MAX_NAME );

		pinbonepose = ( dskpbonepose_t * )( ( unsigned char * )pinmodel + LittleLong (pinframe->ofs_bonepositions) );
		poutbonepose = poutframe->boneposes = malloc ( sizeof(mskbonepose_t) * poutmodel->numbones );

		for( j = 0; j < (int)poutmodel->numbones; j++, poutbone++, pinbonepose++, poutbonepose++ ) 
		{
			for( k = 0; k < 4; k++ )
				poutbonepose->quat[k] = LittleFloat ( pinbonepose->quat[k] );
			for( k = 0; k < 3; k++ )
				poutbonepose->origin[k] = LittleFloat ( pinbonepose->origin[k] );
		}
	}

	free(buffer);
	return 1;
}


//========================
//Load Skeletal Model file
//========================
mskmodel_t *SKM_ReadSKMfile( char* name )
{
	unsigned char	*buffer;
	char			filename[1024];

	int			i, j, l, k;
	int			filesize=0;
	dskmheader_t	*pinmodel;
	dskmmesh_t		*pinmesh;
	dskmvertex_t	*pinskmvert;
	dskmbonevert_t	*pinbonevert;
	dskmcoord_t		*pinstcoord;
	index_t			*pintris;
	unsigned int	*pinreferences;

	mskmodel_t		*poutmodel;
	mskmesh_t		*poutmesh;
	mskvertex_t		*poutskmvert;
	mskbonevert_t	*poutbonevert;
	vec2			*poutstcoord;
	index_t			*pouttris;
	unsigned int	*poutreferences;


	strcpy( filename, name );
	Str_FixedExtension( filename, "skm" );
	printf( "Loading %s\n", filename );
	buffer = loadfile( filename, &filesize );
	if( !buffer || filesize <= 0 )
		return NULL;

	poutmodel = malloc( sizeof(mskmodel_t) );
	pinmodel = ( dskmheader_t * )buffer;
	
	printf("%s: Type:%i\n", filename, LittleLong (pinmodel->type));
	if( LittleLong (pinmodel->type) != SKM_MODELTYPE ) {
		printf( "%s has wrong type number (%i should be %i)",
				 filename, LittleLong (pinmodel->type), SKM_MODELTYPE );
	}
	printf("%s: Filesize:%i\n", filename, LittleLong (pinmodel->filesize));
	if( LittleLong (pinmodel->filesize) > SKM_MAX_FILESIZE ) {
		printf( "%s has has wrong filesize (%i should be less than %i)",
				 filename, LittleLong (pinmodel->filesize), SKM_MAX_FILESIZE );
	}

	poutmodel = malloc( sizeof(mskmodel_t) );
	poutmodel->nummeshes = LittleLong( pinmodel->num_meshes );
	if( poutmodel->nummeshes <= 0 )
		printf("%s has no meshes", filename );
	else if( poutmodel->nummeshes > SKM_MAX_MESHES )
		printf("%s has too many meshes", filename );

	poutmodel->numbones = LittleLong( pinmodel->num_bones );
	if( poutmodel->numbones <= 0 )
		printf("%s has no bones", filename );
	else if( poutmodel->numbones > SKM_MAX_BONES )
		printf("%s has too many bones", filename );

	
	pinmesh = ( dskmmesh_t * )( ( unsigned char * )pinmodel + LittleLong ( pinmodel->ofs_meshes ) );
	poutmesh = poutmodel->meshes = malloc ( sizeof(mskmesh_t) * poutmodel->nummeshes );


	for( i = 0; i < (int)poutmodel->nummeshes; i++, pinmesh++, poutmesh++ ) {
		poutmesh->numverts = LittleLong ( pinmesh->num_verts );
		if( poutmesh->numverts <= 0 )
			printf("mesh %i in model %s has no vertexes", i, filename );
		else if( poutmesh->numverts > SKM_MAX_VERTS )
			printf("mesh %i in model %s has too many vertexes", i, filename );

		poutmesh->numtris = LittleLong ( pinmesh->num_tris );
		if( poutmesh->numtris <= 0 )
			printf("mesh %i in model %s has no indices", i, filename );
		else if( poutmesh->numtris > SKM_MAX_TRIS )
			printf("mesh %i in model %s has too many indices", i, filename );

		poutmesh->numreferences = LittleLong ( pinmesh->num_references );
		if( poutmesh->numreferences <= 0 )
			printf("mesh %i in model %s has no bone references", i, filename );
		else if( poutmesh->numreferences > SKM_MAX_BONES )
			printf("mesh %i in model %s has too many bone references", i, filename );

		
		strncpy( poutmesh->name, pinmesh->meshname, sizeof(poutmesh->name) );//SKM_MAX_NAME

		//shadername is not stored in QF (it registers the actual shader)
		strncpy( poutmesh->shadername, pinmesh->shadername, SKM_MAX_NAME );
		
		poutmesh->skin.shader = NULL;
	

		pinskmvert = ( dskmvertex_t * )( ( unsigned char * )pinmodel + LittleLong( pinmesh->ofs_verts ) );
		poutskmvert = poutmesh->vertexes = malloc( sizeof(mskvertex_t) * poutmesh->numverts );

		for( j = 0; j < (int)poutmesh->numverts; j++, poutskmvert++/*, pinskmvert++*/ ) {//JAL: pinskmvert++ by me
			poutskmvert->numbones = LittleLong( pinskmvert->numbones );

			pinbonevert = ( dskmbonevert_t * )( ( unsigned char * )pinskmvert + sizeof(poutskmvert->numbones) );
			poutbonevert = poutskmvert->verts = malloc( sizeof(mskbonevert_t) * poutskmvert->numbones );

			for( l = 0; l < (int)poutskmvert->numbones; l++, pinbonevert++, poutbonevert++ ) {
				for( k = 0; k < 3; k++ ) {
					poutbonevert->origin[k] = LittleFloat( pinbonevert->origin[k] );
					poutbonevert->normal[k] = LittleFloat( pinbonevert->normal[k] );
				}

				poutbonevert->influence = LittleFloat( pinbonevert->influence );
				poutbonevert->bonenum = LittleLong( pinbonevert->bonenum );
			}

			pinskmvert = ( dskmvertex_t * )( ( unsigned char * )pinbonevert );	//JAL: moved to pinskmvert++
		}

		pinstcoord = ( dskmcoord_t * )( ( unsigned char * )pinmodel + LittleLong (pinmesh->ofs_texcoords) );
		poutstcoord = poutmesh->stcoords = malloc( poutmesh->numverts * sizeof(vec2) );//jalfixme vec2

		for( j = 0; j < (int)poutmesh->numverts; j++, pinstcoord++/*, poutstcoord++*/ ) {
			poutstcoord[j][0] = LittleFloat( pinstcoord->st[0] );
			poutstcoord[j][1] = LittleFloat( pinstcoord->st[1] );
		}

		pintris = ( index_t * )( ( unsigned char * )pinmodel + LittleLong (pinmesh->ofs_indices) );
		pouttris = poutmesh->indexes = malloc( sizeof(index_t) * poutmesh->numtris * 3 );

		for( j = 0; j < (int)poutmesh->numtris; j++, pintris += 3, pouttris += 3 ) {
			pouttris[0] = (index_t)LittleLong( pintris[0] );
			pouttris[1] = (index_t)LittleLong( pintris[1] );
			pouttris[2] = (index_t)LittleLong( pintris[2] );
		}

		pinreferences = ( index_t *)( ( unsigned char * )pinmodel + LittleLong (pinmesh->ofs_references) );
		poutreferences = poutmesh->references = malloc( sizeof(unsigned int) * poutmesh->numreferences );

		for( j = 0; j < (int)poutmesh->numreferences; j++, pinreferences++, poutreferences++ ) {
			*poutreferences = LittleLong( *pinreferences );
		}

	}

	free(buffer);
	return poutmodel;
}


void SKM_precacheimages( mskmodel_t *model )
{
	int i;
	mskmesh_t *mesh;
	for (i = 0, mesh = model->meshes;i < (int)model->nummeshes;i++, mesh++)
		textureforimage(mesh->shadername);
}

//========================
//Load SKM Skeletal Model
//========================
int SKM_LoadModel( char* filename )
{
	mskmodel_t	*model;

	model = SKM_ReadSKMfile( filename );
	if( !model )
		return 0;

	if( !SKM_ReadSKPfile(filename, model) ) //add frames boneposes
		return 0;

	//SKM_precacheimages(model);

	modelSKM = model;	//static handler for the viewer
	
	// Set up for animations
	SKM_GenerateAnimationScenes( modelSKM );

	return 1;
}


//===============================================
//
//	SKM drawing code
//
//===============================================

static mskbonepose_t oldboneposescache[SKM_MAX_BONES];		//old frame pose cache
static mskbonepose_t curboneposescache[SKM_MAX_BONES];		//current frame pose cache
static mskbonepose_t lerpboneposescache[SKM_MAX_BONES];		//lerped pose cache

//========================
// SKM_TransformBoneposes
//========================
void SKM_TransformBoneposes( mskmodel_t *model, mskbonepose_t *boneposes, mskbonepose_t *sourceboneposes )
{
	int				j;
	mskbonepose_t	temppose;

	for( j = 0; j < (int)model->numbones; j++ ) 
	{
		if( model->bones[j].parent >= 0 )
		{
			memcpy( &temppose, &sourceboneposes[j], sizeof(mskbonepose_t));
			Quat_ConcatTransforms ( boneposes[model->bones[j].parent].quat, boneposes[model->bones[j].parent].origin, temppose.quat, temppose.origin, boneposes[j].quat, boneposes[j].origin );
			
		} else
			memcpy( &boneposes[j], &sourceboneposes[j], sizeof(mskbonepose_t));	
	}
}

//========================
// SKM_LerpBoneposes
//========================
void SKM_LerpBoneposes( mskmodel_t *model, mskbonepose_t *boneposes, mskbonepose_t *oldboneposes, mskbonepose_t *lerpboneposes, float frontlerp )
{
	int			i;
	mskbonepose_t	*bonepose, *oldbonepose, *lerpbpose;

	//run all bones
	for( i = 0; i < (int)model->numbones; i++ ) {
		bonepose = boneposes + i;
		oldbonepose = oldboneposes + i;
		lerpbpose = lerpboneposes + i;

		// lerp
		Quat_Lerp( oldbonepose->quat, bonepose->quat, frontlerp, lerpbpose->quat );
		lerpbpose->origin[0] = oldbonepose->origin[0] + (bonepose->origin[0] - oldbonepose->origin[0]) * frontlerp;
		lerpbpose->origin[1] = oldbonepose->origin[1] + (bonepose->origin[1] - oldbonepose->origin[1]) * frontlerp;
		lerpbpose->origin[2] = oldbonepose->origin[2] + (bonepose->origin[2] - oldbonepose->origin[2]) * frontlerp;
	}
}

//========================
// SKM_DrawSkeleton
//========================
void SKM_DrawSkeleton(mskmodel_t *model, mskbonepose_t *boneposes)
{
	int				i, j;
	float			color[3];
	float			tagColor[3][3];
	float			point[3], axis[3][3];
	
	color[RED] = 0.4f;color[GREEN] = 0.5f;color[BLUE] = 0.7f;

	tagColor[0][RED] = 0.4f;tagColor[0][GREEN] = 0.9f;tagColor[0][BLUE] = 0.4f;
	tagColor[1][RED] = 0.9f;tagColor[1][GREEN] = 0.4f;tagColor[1][BLUE] = 0.4f;
	tagColor[2][RED] = 0.9f;tagColor[2][GREEN] = 0.9f;tagColor[2][BLUE] = 0.4f;
	
	for( j = 0; j < (int)model->numbones; j++)
	{
		if (model->bones[j].parent >= 0) 
		{
			if( model->bones[j].flags & SKM_BONEFLAG_ATTACH ) {
				Quat_Matrix( boneposes[j].quat, axis );
				for( i = 0; i < 3; i++ ) {
					point[0] = axis[i][0] + boneposes[j].origin[0];
					point[1] = axis[i][1] + boneposes[j].origin[1];
					point[2] = axis[i][2] + boneposes[j].origin[2];
					R_DrawLine( boneposes[j].origin, point, tagColor[i], 0 );
				}
			}

			R_DrawLine( boneposes[model->bones[j].parent].origin, boneposes[j].origin, color, 0 );
		}
	}
}

//========================
// SKM_DrawModel
//========================
void SKM_DrawModel( mskmodel_t *model, mskbonepose_t *bonepose)
{
	int				meshnum;
	int				j, k, l;
	float			poseaxis[3][3];
	mskbonepose_t	*pose;
	mskvertex_t		*skmverts;
	mskbonevert_t	*boneverts;
	mskmesh_t		*mesh;

	for (meshnum = 0, mesh = model->meshes; meshnum < (int)model->nummeshes; meshnum++, mesh++)
	{
		R_Mesh_ResizeCheck( mesh->numverts );
		
		for( j = 0, skmverts = mesh->vertexes; j < (int)mesh->numverts; j++, skmverts++ ) 
		{
			varray_vertex[j].v[0] = varray_normal[j].v[0] = 0;
			varray_vertex[j].v[1] = varray_normal[j].v[1] = 0;
			varray_vertex[j].v[2] = varray_normal[j].v[2] = 0;

			for( l = 0, boneverts = skmverts->verts; l < (int)skmverts->numbones; l++, boneverts++ ) 
			{
				pose = bonepose + boneverts->bonenum;
				Quat_Matrix( pose->quat, poseaxis );
				
				for( k = 0; k < 3; k++ ) 
				{
					varray_vertex[j].v[k] += boneverts->origin[0] * poseaxis[k][0] +
						boneverts->origin[1] * poseaxis[k][1] +
						boneverts->origin[2] * poseaxis[k][2] +
						boneverts->influence * pose->origin[k];
					varray_normal[j].v[k] += boneverts->normal[0] * poseaxis[k][0] +
						boneverts->normal[1] * poseaxis[k][1] +
						boneverts->normal[2] * poseaxis[k][2];
				}
			}
			
//			VectorNormalizeFast( varray_normal[j] );

			varray_texcoord[j].v[0] = mesh->stcoords[j][0];
			varray_texcoord[j].v[1] = mesh->stcoords[j][1];
		}

		R_DrawMesh( mesh->numverts, mesh->numtris, mesh->indexes, mesh->shadername );
	}
}

//========================
// SKM_DrawFrameLerp
//========================
void SKM_DrawFrameLerp( mskmodel_t *model, int showmodel, int showskeleton, int curframe, int oldframe, float frontlerp )
{
	//bild frame poses into the boneposes caches
	SKM_TransformBoneposes( model, curboneposescache, model->frames[curframe].boneposes );
	SKM_TransformBoneposes( model, oldboneposescache, model->frames[oldframe].boneposes );
	SKM_LerpBoneposes( model, curboneposescache, oldboneposescache, lerpboneposescache, frontlerp );

	if( showmodel ) SKM_DrawModel( model, lerpboneposescache );
	if( showskeleton ) SKM_DrawSkeleton( model, lerpboneposescache );
}

//========================
// SKM_Draw
//========================
void SKM_Draw( int showmodel, int showskeleton, int curframe, int oldframe, float lerp ) 
{
	if( !modelSKM ) {
		printf( "model SKM not loaded\n" );
		return;
	}
	SKM_DrawFrameLerp( modelSKM, showmodel, showskeleton, curframe, oldframe, lerp );
}



//===============================================
//
//	Viewer stuff, animations and such
//
//===============================================
#include "animations.h"
void SKM_GenerateAnimationScenes( mskmodel_t *model )
{
	int		i;
	int		num_animations;

	for ( i=0; i<(int)model->numframes; i++ )
	{
		num_animations = SMViewer_AssignFrameToAnim( i, model->frames[i].name );	
	}

	SMViewer_PrintAnimations();
}