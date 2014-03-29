#ifndef INCLUDE_GL_H
#define INCLUDE_GL_H

#ifdef CHOWDREN_USE_GL
#include <GL/glew.h>
#include <SDL_opengl.h>
#endif

#ifdef CHOWDREN_USE_GLES1
#include <SDL_opengles.h>
#endif

#ifdef CHOWDREN_USE_GLES2
#include <SDL_opengles2.h>
#endif

#ifdef CHOWDREN_IS_WIIU
#include "wiiu_glc.h"
#endif

void glc_enable(GLenum cap);
void glc_disable(GLenum cap);
void glc_begin(GLenum mode);
void glc_ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
               GLfloat near, GLfloat far);
void glc_load_identity();
void glc_color_4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glc_color_4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glc_matrix_mode(GLenum mode);
void glc_translate_3f(GLfloat x, GLfloat y, GLfloat z);
void glc_scale_f(GLfloat x, GLfloat y, GLfloat z);
void glc_push_matrix();
void glc_pop_matrix();
void glc_vertex_3f(GLfloat x, GLfloat y, GLfloat z);
void glc_end();
void glc_multi_texcoord_2f(GLenum target, GLfloat s, GLfloat t);
void glc_texcoord_2f(GLfloat s, GLfloat t);
void glc_rotate_f(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);

#ifndef CHOWDREN_BUILD_GLC

#ifdef CHOWDREN_USE_GL
#define glGenFramebuffers glGenFramebuffersEXT
#define glBindFramebuffer glBindFramebufferEXT
#define glFramebufferTexture2D glFramebufferTexture2DEXT
#define GL_FRAMEBUFFER GL_FRAMEBUFFER_EXT
#define GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_EXT
#endif

#ifdef CHOWDREN_USE_GLES1
#define glGenFramebuffers glGenFramebuffersOES
#define glBindFramebuffer glGenFramebuffersOES
#define glFramebufferTexture2D glFramebufferTexture2DOES
#define glOrtho glOrthof
#define glMultiTexCoord2f(target, s, t) glMultiTexCoord4f(target, s, t, 0, 1)
// XXX still need shaders for this
// #define glCreateProgram glCreateProgram
// #define glLinkProgram glLinkProgram
// #define glGetProgramiv glGetProgramiv
// #define glDetachShader glDetachShader
// #define glUseProgram glUseProgram
// #define glUniform1i glUniform1i
#endif

#ifdef CHOWDREN_USE_GLES2
#define glPushMatrix glc_push_matrix
#define glMatrixMode glc_matrix_mode
#define glDisable glc_disable
#define glEnable glc_enable
#define glPopMatrix glc_pop_matrix
#define glLoadIdentity glc_load_identity
#define glColor4ub glc_color_4ub
#define glColor4f glc_color_4f
#define glOrtho glc_ortho
#define glTranslatef glc_translate_3f
#define glTranslated glc_translate_3f
#define glScalef glc_scale_f
#define glScaled glc_scale_f
#define glRotated glc_rotate_f
#endif

#if defined(CHOWDREN_USE_GLES1) || defined(CHOWDREN_USE_GLES2)
#define glBegin glc_begin
#define glEnd glc_end
#define glVertex2f(x, y) glc_vertex_3f(x, y, 0.0f)
#define glVertex3f glc_vertex_3f
#define glVertex2i(x, y) glc_vertex_3f(x, y, 0.0f)
#define glVertex2d(x, y) glc_vertex_3f(x, y, 0.0f)
#define glMultiTexCoord2f glc_multi_texcoord_2f
#define glTexCoord2f glc_texcoord_2f
#define GL_CLAMP GL_CLAMP_TO_EDGE
#endif

#define glPopAttrib glPopAttribUndef
#define glPushAttrib glPushAttribUndef

#endif // CHOWDREN_BUILD_GLC

#ifdef CHOWDREN_USE_GLES2
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_QUADS 0x0007
#endif

#endif // INCLUDE_GL_H
