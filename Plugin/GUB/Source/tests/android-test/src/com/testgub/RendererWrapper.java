package com.testgub;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLSurfaceView.Renderer;

import org.freedesktop.gstreamer.GStreamer;

public class RendererWrapper implements Renderer {
	static {
    System.loadLibrary("gstreamer_android");
    System.loadLibrary("testGUB-Android");
  }

  private final Context context;

  public RendererWrapper(Context context) {
    set_gst_pipeline_env();
    this.context = context;
    try
    {
      GStreamer.init(this.context);
    }
    catch(Exception e)
    {
      System.out.println("GStreamer unsuccessful initialization");
    }
  }

  @Override
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    on_surface_created();
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    on_surface_changed(width, height);
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    on_draw_frame();
  }

  private static native void set_gst_pipeline_env();
  private static native void on_surface_created();
  private static native void on_surface_changed(int width, int height);
  private static native void on_draw_frame();
}
