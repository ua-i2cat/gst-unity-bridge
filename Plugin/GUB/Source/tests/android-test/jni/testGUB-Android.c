#include <GLES2/gl2.h>
#include <android/log.h>
#include <stdbool.h>
#include <stdlib.h>

#include <gub_pipeline.h>
#include <gub_graphics.h>
#include <gub_gstreamer.h>
#include <gub_log.h>

#include <gst/net/gstnet.h>

#include "testGUB-Android.h"

static GLuint texture;
static GLuint buffer;
static GLuint program;

static GLint a_position_location;
static GLint a_texture_coordinates_location;
static GLint u_texture_unit_location;

static int textureWidth;
static int textureHeight;
static int screenWidth;
static int screenHeight;

static GUBPipeline *pipeline = NULL;

static const bool       SYNC_CLOCK            = false;
static const gchar     *SERVER_CLOCK_ADDRESS  = "192.168.53.190";
static const int        SERVER_CLOCK_PORT     = 5001;
static const guint64    WAIT_FOR_SYNC_SEC     = 30;
static const guint64    SHIFT_VIDEO_SEC       = 50;

static const GLfloat vVertices[] = {
  -1.f,  1.f,   0.f, 0.f,
  -1.f, -1.f,   0.f, 1.f,
   1.f,  1.f,   1.f, 0.f,
   1.f, -1.f,   1.f, 1.f
};

// http://server.immersiatv.eu/public_http/dev/contents/0.5/test_sync/videoplayback.mp4
// http://server.immersiatv.eu/public_http/dev/contents/ibc/dragon/tv_tv_12/tv_tv_12.mpd
// http://server.immersiatv.eu/public_http/dev/contents/ibc/dragon/sp_trans_omni_0_1_2_5/sp_trans_omni_0_1_2_5.mpd
// http://server.immersiatv.eu/public_http/releases/0.2/contents/directiveeffect/DEMO2_1.mp4
// http://avena2.man.poznan.pl/video4k/szymon/big_buck_bunny_1080p_h264.mov

static int create_program();
static GLuint load_shader(GLenum type, const char *shaderSrc);
static void logfunction(gint32 level, const char *message);

void set_gst_pipeline_env()
{
  setenv("GST_DEBUG_DUMP_DOT_DIR", "/sdcard/", 1);
}

void on_surface_created()
{
  if(pipeline)
  {
    gub_pipeline_destroy(pipeline);
    gub_unref();
    UnitySetGraphicsDevice(NULL, kUnityGfxRendererOpenGLES20, kUnityGfxDeviceEventShutdown);
    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &buffer);
  }

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  textureWidth = 0;
  textureHeight = 0;
  buffer = 0;
  texture = 0;

  gub_ref("3");
  UnitySetGraphicsDevice(NULL, kUnityGfxRendererOpenGLES20, kUnityGfxDeviceEventInitialize);
  GUBUnityDebugLogPFN logger = logfunction;
  gub_log_set_unity_handler(logger);
  pipeline = (GUBPipeline*)(gub_pipeline_create("TestPipeline", NULL, NULL, NULL, NULL));

  gchar *server_clock_address = NULL;
  guint64 basetime = 0;
  if(SYNC_CLOCK)
  {
    GstClock *net_clock = gst_net_client_clock_new("net_clock", SERVER_CLOCK_ADDRESS, SERVER_CLOCK_PORT, 0);
    gst_clock_wait_for_sync(net_clock, WAIT_FOR_SYNC_SEC * GST_SECOND);
    if(gst_clock_is_synced(net_clock))
    {
      __android_log_print(ANDROID_LOG_ERROR, "TEST_GUB", "\nSynchronized to network clock\n\n");
      GstClockTime server_basetime = gst_clock_get_time(net_clock);
      if (server_basetime != GST_CLOCK_TIME_NONE)
      {
        server_clock_address = (char*)SERVER_CLOCK_ADDRESS;
        basetime = (guint64)server_basetime - (SHIFT_VIDEO_SEC * GST_SECOND);
        __android_log_print(ANDROID_LOG_ERROR, "TEST_GUB", "Synchronized to %s:%d with basetime = %llu ns, shift vidoe by %llu seconds.\n\n", server_clock_address, SERVER_CLOCK_PORT, server_basetime, SHIFT_VIDEO_SEC);
      }
    }
    else
    {
      __android_log_print(ANDROID_LOG_ERROR, "TEST_GUB", "\nCould not synchronize to network clock\n\n");
    }
    gst_object_unref(net_clock);
  }

	gub_pipeline_setup_decoding_clock(pipeline, "http://server.immersiatv.eu/public_http/dev/contents/ibc/dragon/tv_tv_12/tv_tv_12.mpd", 0, -1, server_clock_address, SERVER_CLOCK_PORT, basetime, 0,0,0,0, FALSE);
  gub_pipeline_play(pipeline);

  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), vVertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  program = create_program();
  a_position_location = glGetAttribLocation(program, "aPosition");
  a_texture_coordinates_location = glGetAttribLocation(program, "aTexCoord");
  u_texture_unit_location = glGetUniformLocation(program, "sTexture");
}

void on_surface_changed(int width, int height)
{
  screenWidth = width;
  screenHeight = height;

  __android_log_print(ANDROID_LOG_ERROR, "TEST_GUB", "screenWidth = %d, screenHeight = %d", screenWidth, screenHeight);
}

void on_draw_frame()
{
  int w = 0;
  int h = 0;
  gub_pipeline_grab_frame(pipeline, &w, &h);

  if( w != textureWidth || h != textureHeight)
  {
    textureWidth = w;
    textureHeight = h;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0, 0, screenWidth, screenHeight);

  gub_pipeline_blit_image(pipeline, (void*)texture);

  glUseProgram(program);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glEnableVertexAttribArray(a_position_location);
  glVertexAttribPointer(a_position_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const GLvoid *)(0));
  glEnableVertexAttribArray(a_texture_coordinates_location);
  glVertexAttribPointer(a_texture_coordinates_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const GLvoid *)(2 * sizeof(GLfloat)));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glUniform1i(u_texture_unit_location, 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void logfunction(gint32 level, const char *message)
{
  (void)(level);              // do not print unused variable warning
  printf("%s\n", message);
}

int create_program()
{
  GLbyte vShaderStr[] =
    "attribute vec4 aPosition;    \n"
    "attribute vec2 aTexCoord;    \n"
    "varying vec2 vTexCoord;      \n"
    "void main()                  \n"
    "{                            \n"
    "   gl_Position = aPosition;  \n"
    "   vTexCoord = aTexCoord;    \n"
    "}                            \n";

  GLbyte fShaderStr[] =
    "precision mediump float;                          \n"
    "varying vec2 vTexCoord;                           \n"
    "uniform sampler2D sTexture;                       \n"
    "void main()                                       \n"
    "{                                                 \n"
    "  gl_FragColor = texture2D(sTexture, vTexCoord);  \n"
    "}                                                 \n";

  GLuint vertexShader;
  GLuint fragmentShader;
  GLuint programObject;
  GLint linked;

  vertexShader = load_shader(GL_VERTEX_SHADER, (const char *)vShaderStr);
  fragmentShader = load_shader(GL_FRAGMENT_SHADER, (const char *)fShaderStr);

  programObject = glCreateProgram();

  if(programObject == 0)
  {
    return 0;
  }

  glAttachShader(programObject, vertexShader);
  glAttachShader(programObject, fragmentShader);

  glBindAttribLocation(programObject, 0, "aPosition");
  glBindAttribLocation(programObject, 1, "aTexCoord");

  glLinkProgram(programObject);
  glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

  if(!linked)
  {
    GLint infoLen = 0;
    glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
    if(infoLen > 1)
    {
      char* infoLog = malloc(sizeof(char) * infoLen);
      glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
      gub_log("Error linking program: %s", infoLog);
      free(infoLog);
    }
    glDeleteProgram(programObject);
    return 0;
  }
  return programObject;
}

GLuint load_shader(GLenum type, const char *shaderSrc)
{
  GLuint shader;
  GLint compiled;

  shader = glCreateShader(type);
  if(shader == 0)
  {
    return 0;
  }

  glShaderSource(shader, 1, &shaderSrc, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

  if(!compiled)
  {
    GLint infoLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if(infoLen > 1)
    {
      char* infoLog = malloc(sizeof(char) * infoLen);
      glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
      gub_log("Error compiling shader: %s", infoLog);
      free(infoLog);
    }
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}
