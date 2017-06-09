package com.testgub;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.widget.Toast;
import android.view.WindowManager;

public class TestGUB extends Activity
{
  private GLSurfaceView glSurfaceView;
  private boolean rendererSet;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    ActivityManager activityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
    ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();

    final boolean supportsEs2 = configurationInfo.reqGlEsVersion >= 0x20000;

    if (supportsEs2)
    {
      glSurfaceView = new GLSurfaceView(this);
      glSurfaceView.setEGLContextClientVersion(2);
      glSurfaceView.setRenderer(new RendererWrapper(this));
      rendererSet = true;
      setContentView(glSurfaceView);
    }
    else
    {
      Toast.makeText(this, "This device does not support OpenGL ES 2.0.", Toast.LENGTH_LONG).show();
      return;
    }
  }

  @Override
  protected void onPause()
  {
    super.onPause();
    if (rendererSet)
    {
      glSurfaceView.onPause();
    }
  }

  @Override
  protected void onResume()
  {
    super.onResume();
    if (rendererSet)
    {
      glSurfaceView.onResume();
    }
  }
}
