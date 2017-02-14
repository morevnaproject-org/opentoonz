#pragma once

#ifndef MYPAINTTOONZBRUSH_H
#define MYPAINTTOONZBRUSH_H

#include "traster.h"
#include "trastercm.h"
#include "tcurves.h"
#include <QPainter>
#include <QImage>

#include "mypaint.hpp"

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
  TRaster32P m_ras;
  RasterController *controller;
public:
  explicit Raster32PMyPaintSurface(const TRaster32P &ras):
    m_ras(ras), controller() { }

  explicit Raster32PMyPaintSurface(const TRaster32P &ras, RasterController &controller):
    m_ras(ras), controller(&controller) { }

  void getColor(float x, float y, float radius,
                float &colorR, float &colorG, float &colorB, float &colorA) override;

  bool drawDab(float x, float y, float radius,
               float colorR, float colorG, float colorB,
               float opaque, float hardness,
               float alphaEraser,
               float aspectRatio, float angle,
               float lockAlpha,
               float colorize) override;
};

//=======================================================
//
// MyPaintToonzBrush
//
//=======================================================

class MyPaintToonzBrush {
  TRaster32P m_ras;
  QImage m_rasImage;
  Raster32PMyPaintSurface m_mypaint_surface;
  mypaint::Brush brush;

  double getNextPadPosition(const TThickQuadratic &q, double t) const;

public:
  MyPaintToonzBrush(const TRaster32P &ras, RasterController &controller, const mypaint::Brush &brush);
  ~MyPaintToonzBrush();

  void reset();
  void strokeTo(const TPointD &p, double pressure, double dtime);

  // colormapped
  void updateDrawing(const TRasterCM32P rasCM, const TRasterCM32P rasBackupCM,
                     const TRect &bbox, int styleId, bool selective) const;
  void eraseDrawing(const TRasterCM32P rasCM, const TRasterCM32P rasBackupCM,
                    const TRect &bbox, bool selective, int selectedStyleId,
                    const std::wstring &mode) const;

  // fullcolor
  void updateDrawing(const TRasterP ras, const TRasterP rasBackup,
                     const TPixel32 &color, const TRect &bbox) const;
  void eraseDrawing(const TRasterP ras, const TRasterP rasBackup,
                    const TRect &bbox, double opacity) const;
};

#endif  // T_BLUREDBRUSH
