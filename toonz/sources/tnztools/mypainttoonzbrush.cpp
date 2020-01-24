
#include <algorithm>

#include "mypainttoonzbrush.h"
#include "tropcm.h"
#include "tpixelutils.h"
#include <toonz/mypainthelpers.hpp>

#include <QColor>

namespace {
void putOnRasterCM(const TRasterCM32P &out, const TRaster32P &in, int styleId) {
  if (!out.getPointer() || !in.getPointer()) return;
  assert(out->getSize() == in->getSize());
  int x, y;
  for (y = 0; y < out->getLy(); y++) {
    for (x = 0; x < out->getLx(); x++) {
#ifdef _DEBUG
      assert(x >= 0 && x < in->getLx());
      assert(y >= 0 && y < in->getLy());
      assert(x >= 0 && x < out->getLx());
      assert(y >= 0 && y < out->getLy());
#endif
      TPixel32 *inPix = &in->pixels(y)[x];
      if (inPix->m == 0) continue;
      TPixelCM32 *outPix = &out->pixels(y)[x];
      bool sameStyleId   = styleId == outPix->getInk();
      // line with the same style : multiply tones
      // line with different style : pick darker tone
      int tone = sameStyleId ? outPix->getTone() * (255 - inPix->m) / 255
                             : std::min(255 - inPix->m, outPix->getTone());
      int ink = !sameStyleId && outPix->getTone() < 255 - inPix->m
                    ? outPix->getInk()
                    : styleId;
      *outPix = TPixelCM32(ink, outPix->getPaint(), tone);
    }
  }
}
}  // namespace

//=======================================================
//
// Raster32PMyPaintSurface::Internal
//
//=======================================================

class Raster32PMyPaintSurface::Internal
    : public mypaint::helpers::SurfaceCustom<readPixel, writePixel, askRead,
                                             askWrite> {
public:
  typedef SurfaceCustom Parent;
  Internal(Raster32PMyPaintSurface &owner)
      : SurfaceCustom(owner.ras->pixels(), owner.ras->getLx(),
                      owner.ras->getLy(), owner.ras->getPixelSize(),
                      owner.ras->getRowSize(), &owner) {}
};

//=======================================================
//
// Raster32PMyPaintSurface
//
//=======================================================

Raster32PMyPaintSurface::Raster32PMyPaintSurface(const TRaster32P &ras)
    : ras(ras), controller(), internal() {
  assert(ras);
  internal = new Internal(*this);
}

Raster32PMyPaintSurface::Raster32PMyPaintSurface(const TRaster32P &ras,
                                                 RasterController &controller)
    : ras(ras), controller(&controller), internal() {
  assert(ras);
  internal = new Internal(*this);
}

Raster32PMyPaintSurface::~Raster32PMyPaintSurface() { delete internal; }

bool Raster32PMyPaintSurface::getColor(float x, float y, float radius,
                                       float &colorR, float &colorG,
                                       float &colorB, float &colorA) {
  return internal->getColor(x, y, radius, colorR, colorG, colorB, colorA);
}

bool Raster32PMyPaintSurface::drawDab(const mypaint::Dab &dab) {
  return internal->drawDab(dab);
}

bool Raster32PMyPaintSurface::getAntialiasing() const {
  return internal->antialiasing;
}

void Raster32PMyPaintSurface::setAntialiasing(bool value) {
  internal->antialiasing = value;
}

//=======================================================
//
// MyPaintToonzBrush
//
//=======================================================

MyPaintToonzBrush::MyPaintToonzBrush(const TRaster32P &ras,
                                     RasterController &controller,
                                     const mypaint::Brush &brush)
    : ras(ras)
    , mypaintSurface(ras, controller)
    , brush(brush)
    , reset(true)
{
  // read brush antialiasing settings
  float aa = this->brush.getBaseValue(MYPAINT_BRUSH_SETTING_ANTI_ALIASING);
  mypaintSurface.setAntialiasing(aa > 0.5f);

  // reset brush antialiasing to zero to avoid radius and hardness correction
  this->brush.setBaseValue(MYPAINT_BRUSH_SETTING_ANTI_ALIASING, 0.f);
  for (int i = 0; i < MYPAINT_BRUSH_INPUTS_COUNT; ++i)
    this->brush.setMappingN(MYPAINT_BRUSH_SETTING_ANTI_ALIASING,
                            (MyPaintBrushInput)i, 0);
}

void MyPaintToonzBrush::beginStroke() {
  brush.reset();
  brush.newStroke();
  reset = true;
}

void MyPaintToonzBrush::endStroke()
  { if (!reset) beginStroke(); }

void MyPaintToonzBrush::strokeTo(
  const TPointD &position,
  double pressure,
  const TPointD &tilt,
  double dtime )
{
  if (reset) {
    reset = false;
    // we need to jump to initial point (heuristic)
    brush.setState(MYPAINT_BRUSH_STATE_X, position.x);
    brush.setState(MYPAINT_BRUSH_STATE_Y, position.y);
    brush.setState(MYPAINT_BRUSH_STATE_ACTUAL_X, position.x);
    brush.setState(MYPAINT_BRUSH_STATE_ACTUAL_Y, position.y);
    return;
  }

  brush.strokeTo(
    mypaintSurface,
    position.x,
    position.y,
    pressure,
    0.0,
    0.0,
    dtime );
}

//----------------------------------------------------------------------------------

void MyPaintToonzBrush::updateDrawing(const TRasterCM32P rasCM,
                                      const TRasterCM32P rasBackupCM,
                                      const TRect &bbox, int styleId) const {
  if (!rasCM) return;

  TRect rasRect    = rasCM->getBounds();
  TRect targetRect = bbox * rasRect;
  if (targetRect.isEmpty()) return;

  rasCM->copy(rasBackupCM->extract(targetRect), targetRect.getP00());
  putOnRasterCM(rasCM->extract(targetRect), ras->extract(targetRect),
                styleId);
}
