#pragma once

#ifndef MYPAINTTOONZBRUSH_H
#define MYPAINTTOONZBRUSH_H

#include <toonz/mypaint.h>
#include "traster.h"
#include "trastercm.h"
#include "tcurves.h"
#include <QPainter>
#include <QImage>


class RasterController {
public:
  virtual ~RasterController() { }
  virtual bool askRead(const TRect &rect) { return true; }
  virtual bool askWrite(const TRect &rect) { return true; }
};

//=======================================================
//
// Raster32PMyPaintSurface
//
//=======================================================

class Raster32PMyPaintSurface: public mypaint::Surface {
private:
  class Internal;

  TRaster32P ras;
  RasterController *controller;
  Internal *internal;

  inline static void readPixel(const void *pixelPtr, float &colorR, float &colorG, float &colorB, float &colorA) {
    const TPixel32 &pixel = *(const TPixel32*)pixelPtr;
    colorR = (float)pixel.r/(float)TPixel32::maxChannelValue;
    colorG = (float)pixel.g/(float)TPixel32::maxChannelValue;
    colorB = (float)pixel.b/(float)TPixel32::maxChannelValue;
    colorA = (float)pixel.m/(float)TPixel32::maxChannelValue;
  }

  inline static void writePixel(void *pixelPtr, float colorR, float colorG, float colorB, float colorA) {
    TPixel32 &pixel = *(TPixel32*)pixelPtr;
    pixel.r = (TPixel32::Channel)roundf(colorR * TPixel32::maxChannelValue);
    pixel.g = (TPixel32::Channel)roundf(colorG * TPixel32::maxChannelValue);
    pixel.b = (TPixel32::Channel)roundf(colorB * TPixel32::maxChannelValue);
    pixel.m = (TPixel32::Channel)roundf(colorA * TPixel32::maxChannelValue);
  }

  inline static bool askRead(void *surfaceController, const void* /* surfacePointer */, int x0, int y0, int x1, int y1) {
    Raster32PMyPaintSurface &owner = *((Raster32PMyPaintSurface*)surfaceController);
    return !owner.controller || owner.controller->askRead(TRect(x0, y0, x1, y1));
  }

  inline static bool askWrite(void *surfaceController, const void* /* surfacePointer */, int x0, int y0, int x1, int y1) {
    Raster32PMyPaintSurface &owner = *((Raster32PMyPaintSurface*)surfaceController);
    return !owner.controller || owner.controller->askWrite(TRect(x0, y0, x1, y1));
  }

public:
  explicit Raster32PMyPaintSurface(const TRaster32P &ras);
  explicit Raster32PMyPaintSurface(const TRaster32P &ras, RasterController &controller);
  ~Raster32PMyPaintSurface();

  bool getColor(float x, float y, float radius,
                float &colorR, float &colorG, float &colorB, float &colorA) override;

  bool drawDab(const mypaint::Dab &dab) override;

  bool getAntialiasing() const;
  void setAntialiasing(bool value);

  RasterController* getController() const
    { return controller; }
};

//=======================================================
//
// MyPaintToonzBrush
//
//=======================================================

class MyPaintToonzBrush {
private:
  TRaster32P ras;
  Raster32PMyPaintSurface mypaintSurface;
  mypaint::Brush brush;
  bool reset;

public:
  MyPaintToonzBrush(
    const TRaster32P &ras,
    RasterController &controller,
    const mypaint::Brush &brush);

  void beginStroke();
  void strokeTo(
    const TPointD &position,
    double pressure,
    const TPointD &tilt,
    double dtime );
  void endStroke();

  const TRaster32P& getRaster() const
    { return ras; }
  RasterController& getController()
    { return *mypaintSurface.getController(); }
  const mypaint::Brush& getBrush() const
    { return brush; }
};

#endif  // T_BLUREDBRUSH
