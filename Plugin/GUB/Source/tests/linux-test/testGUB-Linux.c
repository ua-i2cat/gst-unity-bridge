#include <iostream>
#include <string>
#include <vector>

extern "C"
{
  #include <gub_pipeline.h>
  #include <gub_graphics.h>
  #include <gub_gstreamer.h>
  #include <gub_log.h>
}
#include <SDL2/SDL.h>
#include <GL/glut.h>
#include <SDL2/SDL_video.h>

#include <gst/net/gstnet.h>

void log(gint32 level, const char *message)
{
  std::cout << message << "\n";
}

static const bool         SYNC_CLOCK            = false;
static const std::string  SERVER_CLOCK_ADDRESS  = "192.168.53.190";
static const int          SERVER_CLOCK_PORT     = 5001;
static const int          WAIT_FOR_SYNC_SEC     = 30;
static const guint64      SHIFT_VIDEO_SEC       = 5;

static const int          STREAM_URL_IDX        = 4;

static const std::vector<const gchar*> STREAMS_URL = {
    "http://server.immersiatv.eu/public_http/dev/contents/0.5/test_sync/videoplayback.mp4"                                // 0
  , "http://server.immersiatv.eu/public_http/dev/contents/ibc/dragon/tv_tv_12/tv_tv_12.mpd"                               // 1
  , "http://server.immersiatv.eu/public_http/dev/contents/ibc/dragon/sp_trans_omni_0_1_2_5/sp_trans_omni_0_1_2_5.mpd"     // 2
  , "http://server.immersiatv.eu/public_http/releases/0.2/contents/directiveeffect/DEMO2_1.mp4"                           // 3
  , "http://avena2.man.poznan.pl/video4k/szymon/big_buck_bunny_1080p_h264.mov"                                            // 4
};

int main()
{
  SDL_Init(SDL_INIT_EVERYTHING);

  gub_ref("3");
  UnitySetGraphicsDevice(NULL, kUnityGfxRendererOpenGL, kUnityGfxDeviceEventInitialize);
  GUBUnityDebugLogPFN logger = log;
  gub_log_set_unity_handler(logger);
  GUBPipeline *pipeline = reinterpret_cast<GUBPipeline*>(gub_pipeline_create("TestPipeline", NULL, NULL, NULL, NULL));

  gchar *server_clock_address = NULL;
  guint64 basetime = 0;
  if(SYNC_CLOCK)
  {
    GstClock *net_clock = gst_net_client_clock_new("net_clock", (gchar*)SERVER_CLOCK_ADDRESS.c_str(), SERVER_CLOCK_PORT, 0);
    gst_clock_wait_for_sync(net_clock, WAIT_FOR_SYNC_SEC * GST_SECOND);
    if(gst_clock_is_synced(net_clock))
    {
      printf("\n\nSynchronized to network clock\n\n");
      GstClockTime server_basetime = gst_clock_get_time(net_clock);
      if (server_basetime != GST_CLOCK_TIME_NONE)
      {
        server_clock_address = (gchar*)SERVER_CLOCK_ADDRESS.c_str();
        basetime = (guint64)server_basetime - (SHIFT_VIDEO_SEC * GST_SECOND);
        printf("Synchronized to %s:%d with basetime = %lu ns, shift vidoe by %lu seconds.\n\n", server_clock_address, SERVER_CLOCK_PORT, server_basetime, SHIFT_VIDEO_SEC);
      }
    }
    else
    {
      printf("\n\nCould not synchronize to network clock\n\n");
    }
    gst_object_unref(net_clock);
  }

  gub_pipeline_setup_decoding_clock(pipeline, STREAMS_URL[STREAM_URL_IDX], 0, -1, server_clock_address, SERVER_CLOCK_PORT, basetime, 0,0,0,0, FALSE);
  gub_pipeline_play(pipeline);

  unsigned int width = 0;
  unsigned int height = 0;

  SDL_Window *window = SDL_CreateWindow("testGUB", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (window == nullptr)
  {
    printf("Could not create window: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr)
  {
    printf("Could not create renderer: %s\n", SDL_GetError());
    return 1;
  }

  GLuint texture_map;
  glGenTextures( 1, &texture_map );
  glBindTexture(GL_TEXTURE_2D,texture_map);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  SDL_Event event;
  while(true)
  {
    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT)
    {
      break;
    }

  	int w = 0;
  	int h = 0;
    gub_pipeline_grab_frame(pipeline, &w, &h);

    if( w != width || h != height)
    {
      width = w;
      height = h;
      SDL_SetWindowSize(window, width, height);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (float)width, (float)height, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gub_pipeline_blit_image(pipeline, (void*)texture_map);

    glBindTexture(GL_TEXTURE_2D, texture_map);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
      glTexCoord2f(1.0f, 0.0f); glVertex2f(width, 0.0f);
      glTexCoord2f(1.0f, 1.0f); glVertex2f(width, height);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, height);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    SDL_GL_SwapWindow(window);
    //SDL_Delay(500);
  }

  gub_pipeline_destroy(pipeline);
  gub_unref();
  UnitySetGraphicsDevice(NULL, kUnityGfxRendererOpenGL, kUnityGfxDeviceEventShutdown);

  glDeleteTextures(1, &texture_map);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
