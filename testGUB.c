#include <iostream>
#include <string>

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


void log(gint32 level, const char *message)
{
    std::cout << message << "\n";
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   Uint32 rmask = 0xff000000;
   Uint32 gmask = 0x00ff0000;
   Uint32 bmask = 0x0000ff00;
   Uint32 amask = 0x000000ff;
#else
   Uint32 rmask = 0x000000ff;
   Uint32 gmask = 0x0000ff00;
   Uint32 bmask = 0x00ff0000;
   Uint32 amask = 0xff000000;
#endif


int main()
{   
    gst_debug_set_threshold_for_name("*clock", GstDebugLevel::GST_LEVEL_DEBUG);
    SDL_Init(SDL_INIT_EVERYTHING);
        
    
    SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 700, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    //SDL_Surface *screen = SDL_GetWindowSurface(window);
    
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

    gub_ref("*clock:7");
    UnitySetGraphicsDevice(NULL, kUnityGfxRendererOpenGL, kUnityGfxDeviceEventInitialize);
    GUBUnityDebugLogPFN logger = log;
    gub_log_set_unity_handler(logger);
    GUBPipeline *pipeline = reinterpret_cast<GUBPipeline*>(gub_pipeline_create("TestPipeline", NULL, NULL, NULL, NULL));
    gub_pipeline_setup_decoding_clock(pipeline, "http://server.immersiatv.eu/public_http/dev/contents/0.5/test_sync/videoplayback.mp4", 0, -1, "127.0.0.1", 55555, 0, 0,0,0,0, FALSE);
    gub_pipeline_play(pipeline);
    
    while(1)
    {	
	int w = 0;
	int h = 0;    
	gub_pipeline_grab_frame(pipeline, &w, &h);
	
	SDL_Surface *surface = SDL_CreateRGBSurface(0, w, h, 24, rmask, gmask, bmask, amask);
	
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	gub_pipeline_blit_image(pipeline, &texture);
	SDL_FreeSurface(surface);
	
	if(texture != NULL)
	{
	   
	    SDL_RenderDrawLine(renderer, 5, 5, 100, 100);
	    SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
	          SDL_Delay(500);

	    

	}
	SDL_DestroyTexture(texture);
	
    }    

   
    gub_pipeline_destroy(pipeline);
    
    gub_unref();
 
    UnitySetGraphicsDevice(NULL, kUnityGfxRendererOpenGL, kUnityGfxDeviceEventShutdown);
    SDL_Quit();
    return 0;
}
