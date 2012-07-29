/*
Copyright (C) 1997-2001 Id Software, Inc.

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

/*
	Copyright (C) 1999-2000  Brian Paul, All Rights Reserved.

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
	BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
	AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


	The Mesa OpenGL headers were originally adapted in 2001 for dynamic OpenGL
	binding by Zephaniah E. Hull and later rewritten by Joseph Carter.  This
	version of the file is for the generation 3 DynGL code, and has been
	adapted by Joseph Carter.  He and Zeph have decided to hereby disclaim all
	Copyright of this work.  It is released to the Public Domain WITHOUT ANY
	WARRANTY whatsoever, express or implied, in the hopes that others will use
	it instead of other less-evolved hacks which usually don't work right.  ;)
*/

/*
	The following code is loosely based on DynGL code by Joseph Carter
	and Zephaniah E. Hull. Adapted by Victor Luchits for qfusion project.
*/

/*
** QGL.H
*/
#ifndef __QGL_H__
#define __QGL_H__

#define GL_GLEXT_LEGACY
#define GLX_GLXEXT_LEGACY

#if !defined (__MACOSX__)
#include <GL/gl.h>
#endif

#if defined (__linux__) || defined (__FreeBSD__)
#include <GL/glx.h>
#endif

#if defined (__MACOSX__)
# include <OpenGL/gl.h>
# include <OpenGL/glext.h>
#endif

#undef GL_GLEXT_LEGACY
#undef GLX_GLXEXT_LEGACY

QGL_EXTERN	qboolean	QGL_Init( const char *dllname );
QGL_EXTERN	void		QGL_Shutdown( void );

QGL_EXTERN	void		*qglGetProcAddress( const GLubyte * );
QGL_EXTERN	const char	*(*qglGetGLWExtensionsString)( void );

/*
** extension constants
*/

#define GL_TEXTURE0_SGIS									0x835E
#define GL_TEXTURE1_SGIS									0x835F
#define GL_TEXTURE0_ARB										0x84C0
#define GL_TEXTURE1_ARB										0x84C1
#define GL_MAX_TEXTURE_UNITS								0x84E2
#define GL_MAX_TEXTURE_UNITS_ARB							0x84E2

#ifndef GL_POLYGON_OFFSET
#define GL_POLYGON_OFFSET									0x8037
#endif

/* GL_ARB_texture_env_combine */
#ifndef GL_ARB_texture_env_combine
#define GL_ARB_texture_env_combine

#define GL_COMBINE_ARB										0x8570
#define GL_COMBINE_RGB_ARB									0x8571
#define GL_COMBINE_ALPHA_ARB								0x8572
#define GL_RGB_SCALE_ARB									0x8573
#define GL_ADD_SIGNED_ARB									0x8574
#define GL_INTERPOLATE_ARB									0x8575
#define GL_CONSTANT_ARB										0x8576
#define GL_PRIMARY_COLOR_ARB								0x8577
#define GL_PREVIOUS_ARB										0x8578
#define GL_SOURCE0_RGB_ARB									0x8580
#define GL_SOURCE1_RGB_ARB									0x8581
#define GL_SOURCE2_RGB_ARB									0x8582
#define GL_SOURCE0_ALPHA_ARB								0x8588
#define GL_SOURCE1_ALPHA_ARB								0x8589
#define GL_SOURCE2_ALPHA_ARB								0x858A
#define GL_OPERAND0_RGB_ARB									0x8590
#define GL_OPERAND1_RGB_ARB									0x8591
#define GL_OPERAND2_RGB_ARB									0x8592
#define GL_OPERAND0_ALPHA_ARB								0x8598
#define GL_OPERAND1_ALPHA_ARB								0x8599
#define GL_OPERAND2_ALPHA_ARB								0x859A
#endif /* GL_ARB_texture_env_combine */

/* GL_ARB_texture_compression */
#ifndef GL_ARB_texture_compression
#define GL_ARB_texture_compression

#define GL_COMPRESSED_ALPHA_ARB								0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB							0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB					0x84EB
#define GL_COMPRESSED_INTENSITY_ARB							0x84EC
#define GL_COMPRESSED_RGB_ARB								0x84ED
#define GL_COMPRESSED_RGBA_ARB								0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB						0x84EF
#define GL_TEXTURE_IMAGE_SIZE_ARB							0x86A0
#define GL_TEXTURE_COMPRESSED_ARB							0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB				0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB					0x86A3
#endif /* GL_ARB_texture_compression */

/* GL_EXT_texture_filter_anisotropic */
#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic

#define GL_TEXTURE_MAX_ANISOTROPY_EXT						0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT					0x84FF
#endif /* GL_EXT_texture_filter_anisotropic */

/* GL_EXT_texture_edge_clamp */
#ifndef GL_EXT_texture_edge_clamp
#define GL_EXT_texture_edge_clamp

#define GL_CLAMP_TO_EDGE									0x812F
#endif /* GL_EXT_texture_edge_clamp */

/* GL_ARB_vertex_buffer_object */
#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object

typedef int GLintptrARB;
typedef int GLsizeiptrARB;

#define GL_ARRAY_BUFFER_ARB									0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB							0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB							0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB					0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB					0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB					0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB					0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB					0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB			0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB				0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB			0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB			0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB					0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB			0x889F
#define GL_STREAM_DRAW_ARB									0x88E0
#define GL_STREAM_READ_ARB									0x88E1
#define GL_STREAM_COPY_ARB									0x88E2
#define GL_STATIC_DRAW_ARB									0x88E4
#define GL_STATIC_READ_ARB									0x88E5
#define GL_STATIC_COPY_ARB									0x88E6
#define GL_DYNAMIC_DRAW_ARB									0x88E8
#define GL_DYNAMIC_READ_ARB									0x88E9
#define GL_DYNAMIC_COPY_ARB									0x88EA
#define GL_READ_ONLY_ARB									0x88B8
#define GL_WRITE_ONLY_ARB									0x88B9
#define GL_READ_WRITE_ARB									0x88BA
#define GL_BUFFER_SIZE_ARB									0x8764
#define GL_BUFFER_USAGE_ARB									0x8765
#define GL_BUFFER_ACCESS_ARB								0x88BB
#define GL_BUFFER_MAPPED_ARB								0x88BC
#define GL_BUFFER_MAP_POINTER_ARB							0x88BD
#endif /* GL_ARB_vertex_buffer_object */

/* GL_ARB_texture_cube_map */
#ifndef GL_ARB_texture_cube_map
#define GL_ARB_texture_cube_map

#define GL_NORMAL_MAP_ARB									0x8511
#define GL_REFLECTION_MAP_ARB								0x8512
#define GL_TEXTURE_CUBE_MAP_ARB								0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB						0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB					0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB					0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB					0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB					0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB					0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB					0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB						0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB					0x851C
#endif /* GL_ARB_texture_cube_map */

/* GL_EXT_bgra */
#ifndef GL_EXT_bgra
#define GL_EXT_bgra

#define GL_BGR_EXT											0x80E0
#define GL_BGRA_EXT											0x80E1
#endif /* GL_EXT_bgra */

/* gl_ext_texture3D */
#ifndef GL_EXT_texture3D
#define GL_EXT_texture3D

#define GL_PACK_SKIP_IMAGES									0x806B
#define GL_PACK_IMAGE_HEIGHT								0x806C
#define GL_UNPACK_SKIP_IMAGES								0x806D
#define GL_UNPACK_IMAGE_HEIGHT								0x806E
#define GL_TEXTURE_3D										0x806F
#define GL_PROXY_TEXTURE_3D									0x8070
#define GL_TEXTURE_DEPTH									0x8071
#define GL_TEXTURE_WRAP_R									0x8072
#define GL_MAX_3D_TEXTURE_SIZE								0x8073
#define GL_TEXTURE_BINDING_3D								0x806A
#endif /* GL_EXT_texture3D */

/* GL_ARB_shader_objects */
#ifndef GL_ARB_shader_objects
#define GL_ARB_shader_objects

typedef char GLcharARB;
typedef unsigned int GLhandleARB;

#define GL_PROGRAM_OBJECT_ARB								0x8B40
#define GL_OBJECT_TYPE_ARB									0x8B4E
#define GL_OBJECT_SUBTYPE_ARB								0x8B4F
#define GL_OBJECT_DELETE_STATUS_ARB							0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB						0x8B81
#define GL_OBJECT_LINK_STATUS_ARB							0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB						0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB						0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB						0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB						0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB				0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB					0x8B88
#define GL_SHADER_OBJECT_ARB								0x8B48
#define GL_FLOAT											0x1406
#define GL_FLOAT_VEC2_ARB									0x8B50
#define GL_FLOAT_VEC3_ARB									0x8B51
#define GL_FLOAT_VEC4_ARB									0x8B52
#define GL_INT												0x1404
#define GL_INT_VEC2_ARB										0x8B53
#define GL_INT_VEC3_ARB										0x8B54
#define GL_INT_VEC4_ARB										0x8B55
#define GL_BOOL_ARB											0x8B56
#define GL_BOOL_VEC2_ARB									0x8B57
#define GL_BOOL_VEC3_ARB									0x8B58
#define GL_BOOL_VEC4_ARB									0x8B59
#define GL_FLOAT_MAT2_ARB									0x8B5A
#define GL_FLOAT_MAT3_ARB									0x8B5B
#define GL_FLOAT_MAT4_ARB									0x8B5C
#define GL_SAMPLER_1D_ARB									0x8B5D
#define GL_SAMPLER_2D_ARB									0x8B5E
#define GL_SAMPLER_3D_ARB									0x8B5F
#define GL_SAMPLER_CUBE_ARB									0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB							0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB							0x8B62
#define GL_SAMPLER_2D_RECT_ARB								0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB						0x8B64
#endif /* GL_ARB_shader_objects */

/* GL_ARB_vertex_shader */
#ifndef GL_ARB_vertex_shader
#define GL_ARB_vertex_shader

#define GL_VERTEX_SHADER_ARB								0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB				0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB							0x8B4B
#define GL_MAX_VERTEX_ATTRIBS_ARB							0x8869
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB						0x8872
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB				0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB				0x8B4D
#define GL_MAX_TEXTURE_COORDS_ARB							0x8871
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB					0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB						0x8643
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB						0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB			0x8B8A
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB					0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB						0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB					0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB						0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB				0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB						0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB					0x8645
#define GL_FLOAT											0x1406
#define GL_FLOAT_VEC2_ARB									0x8B50
#define GL_FLOAT_VEC3_ARB									0x8B51
#define GL_FLOAT_VEC4_ARB									0x8B52
#define GL_FLOAT_MAT2_ARB									0x8B5A
#define GL_FLOAT_MAT3_ARB									0x8B5B
#define GL_FLOAT_MAT4_ARB									0x8B5C
#endif /* GL_ARB_vertex_shader */

/* GL_ARB_fragment_shader */
#ifndef GL_ARB_fragment_shader
#define GL_ARB_fragment_shader

#define GL_FRAGMENT_SHADER_ARB								0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB				0x8B49
#define GL_MAX_TEXTURE_COORDS_ARB							0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB						0x8872
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB				0x8B8B
#endif /* GL_ARB_fragment_shader */

/* GL_ARB_shading_language_100 */
#ifndef GL_ARB_shading_language_100
#define GL_ARB_shading_language_100

#define GL_SHADING_LANGUAGE_VERSION_ARB						0x8B8C
#endif /* GL_ARB_shading_language_100 */

/* ARB_depth_texture */
#ifndef ARB_depth_texture
#define ARB_depth_texture

#define GL_DEPTH_COMPONENT16								0x81A5
#define GL_DEPTH_COMPONENT24								0x81A6
#define GL_DEPTH_COMPONENT32								0x81A7
#define GL_TEXTURE_DEPTH_SIZE								0x884A
#define GL_DEPTH_TEXTURE_MODE								0x884B
#endif /* ARB_depth_texture */

/* GL_ARB_shadow */
#ifndef GL_ARB_shadow
#define GL_ARB_shadow

#define	GL_DEPTH_TEXTURE_MODE_ARB							0x884B
#define GL_TEXTURE_COMPARE_MODE_ARB							0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB							0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB							0x884E
#define GL_TEXTURE_COMPARE_FAIL_VALUE_ARB					0x80BF
#endif /* GL_ARB_shadow */

/* GL_ARB_occlusion_query */
#ifndef GL_ARB_occlusion_query
#define GL_ARB_occlusion_query

#define GL_QUERY_COUNTER_BITS_ARB							0x8864
#define GL_CURRENT_QUERY_ARB								0x8865
#define GL_QUERY_RESULT_ARB									0x8866
#define GL_QUERY_RESULT_AVAILABLE_ARB						0x8867
#define GL_SAMPLES_PASSED_ARB								0x8914
#endif /* GL_ARB_occlusion_query */

/* GL_EXT_framebuffer_object */
#ifndef GL_EXT_framebuffer_object
#define GL_EXT_framebuffer_object

#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT				0x0506
#define GL_MAX_RENDERBUFFER_SIZE_EXT						0x84E8
#define GL_FRAMEBUFFER_BINDING_EXT							0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT							0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT			0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT			0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT			0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT	0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT							0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT			0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT	0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT	0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT			0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT				0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT			0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT			0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT						0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS_EXT						0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT							0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT							0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT							0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT							0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT							0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT							0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT							0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT							0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT							0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT							0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT							0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT							0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT							0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT							0x8CED
#define GL_COLOR_ATTACHMENT14_EXT							0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT							0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT								0x8D00
#define GL_STENCIL_ATTACHMENT_EXT							0x8D20
#define GL_FRAMEBUFFER_EXT									0x8D40
#define GL_RENDERBUFFER_EXT									0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT							0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT							0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT					0x8D44
#define GL_STENCIL_INDEX1_EXT								0x8D46
#define GL_STENCIL_INDEX4_EXT								0x8D47
#define GL_STENCIL_INDEX8_EXT								0x8D48
#define GL_STENCIL_INDEX16_EXT								0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT						0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT						0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT						0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT						0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT						0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT					0x8D55
#endif /* GL_EXT_framebuffer_object */

/* GL_NVX_gpu_memory_info */
#ifndef GL_NVX_gpu_memory_info
#define GL_NVX_gpu_memory_info

#define	GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX				0x9047
#define	GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX			0x9048
#define	GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX		0x9049
#define	GPU_MEMORY_INFO_EVICTION_COUNT_NVX					0x904A
#define	GPU_MEMORY_INFO_EVICTED_MEMORY_NVX					0x904B
#endif /* GL_NVX_gpu_memory_info */

/* GL_ATI_meminfo */
#ifndef GL_ATI_meminfo
#define GL_ATI_meminfo

#define VBO_FREE_MEMORY_ATI									0x87FB
#define TEXTURE_FREE_MEMORY_ATI								0x87FC
#define RENDERBUFFER_FREE_MEMORY_ATI						0x87FD
#endif /* GL_ATI_meminfo */

#endif /*__QGL_H__*/

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef QGL_FUNC
#define QGL_FUNC
#endif

// WGL Functions
QGL_WGL(PROC, wglGetProcAddress, (LPCSTR));
QGL_WGL(int, wglChoosePixelFormat, (HDC, CONST PIXELFORMATDESCRIPTOR *));
QGL_WGL(int, wglDescribePixelFormat, (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR));
QGL_WGL(BOOL, wglSetPixelFormat, (HDC, int, CONST PIXELFORMATDESCRIPTOR *));
QGL_WGL(BOOL, wglSwapBuffers, (HDC));
QGL_WGL(HGLRC, wglCreateContext, (HDC));
QGL_WGL(BOOL, wglDeleteContext, (HGLRC));
QGL_WGL(BOOL, wglMakeCurrent, (HDC, HGLRC));

// GLX Functions
QGL_GLX(void *, glXGetProcAddressARB, (const GLubyte *procName));
QGL_GLX(XVisualInfo *, glXChooseVisual, (Display *dpy, int screen, int *attribList));
QGL_GLX(GLXContext, glXCreateContext, (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct));
QGL_GLX(void, glXDestroyContext, (Display *dpy, GLXContext ctx));
QGL_GLX(Bool, glXMakeCurrent, (Display *dpy, GLXDrawable drawable, GLXContext ctx));
QGL_GLX(Bool, glXCopyContext, (Display *dpy, GLXContext src, GLXContext dst, GLuint mask));
QGL_GLX(Bool, glXSwapBuffers, (Display *dpy, GLXDrawable drawable));
QGL_GLX(Bool, glXQueryVersion, (Display *dpy, int *major, int *minor));
QGL_GLX(const char *, glXQueryExtensionsString, (Display *dpy, int screen));

// GL Functions
QGL_FUNC(void, glAlphaFunc, (GLenum func, GLclampf ref));
QGL_FUNC(void, glArrayElement, (GLint i));
QGL_FUNC(void, glBegin, (GLenum mode));
QGL_FUNC(void, glBindTexture, (GLenum target, GLuint texture));
QGL_FUNC(void, glBlendFunc, (GLenum sfactor, GLenum dfactor));
QGL_FUNC(void, glClear, (GLbitfield mask));
QGL_FUNC(void, glClearColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha));
QGL_FUNC(void, glClearDepth, (GLclampd depth));
QGL_FUNC(void, glClearStencil, (GLint s));
QGL_FUNC(void, glClipPlane, (GLenum plane, const GLdouble *equation));
QGL_FUNC(void, glColor4f, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha));
QGL_FUNC(void, glColor4fv, (const GLfloat *v));
QGL_FUNC(void, glColor4ubv, (const GLubyte *v));
QGL_FUNC(void, glColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha));
QGL_FUNC(void, glColorPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer));
QGL_FUNC(void, glCullFace, (GLenum mode));
QGL_FUNC(void, glGenTextures, (GLsizei n, const GLuint *textures));
QGL_FUNC(void, glDeleteTextures, (GLsizei n, const GLuint *textures));
QGL_FUNC(void, glDepthFunc, (GLenum func));
QGL_FUNC(void, glDepthMask, (GLboolean flag));
QGL_FUNC(void, glDepthRange, (GLclampd zNear, GLclampd zFar));
QGL_FUNC(void, glDisable, (GLenum cap));
QGL_FUNC(void, glDisableClientState, (GLenum array));
QGL_FUNC(void, glDrawBuffer, (GLenum mode));
QGL_FUNC(void, glReadBuffer, (GLenum mode));
QGL_FUNC(void, glDrawElements, (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices));
QGL_FUNC(void, glEnable, (GLenum cap));
QGL_FUNC(void, glEnableClientState, (GLenum array));
QGL_FUNC(void, glEnd, (void));
QGL_FUNC(void, glFinish, (void));
QGL_FUNC(void, glFlush, (void));
QGL_FUNC(void, glFrontFace, (GLenum mode));
QGL_FUNC(GLenum, glGetError, (void));
QGL_FUNC(void, glGetIntegerv, (GLenum pname, GLint *params));
QGL_FUNC(const GLubyte *, glGetString, (GLenum name));
QGL_FUNC(void, glLoadIdentity, (void));
QGL_FUNC(void, glLoadMatrixf, (const GLfloat *m));
QGL_FUNC(void, glMatrixMode, (GLenum mode));
QGL_FUNC(void, glNormalPointer, (GLenum type, GLsizei stride, const GLvoid *pointer));
QGL_FUNC(void, glOrtho, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar));
QGL_FUNC(void, glPolygonMode, (GLenum face, GLenum mode));
QGL_FUNC(void, glPolygonOffset, (GLfloat factor, GLfloat units));
QGL_FUNC(void, glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels));
QGL_FUNC(void, glScissor, (GLint x, GLint y, GLsizei width, GLsizei height));
QGL_FUNC(void, glShadeModel, (GLenum mode));
QGL_FUNC(void, glStencilFunc, (GLenum func, GLint ref, GLuint mask));
QGL_FUNC(void, glStencilMask, (GLuint mask));
QGL_FUNC(void, glStencilOp, (GLenum fail, GLenum zfail, GLenum zpass));
QGL_FUNC(void, glTexCoord2f, (GLfloat s, GLfloat t));
QGL_FUNC(void, glTexCoordPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer));
QGL_FUNC(void, glTexEnvfv, (GLenum target, GLenum pname, const GLfloat *params));
QGL_FUNC(void, glTexEnvi, (GLenum target, GLenum pname, GLint param));
QGL_FUNC(void, glTexGenfv, (GLenum coord, GLenum pname, const GLfloat *params));
QGL_FUNC(void, glTexGeni, (GLenum coord, GLenum pname, GLint param));
QGL_FUNC(void, glTexImage2D, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels));
QGL_FUNC(void, glTexParameteri, (GLenum target, GLenum pname, GLint param));
QGL_FUNC(void, glTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels));
QGL_FUNC(void, glCopyTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height));
QGL_FUNC(void, glVertex2f, (GLfloat x, GLfloat y));
QGL_FUNC(void, glVertex3f, (GLfloat x, GLfloat y, GLfloat z));
QGL_FUNC(void, glVertex3fv, (const GLfloat *v));
QGL_FUNC(void, glVertexPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer));
QGL_FUNC(void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height));

QGL_FUNC(void, glFogfv, (GLenum pname, const GLfloat *params));
QGL_FUNC(void, glFogiv, (GLenum pname, const GLint *params));
QGL_FUNC(void, glFogi, (GLenum pname, const GLint param));

QGL_FUNC(void, glLightModelf, (GLenum pname, GLfloat param));
QGL_FUNC(void, glLightModelfv, (GLenum pname, const GLfloat *params));
QGL_FUNC(void, glLightModeli, (GLenum pname, GLint param));
QGL_FUNC(void, glLightModeliv, (GLenum pname, const GLint *params));
QGL_FUNC(void, glLightf, (GLenum light, GLenum pname, GLfloat param));
QGL_FUNC(void, glLightfv, (GLenum light, GLenum pname, const GLfloat *params));
QGL_FUNC(void, glLighti, (GLenum light, GLenum pname, GLint param));
QGL_FUNC(void, glLightiv, (GLenum light, GLenum pname, const GLint *params));

QGL_FUNC(void, glPixelStorei, (GLenum pname, GLint param));

QGL_EXT(void, glLockArraysEXT, (int , int ));
QGL_EXT(void, glUnlockArraysEXT, (void));
QGL_EXT(void, glSelectTextureSGIS, (GLenum ));
QGL_EXT(void, glActiveTextureARB, (GLenum ));
QGL_EXT(void, glClientActiveTextureARB, (GLenum ));
QGL_EXT(void, glDrawRangeElementsEXT, (GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *));
QGL_EXT(void, glBindBufferARB, (GLenum target, GLuint buffer));
QGL_EXT(void, glDeleteBuffersARB, (GLsizei n, const GLuint *buffers));
QGL_EXT(void, glGenBuffersARB, (GLsizei n, GLuint *buffers));
QGL_EXT(void, glBufferDataARB, (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage));
QGL_EXT(void, glBufferSubDataARB, (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data));
QGL_EXT(void, glTexImage3D, (GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels));
QGL_EXT(void, glTexSubImage3D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels));

QGL_EXT(void, glDeleteObjectARB, (GLhandleARB obj));
QGL_EXT(GLhandleARB, glGetHandleARB, (GLenum pname));
QGL_EXT(void, glDetachObjectARB, (GLhandleARB containerObj, GLhandleARB attachedObj));
QGL_EXT(GLhandleARB, glCreateShaderObjectARB, (GLenum shaderType));
QGL_EXT(void, glShaderSourceARB, (GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length));
QGL_EXT(void, glCompileShaderARB, (GLhandleARB shaderObj));
QGL_EXT(GLhandleARB, glCreateProgramObjectARB, (void));
QGL_EXT(void, glAttachObjectARB, (GLhandleARB containerObj, GLhandleARB obj));
QGL_EXT(void, glLinkProgramARB, (GLhandleARB programObj));
QGL_EXT(void, glUseProgramObjectARB, (GLhandleARB programObj));
QGL_EXT(void, glValidateProgramARB, (GLhandleARB programObj));
QGL_EXT(void, glUniform1fARB, (GLint location, GLfloat v0));
QGL_EXT(void, glUniform2fARB, (GLint location, GLfloat v0, GLfloat v1));
QGL_EXT(void, glUniform3fARB, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2));
QGL_EXT(void, glUniform4fARB, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3));
QGL_EXT(void, glUniform1iARB, (GLint location, GLint v0));
QGL_EXT(void, glUniform2iARB, (GLint location, GLint v0, GLint v1));
QGL_EXT(void, glUniform3iARB, (GLint location, GLint v0, GLint v1, GLint v2));
QGL_EXT(void, glUniform4iARB, (GLint location, GLint v0, GLint v1, GLint v2, GLint v3));
QGL_EXT(void, glUniform1fvARB, (GLint location, GLsizei count, const GLfloat *value));
QGL_EXT(void, glUniform2fvARB, (GLint location, GLsizei count, const GLfloat *value));
QGL_EXT(void, glUniform3fvARB, (GLint location, GLsizei count, const GLfloat *value));
QGL_EXT(void, glUniform4fvARB, (GLint location, GLsizei count, const GLfloat *value));
QGL_EXT(void, glUniform1ivARB, (GLint location, GLsizei count, const GLint *value));
QGL_EXT(void, glUniform2ivARB, (GLint location, GLsizei count, const GLint *value));
QGL_EXT(void, glUniform3ivARB, (GLint location, GLsizei count, const GLint *value));
QGL_EXT(void, glUniform4ivARB, (GLint location, GLsizei count, const GLint *value));
QGL_EXT(void, glUniformMatrix2fvARB, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value));
QGL_EXT(void, glUniformMatrix3fvARB, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value));
QGL_EXT(void, glUniformMatrix4fvARB, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value));
QGL_EXT(void, glGetObjectParameterfvARB, (GLhandleARB obj, GLenum pname, GLfloat *params));
QGL_EXT(void, glGetObjectParameterivARB, (GLhandleARB obj, GLenum pname, GLint *params));
QGL_EXT(void, glGetInfoLogARB, (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog));
QGL_EXT(void, glGetAttachedObjectsARB, (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj));
QGL_EXT(GLint, glGetUniformLocationARB, (GLhandleARB programObj, const GLcharARB *name));
QGL_EXT(void, glGetActiveUniformARB, (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name));
QGL_EXT(void, glGetUniformfvARB, (GLhandleARB programObj, GLint location, GLfloat *params));
QGL_EXT(void, glGetUniformivARB, (GLhandleARB programObj, GLint location, GLint *params));
QGL_EXT(void, glGetShaderSourceARB, (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source));

QGL_EXT(void, glVertexAttribPointerARB, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer));
QGL_EXT(void, glEnableVertexAttribArrayARB, (GLuint index));
QGL_EXT(void, glDisableVertexAttribArrayARB, (GLuint index));
QGL_EXT(void, glBindAttribLocationARB, (GLhandleARB programObj, GLuint index, const GLcharARB *name));
QGL_EXT(void, glGetActiveAttribARB, (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name));
QGL_EXT(GLint, glGetAttribLocationARB, (GLhandleARB programObj, const GLcharARB *name));

QGL_EXT(void, glGenQueriesARB, (GLsizei n, GLuint *ids));
QGL_EXT(void, glDeleteQueriesARB, (GLsizei n, const GLuint *ids));
QGL_EXT(GLboolean, glIsQueryARB, (GLuint id));
QGL_EXT(void, glBeginQueryARB, (GLenum target, GLuint id));
QGL_EXT(void, glEndQueryARB, (GLenum target));
QGL_EXT(void, glGetQueryivARB, (GLenum target, GLenum pname, GLint *params));
QGL_EXT(void, glGetQueryObjectivARB, (GLuint id, GLenum pname, GLint *params));
QGL_EXT(void, glGetQueryObjectuivARB, (GLuint id, GLenum pname, GLuint *params));

QGL_EXT(void, glDrawArraysInstancedARB, (GLenum mode, GLint first, GLsizei count, GLsizei primcount));
QGL_EXT(void, glDrawElementsInstancedARB, (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount));

QGL_EXT(GLboolean, glIsRenderbufferEXT, (GLuint));
QGL_EXT(void, glBindRenderbufferEXT ,(GLenum, GLuint));
QGL_EXT(void, glDeleteRenderbuffersEXT, (GLsizei, const GLuint *));
QGL_EXT(void, glGenRenderbuffersEXT, (GLsizei, GLuint *));
QGL_EXT(void, glRenderbufferStorageEXT, (GLenum, GLenum, GLsizei, GLsizei));
QGL_EXT(void, glGetRenderbufferParameterivEXT, (GLenum, GLenum, GLint *));
QGL_EXT(GLboolean, glIsFramebufferEXT, (GLuint));
QGL_EXT(void, glBindFramebufferEXT, (GLenum, GLuint));
QGL_EXT(void, glDeleteFramebuffersEXT, (GLsizei, const GLuint *));
QGL_EXT(void, glGenFramebuffersEXT, (GLsizei, GLuint *));
QGL_EXT(GLenum, glCheckFramebufferStatusEXT, (GLenum));
QGL_EXT(void, glFramebufferTexture1DEXT, (GLenum, GLenum, GLenum, GLuint, GLint));
QGL_EXT(void, glFramebufferTexture2DEXT, (GLenum, GLenum, GLenum, GLuint, GLint));
QGL_EXT(void, glFramebufferTexture3DEXT, (GLenum, GLenum, GLenum, GLuint, GLint, GLint));
QGL_EXT(void, glFramebufferRenderbufferEXT, (GLenum, GLenum, GLenum, GLuint));
QGL_EXT(void, glGetFramebufferAttachmentParameterivEXT, (GLenum, GLenum, GLenum, GLint *));
QGL_EXT(void, glGenerateMipmapEXT, (GLenum));

QGL_EXT(void, glSwapInterval, (int interval));

// WGL_EXT Functions
QGL_WGL_EXT(const char *, wglGetExtensionsStringEXT, (void));
QGL_WGL_EXT(BOOL, wglGetDeviceGammaRamp3DFX, (HDC, WORD *));
QGL_WGL_EXT(BOOL, wglSetDeviceGammaRamp3DFX, (HDC, WORD *));

// GLX_EXT Functions
