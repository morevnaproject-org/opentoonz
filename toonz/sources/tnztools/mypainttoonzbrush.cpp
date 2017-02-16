
#include <algorithm>

#include "mypainttoonzbrush.h"
#include "tropcm.h"
#include "tpixelutils.h"

#include <QColor>


//=======================================================
//
// Raster32PMyPaintSurface
//
//=======================================================

void Raster32PMyPaintSurface::getColor(float x, float y, float radius,
                                       float &colorR, float &colorG, float &colorB, float &colorA)
{
  colorR = 0.f;
  colorG = 0.f;
  colorB = 0.f;
  colorA = 0.f;

  const float precision = 1e-5f;
  int x0 = std::max(0, (int)floor(x - radius - 1.f + precision));
  int x1 = std::min(m_ras->getLx()-1, (int)ceil(x + radius + 1.f - precision));
  int y0 = std::max(0, (int)floor(y - radius - 1.f + precision));
  int y1 = std::min(m_ras->getLy()-1, (int)ceil(y + radius + 1.f - precision));
  if (x0 > x1 || y0 > y1)
    return;
  if (controller && !controller->askRead(TRect(x0, y0, x1, y1)))
    return;

  // TODO: optimizations
  // TODO: antialiasing
  float rr = radius*radius;
  double sumR = 0.0;
  double sumG = 0.0;
  double sumB = 0.0;
  double sumA = 0.0;
  double sumW = 0.0;
  for(int ix = x0; ix <= x1; ++ix) {
    for(int iy = y0; iy <= y1; ++iy) {
      float dx = x - (float)ix;
      float dy = y - (float)iy;
      float w = (dx*dx + dy*dy)/rr;

      TPixel32 &pixel = m_ras->pixels(iy)[ix];
      sumR += w*(float)pixel.r/(float)TPixel32::maxChannelValue;
      sumG += w*(float)pixel.g/(float)TPixel32::maxChannelValue;
      sumB += w*(float)pixel.b/(float)TPixel32::maxChannelValue;
      sumA += w*(float)pixel.m/(float)TPixel32::maxChannelValue;
      sumW += w;
    }
  }

  colorR = (float)(sumR/sumW);
  colorG = (float)(sumG/sumW);
  colorB = (float)(sumB/sumW);
  colorA = (float)(sumA/sumW);
}

//----------------------------------------------------------------------------------

bool Raster32PMyPaintSurface::drawDab(float x, float y, float radius,
                                      float colorR, float colorG, float colorB,
                                      float opaque, float hardness,
                                      float alphaEraser,
                                      float aspectRatio, float angle,
                                      float lockAlpha,
                                      float colorize )
{
  const float precision = 1e-5f;
  int x0 = std::max(0, (int)floor(x - radius - 1.f + precision));
  int x1 = std::min(m_ras->getLx()-1, (int)ceil(x + radius + 1.f - precision));
  int y0 = std::max(0, (int)floor(y - radius - 1.f + precision));
  int y1 = std::min(m_ras->getLy()-1, (int)ceil(y + radius + 1.f - precision));
  if (x0 > x1 || y0 > y1)
    return false;
  if (controller && !controller->askWrite(TRect(x0, y0, x1, y1)))
    return false;

  // TODO: optimizations
  // TODO: antialiasing
  float rr = radius*radius;
  float s  = sinf(angle);
  float c  = cosf(angle);
  for(int ix = x0; ix <= x1; ++ix) {
    for(int iy = y0; iy <= y1; ++iy) {
      float dx = x - (float)ix;
      float dy = y - (float)iy;
      float ddx = -dx*c + dy*s;
      float ddy = -dy*c - dx*s;
      ddy /= aspectRatio;

      float dd = (ddx*ddx + ddy*ddy)/rr;
      float o = dd > 1.f      ? 0.f
              : dd > hardness ? (1.f - dd)*hardness/(1.f - hardness)
              :                 1.f + dd*(hardness - 1.f)/hardness;
      o = std::min(std::max(o, 0.f), 1.f);
      o *= opaque;

      // read pixel
      TPixel32 &pixel = m_ras->pixels(iy)[ix];
      float destR = (float)pixel.r/(float)TPixel32::maxChannelValue;
      float destG = (float)pixel.g/(float)TPixel32::maxChannelValue;
      float destB = (float)pixel.b/(float)TPixel32::maxChannelValue;
      float destA = (float)pixel.m/(float)TPixel32::maxChannelValue;

      //destR *= destA;
      //destG *= destA;
      //destB *= destA;

      { // blend normal and eraze
        float blendNormal = 1.f*(1.f - lockAlpha)*(1.f - colorize);
        float oa = blendNormal*o;
        float ob = 1.f - oa;
        oa *= alphaEraser;
        destR = oa*colorR + ob*destR;
        destG = oa*colorG + ob*destG;
        destB = oa*colorB + ob*destB;
        destA = oa + ob*destA;
      }

      { // blend lock alpha
        float oa = lockAlpha*o;
        float ob = 1.f - oa;
        oa *= destA;
        destR = oa*colorR + ob*destR;
        destG = oa*colorG + ob*destG;
        destB = oa*colorB + ob*destB;
      }

      { // blend color
        const float lr = 0.30f;
        const float lg = 0.59f;
        const float lb = 0.11f;

        float srcLum = colorR*lr + colorG*lg + colorB*lb;
        float destLum = destR*lr + destG*lg + destB*lb;
        float dLum = destLum - srcLum;
        float r = colorR + dLum;
        float g = colorG + dLum;
        float b = colorB + dLum;

        float lum = r*lr + g*lg + b*lb;
        float cmin = std::min(std::min(r, g), b);
        float cmax = std::max(std::max(r, g), b);
        if (cmin < 0.f) {
            r = lum + (r - lum)*lum/(lum - cmin);
            g = lum + (g - lum)*lum/(lum - cmin);
            b = lum + (b - lum)*lum/(lum - cmin);
        }
        if (cmax > 1.f) {
            r = lum + (r - lum)*(1.f-lum)/(cmax - lum);
            g = lum + (g - lum)*(1.f-lum)/(cmax - lum);
            b = lum + (b - lum)*(1.f-lum)/(cmax - lum);
        }

        float oa = colorize*o;
        float ob = 1.f - oa;
        destR = oa*r + ob*destR;
        destG = oa*g + ob*destG;
        destB = oa*b + ob*destB;
      }

      // write pixel
      destR = std::min(std::max(destR, 0.f), 1.f);
      destG = std::min(std::max(destG, 0.f), 1.f);
      destB = std::min(std::max(destB, 0.f), 1.f);
      destA = std::min(std::max(destA, 0.f), 1.f);

      if (destA > precision) {
        //destR /= destA;
        //destG /= destA;
        //destB /= destA;
      }

      pixel.r = (TPixel32::Channel)roundf(destR * TPixel32::maxChannelValue);
      pixel.g = (TPixel32::Channel)roundf(destG * TPixel32::maxChannelValue);
      pixel.b = (TPixel32::Channel)roundf(destB * TPixel32::maxChannelValue);
      pixel.m = (TPixel32::Channel)roundf(destA * TPixel32::maxChannelValue);
    }
  }

  return true;
}

//=======================================================
//
// MyPaintToonzBrush
//
//=======================================================

MyPaintToonzBrush::MyPaintToonzBrush(const TRaster32P &ras, RasterController &controller, const mypaint::Brush &brush):
  m_ras(ras),
  m_mypaint_surface(m_ras, controller),
  brush(brush)
{ }

//----------------------------------------------------------------------------------

MyPaintToonzBrush::~MyPaintToonzBrush() { }

//----------------------------------------------------------------------------------

void MyPaintToonzBrush::reset() {
  brush.reset();
}

//----------------------------------------------------------------------------------

void MyPaintToonzBrush::strokeTo(const TPointD &point, double pressure, double dtime) {
  m_ras->lock();
  brush.strokeTo(m_mypaint_surface, point.x, point.y, pressure, 0.f, 0.f, dtime);
  m_ras->unlock();
}

