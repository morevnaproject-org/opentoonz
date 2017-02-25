
#include <algorithm>

#include "mypainttoonzbrush.h"
#include "tropcm.h"
#include "tpixelutils.h"
#include <toonz/mypainthelpers.hpp>

#include <QColor>


//=======================================================
//
// Raster32PMyPaintSurface::Internal
//
//=======================================================

class Raster32PMyPaintSurface::Internal:
  public mypaint::helpers::SurfaceCustom<readPixel, writePixel, askRead, askWrite>
{
public:
  typedef SurfaceCustom Parent;
  Internal(Raster32PMyPaintSurface &owner):
    SurfaceCustom( owner.m_ras->pixels(),
                   owner.m_ras->getLx(),
                   owner.m_ras->getLy(),
                   owner.m_ras->getPixelSize(),
                   owner.m_ras->getRowSize(),
                   &owner )
  { }
};

//=======================================================
//
// Raster32PMyPaintSurface
//
//=======================================================

Raster32PMyPaintSurface::Raster32PMyPaintSurface(const TRaster32P &ras):
  m_ras(ras),
  controller(),
  internal()
{
  assert(ras);
  internal = new Internal(*this);
}

Raster32PMyPaintSurface::Raster32PMyPaintSurface(const TRaster32P &ras, RasterController &controller):
  m_ras(ras),
  controller(&controller),
  internal()
{
  assert(ras);
  internal = new Internal(*this);
}

Raster32PMyPaintSurface::~Raster32PMyPaintSurface()
  { delete internal; }

bool Raster32PMyPaintSurface::getColor(float x, float y, float radius,
                                       float &colorR, float &colorG, float &colorB, float &colorA)
{ return internal->getColor(x, y, radius, colorR, colorG, colorB, colorA); }

bool Raster32PMyPaintSurface::drawDab(const mypaint::Dab &dab)
  { return internal->drawDab(dab); }

//=======================================================
//
// MyPaintToonzBrush
//
//=======================================================

MyPaintToonzBrush::MyPaintToonzBrush(const TRaster32P &ras, RasterController &controller, const mypaint::Brush &brush):
  m_ras(ras),
  m_mypaintSurface(m_ras, controller),
  brush(brush)
{ }

void MyPaintToonzBrush::reset()
  { brush.reset(); }

void MyPaintToonzBrush::strokeTo(const TPointD &point, double pressure, double dtime)
  { brush.strokeTo(m_mypaintSurface, point.x, point.y, pressure, 0.f, 0.f, dtime); }

