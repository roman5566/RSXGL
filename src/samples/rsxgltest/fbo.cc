/*
 * rsxgltest - framebuffer object demo. A cube within a cube, maaaaaaan!
 */

#define GL3_PROTOTYPES
#include <GL3/gl3.h>
#include <GL3/gl3ext.h>

#include "rsxgltest.h"
#include "math3d.h"
#include "sine_wave.h"

#include <stddef.h>
#include "fbo_inner_vert.h"
#include "fbo_inner_frag.h"

#include <io/pad.h>

#include <math.h>
#include <Eigen/Geometry>

#include "texture.h"
#include "nagel_bin.h"

const char * rsxgltest_name = "fbo";

struct sine_wave_t rgb_waves[3] = {
  { 0.5f,
    0.5f,
    1.0f
  },
  { 0.5f,
    0.5f,
    1.5f
  },
  { 0.5f,
    0.5f,
    2.5f
  }
};

struct sine_wave_t xyz_waves[3] = {
  { 0.5f,
    0.5f,
    1.0f / 4.0f
  },
  { 0.5f,
    0.5f,
    1.5f / 4.0f
  },
  { 0.5f,
    0.5f,
    2.5f / 4.0f
  }
};

GLuint vao = 0;
GLuint buffers[2] = { 0,0 };

Image image;
GLuint textures[2] = { 0,0 };
GLuint program = 0;

GLuint fbo = 0, rbo[2] = { 0,0 };

GLint ProjMatrix_location = -1, TransMatrix_location = -1, NormalMatrix_location = -1,
  vertex_location = 0, normal_location = 1, tc_location = 2,
  light_location = -1;

const float geometry[] = {
    // -X
    -0.5,-0.5,-0.5,
    -1.0,0.0,0.0,
    0,0,

    -0.5,0.5,-0.5,
    -1.0,0.0,0.0,
    1,0,

    -0.5,0.5,0.5,
    -1.0,0.0,0.0,
    1,1,

    -0.5,-0.5,0.5,
    -1.0,0.0,0.0,
    0,1,

    // +X
    0.5,-0.5,-0.5,
    1.0,0.0,0.0,
    0,0,

    0.5,-0.5,0.5,
    1.0,0.0,0.0,
    1,0,

    0.5,0.5,0.5,
    1.0,0.0,0.0,
    1,1,

    0.5,0.5,-0.5,
    1.0,0.0,0.0,
    0,1,

    // -Y
    -0.5,-0.5,-0.5,
    0.0,-1.0,0.0,
    0,0,

    -0.5,-0.5,0.5,
    0.0,-1.0,0.0,
    1,0,

    0.5,-0.5,0.5,
    0.0,-1.0,0.0,
    1,1,

    0.5,-0.5,-0.5,
    0.0,-1.0,0.0,
    0,1,

    // +Y
    -0.5,0.5,-0.5,
    0.0,1.0,0.0,
    0,0,

    0.5,0.5,-0.5,
    0.0,1.0,0.0,
    1,0,

    0.5,0.5,0.5,
    0.0,1.0,0.0,
    1,1,

    -0.5,0.5,0.5,
    0.0,1.0,0.0,
    0,1,

    // -Z
    -0.5,-0.5,-0.5,
    0.0,0.0,-1.0,
    0,0,

    -0.5,0.5,-0.5,
    0.0,0.0,-1.0,
    1,0,

    0.5,0.5,-0.5,
    0.0,0.0,-1.0,
    1,1,

    0.5,-0.5,-0.5,
    0.0,0.0,-1.0,
    0,1,

    // +Z
    -0.5,-0.5,0.5,
    0.0,0.0,1.0,
    0,0,

    -0.5,0.5,0.5,
    0.0,0.0,1.0,
    1,0,

    0.5,0.5,0.5,
    0.0,0.0,1.0,
    1,1,

    0.5,-0.5,0.5,
    0.0,0.0,1.0,
    0,1
  };

  const GLuint indices[] = {
    // -X
    0, 1, 2,
    2, 3, 0,

    // +X
    4, 5, 6,
    6, 7, 4,

    // -Y
    8, 9, 10,
    10, 11, 8,

    // +Y
    12, 13, 14,
    14, 15, 12,

    // -Z
    16, 17, 18,
    18, 19, 16,

    // +Z
    20, 21, 22,
    22, 23, 20
  };

GLuint * client_indices = 0;

#define DTOR(X) ((X)*0.01745329f)
#define RTOD(d) ((d)*57.295788f)

// Projection matrix for the outer cube:
Eigen::Projective3f ProjMatrix(perspective(DTOR(54.3),1920.0 / 1080.0,0.1,1000.0));

// Projection matrix for the inner cube - the image that gets rendered to the FBO:
Eigen::Projective3f ProjMatrix2(perspective(DTOR(35.3),1920.0 / 1080.0,0.1,1000.0));

Eigen::Affine3f ViewMatrixInv = 
  Eigen::Affine3f(Eigen::Affine3f::Identity() * 
		  (
		   Eigen::Translation3f(1.779,2.221,4.034) *
		   (
		    Eigen::AngleAxisf(DTOR(0),Eigen::Vector3f::UnitZ()) *
		    Eigen::AngleAxisf(DTOR(23.8),Eigen::Vector3f::UnitY()) *
		    Eigen::AngleAxisf(DTOR(-26.738),Eigen::Vector3f::UnitX())
		    )
		   )
		  ).inverse();

const GLfloat light[4] = { 10.0f, 10.0f, 10.0f, 1.0f };

extern "C"
void
rsxgltest_pad(unsigned int,const padData * paddata)
{
  if(paddata -> BTN_UP) {
    tcp_printf("up\n");
  }
  else if(paddata -> BTN_DOWN) {
    tcp_printf("down\n");
  }
}

extern "C"
void
rsxgltest_init(int argc,const char ** argv)
{
  tcp_printf("%s\n",__PRETTY_FUNCTION__);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  // Set up us the program:
  GLuint shaders[2] = { 0,0 };

  {
    shaders[0] = glCreateShader(GL_VERTEX_SHADER);
    shaders[1] = glCreateShader(GL_FRAGMENT_SHADER);
    
    program = glCreateProgram();
    
    glAttachShader(program,shaders[0]);
    glAttachShader(program,shaders[1]);

#if 0    
    // Supply shader binaries:
    glShaderBinary(1,shaders,0,fbo_inner_vpo,fbo_inner_vpo_size);
    glShaderBinary(1,shaders + 1,0,fbo_inner_fpo,fbo_inner_fpo_size);
#endif

    // Supply shader SOURCES!
    const GLchar * shader_srcs[] = { (const GLchar *)fbo_inner_vert, (const GLchar *)fbo_inner_frag };
    GLint shader_srcs_lengths[] = { fbo_inner_vert_len, fbo_inner_frag_len };
    GLint compiled = 0;
    
    glShaderSource(shaders[0],1,shader_srcs,shader_srcs_lengths);
    glCompileShader(shaders[0]);
    
    glGetShaderiv(shaders[0],GL_COMPILE_STATUS,&compiled);
    tcp_printf("shader compile status: %i\n",compiled);
    
    glShaderSource(shaders[1],1,shader_srcs + 1,shader_srcs_lengths + 1);
    glCompileShader(shaders[1]);
    
    glGetShaderiv(shaders[1],GL_COMPILE_STATUS,&compiled);
    tcp_printf("shader compile status: %i\n",compiled);

    //glBindAttribLocation(program,vertex_location,"vertex");
    //glBindAttribLocation(program,normal_location,"normal");
    //glBindAttribLocation(program,tc_location,"uv");
    
    // Link the program for real:
    glLinkProgram(program);
    glValidateProgram(program);
  
    summarize_program("draw",program);

    vertex_location = glGetAttribLocation(program,"vertex");
    normal_location = glGetAttribLocation(program,"normal");
    tc_location = glGetAttribLocation(program,"uv");

    tcp_printf("attrib locations: %i %i %i\n",
	       glGetAttribLocation(program,"vertex"),
	       glGetAttribLocation(program,"normal"),
	       glGetAttribLocation(program,"uv"));

    ProjMatrix_location = glGetUniformLocation(program,"ProjMatrix");
    TransMatrix_location = glGetUniformLocation(program,"TransMatrix");
    NormalMatrix_location = glGetUniformLocation(program,"NormalMatrix");
    GLint texture_location = glGetUniformLocation(program,"texture");
    light_location = glGetUniformLocation(program,"light");

    tcp_printf("vertex_location: %i\n",vertex_location);
    tcp_printf("normal_location: %i\n",normal_location);
    tcp_printf("tc_location: %i\n",tc_location);
    tcp_printf("ProjMatrix_location: %i TransMatrix_location: %i\n",
	       ProjMatrix_location,TransMatrix_location);
    tcp_printf("texture_location: %i\n",texture_location);

    glUseProgram(program);
    
    glUniform1i(texture_location,0);
    glUniform4fv(light_location,1,light);
  }

  // Set up us the vertex data:
  glGenBuffers(2,buffers);

  glBindBuffer(GL_ARRAY_BUFFER,buffers[0]);

  glBufferData(GL_ARRAY_BUFFER,sizeof(float) * 6 * 4 * 8,geometry,GL_STATIC_DRAW);
  glEnableVertexAttribArray(vertex_location);
  glEnableVertexAttribArray(normal_location);
  glEnableVertexAttribArray(tc_location);
  glVertexAttribPointer(vertex_location,3,GL_FLOAT,GL_FALSE,sizeof(float) * 8,0);
  glVertexAttribPointer(normal_location,3,GL_FLOAT,GL_FALSE,sizeof(float) * 8,(const GLvoid *)(sizeof(float) * 3));
  glVertexAttribPointer(tc_location,2,GL_FLOAT,GL_FALSE,sizeof(float) * 8,(const GLvoid *)(sizeof(float) * 6));

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffers[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(GLuint) * 6 * 2 * 3,indices,GL_STATIC_DRAW);

  client_indices = (GLuint *)malloc(sizeof(GLuint) * 6 * 2 * 3);
  memcpy(client_indices,indices,sizeof(GLuint) * 6 * 2 * 3);

  // Texture map:
  image = loadPng(nagel_bin);
  tcp_printf("image size: %u %u\n",image.width,image.height);

  glGenTextures(2,textures);

  // image asset:
  glBindTexture(GL_TEXTURE_2D,textures[0]);

  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image.width,image.height,0,GL_BGRA,GL_UNSIGNED_BYTE,image.data);

  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  // rendering surface:
  glBindTexture(GL_TEXTURE_2D,textures[1]);

  //glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA,image.width,image.height);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image.width,image.height,0,GL_BGRA,GL_UNSIGNED_BYTE,0);

  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  // Framebuffer object:
  glGenRenderbuffers(2,rbo);
  report_glerror("glGenRenderbuffers");

  //glBindRenderbuffer(GL_RENDERBUFFER,rbo[0]);
  //glRenderbufferStorage(GL_RENDERBUFFER,GL_RGBA,image.width,image.height);

  glBindRenderbuffer(GL_RENDERBUFFER,rbo[1]);
  glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT,image.width,image.height);

  glBindRenderbuffer(GL_RENDERBUFFER,0);

  //
  glGenFramebuffers(1,&fbo);

  glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  //glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,rbo[0]);
  //glFramebufferTexture(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,texture,0);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[1],0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,rbo[1]);

  glBindFramebuffer(GL_FRAMEBUFFER,0);

  glBindTexture(GL_TEXTURE_2D,0);
}

extern "C"
int
rsxgltest_draw()
{
  tcp_printf("%s\n",__PRETTY_FUNCTION__);

  float rgb[3] = {
    compute_sine_wave(rgb_waves,rsxgltest_elapsed_time),
    compute_sine_wave(rgb_waves + 1,rsxgltest_elapsed_time),
    compute_sine_wave(rgb_waves + 2,rsxgltest_elapsed_time)
  };

  float xyz[3] = {
    compute_sine_wave(xyz_waves,rsxgltest_elapsed_time),
    compute_sine_wave(xyz_waves + 1,rsxgltest_elapsed_time),
    compute_sine_wave(xyz_waves + 2,rsxgltest_elapsed_time)
  };

  Eigen::Affine3f rotmat = 
    Eigen::Affine3f::Identity() * 
    Eigen::AngleAxisf(DTOR(xyz[2]) * 360.0f,Eigen::Vector3f::UnitZ()) *
    Eigen::AngleAxisf(DTOR(xyz[1]) * 360.0f,Eigen::Vector3f::UnitY()) *
    Eigen::AngleAxisf(DTOR(xyz[0]) * 360.0f,Eigen::Vector3f::UnitX());

  //Eigen::Affine3f modelview = ViewMatrixInv * (Eigen::Affine3f::Identity() * rotmat * Eigen::UniformScaling< float >(3.0));

  Transform3f modelview = ViewMatrixInv * (Transform3f::Identity() * rotmat * Eigen::UniformScaling< float >(3.0));

  glUniformMatrix4fv(TransMatrix_location,1,GL_FALSE,modelview.data());
  
  Eigen::Affine3f normal = Eigen::Affine3f::Identity();
  normal.linear() = modelview.linear().inverse().transpose();
  glUniformMatrix4fv(NormalMatrix_location,1,GL_FALSE,normal.data());

  //
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glViewport(0,0,image.width,image.height);

  glClearColor(1.0 - rgb[0],1.0 - rgb[1],1.0 - rgb[2],1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindTexture(GL_TEXTURE_2D,textures[0]);

  {
    glUniformMatrix4fv(ProjMatrix_location,1,GL_FALSE,ProjMatrix2.data());
    glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,0 /*client_indices*/);
  }

  //
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  glViewport(0,0,rsxgltest_width,rsxgltest_height);

  glClearColor(rgb[0],rgb[1],rgb[2],1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindTexture(GL_TEXTURE_2D,textures[1]);

  {
    glUniformMatrix4fv(ProjMatrix_location,1,GL_FALSE,ProjMatrix.data());
    glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,0 /*client_indices*/);
  }

  return 1;
}

extern "C"
void
rsxgltest_exit()
{
  glBindBuffer(GL_ARRAY_BUFFER,0);

  glVertexAttribPointer(vertex_location,3,GL_FLOAT,GL_FALSE,0,0);
  glVertexAttribPointer(normal_location,3,GL_FLOAT,GL_FALSE,0,0);
  glVertexAttribPointer(tc_location,2,GL_FLOAT,GL_FALSE,0,0);

  glDeleteBuffers(2,buffers);

  glDeleteProgram(program);
}
