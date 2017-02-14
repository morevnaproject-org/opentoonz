
#include <algorithm>

#include "mypainttoonzbrush.h"
#include "tropcm.h"
#include "tpixelutils.h"

#include <QColor>


namespace {

static QVector<QRgb> colorTable;

QImage rasterToQImage(const TRasterP &ras, bool premultiplied = false) {
  QImage image;
  if (TRaster32P ras32 = ras)
    image = QImage(ras->getRawData(), ras->getLx(), ras->getLy(),
                   premultiplied ? QImage::Format_ARGB32_Premultiplied
                                 : QImage::Format_ARGB32);
  else if (TRasterGR8P ras8 = ras) {
    image = QImage(ras->getRawData(), ras->getLx(), ras->getLy(),
                   ras->getWrap(), QImage::Format_Indexed8);
    image.setColorTable(colorTable);
  }
  return image;
}

//----------------------------------------------------------------------------------

void putOnRasterCM(const TRasterCM32P &out, const TRaster32P &in, int styleId,
                   bool selective) {
  if (!out.getPointer() || !in.getPointer()) return;
  assert(out->getSize() == in->getSize());
  int x, y;
  if (!selective) {
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
        int tone = sameStyleId ? outPix->getTone() * (255 - inPix->m) / 255
                               : outPix->getTone();
        int ink = !sameStyleId && outPix->getTone() < 255 - inPix->m
                      ? outPix->getInk()
                      : styleId;
        *outPix =
            TPixelCM32(ink, outPix->getPaint(), std::min(255 - inPix->m, tone));
      }
    }
  } else {
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
        int tone = sameStyleId ? outPix->getTone() * (255 - inPix->m) / 255
                               : outPix->getTone();
        int ink = outPix->getTone() < 255 && !sameStyleId &&
                          outPix->getTone() <= 255 - inPix->m
                      ? outPix->getInk()
                      : styleId;
        *outPix =
            TPixelCM32(ink, outPix->getPaint(), std::min(255 - inPix->m, tone));
      }
    }
  }
}

//----------------------------------------------------------------------------------

void eraseFromRasterCM(const TRasterCM32P &out, const TRaster32P &in,
                       bool selective, int selectedStyleId,
                       const std::wstring &mode) {
  if (!out.getPointer() || !in.getPointer()) return;
  assert(out->getSize() == in->getSize());
  bool eraseLine  = mode == L"Lines" || mode == L"Lines & Areas";
  bool eraseAreas = mode == L"Areas" || mode == L"Lines & Areas";
  int x, y;

  for (y = 0; y < out->getLy(); y++) {
    for (x = 0; x < out->getLx(); x++) {
#ifdef _DEBUG
      assert(y >= 0 && y < in->getLy());
      assert(y >= 0 && y < out->getLy());
#endif
      TPixel32 *inPix = &in->pixels(y)[x];
      if (inPix->m == 0) continue;
      TPixelCM32 *outPix = &out->pixels(y)[x];
      bool eraseInk =
          !selective || (selective && selectedStyleId == outPix->getInk());
      bool erasePaint =
          !selective || (selective && selectedStyleId == outPix->getPaint());
      int paint = eraseAreas && erasePaint ? 0 : outPix->getPaint();
      int tone  = inPix->m > 0 && eraseLine && eraseInk
                     ? std::max(outPix->getTone(), (int)inPix->m)
                     : outPix->getTone();
      *outPix = TPixelCM32(outPix->getInk(), paint, tone);
    }
  }
}

//----------------------------------------------------------------------------------

TRasterP rasterFromQImage(
    const QImage &image)  // no need of const& - Qt uses implicit sharing...
{
  QImage::Format format = image.format();
  if (format == QImage::Format_ARGB32 ||
      format == QImage::Format_ARGB32_Premultiplied)
    return TRaster32P(image.width(), image.height(), image.width(),
                      (TPixelRGBM32 *)image.bits(), false);
  if (format == QImage::Format_Indexed8)
    return TRasterGR8P(image.width(), image.height(), image.bytesPerLine(),
                       (TPixelGR8 *)image.bits(), false);
  return TRasterP();
}
}

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
      float ddx = dx*c + dy*s;
      float ddy = dy*c - dx*s;
      ddy /= aspectRatio;

      float dd = (ddx*ddx + ddy*ddy)/rr;
      float o = dd <= hardness
              ? 1.f + dd*(hardness - 1.f)/hardness
              : hardness + (hardness - dd)/hardness;
      o *= opaque;

      // read pixel
      TPixel32 &pixel = m_ras->pixels(iy)[ix];
      float destR = (float)pixel.r/(float)TPixel32::maxChannelValue;
      float destG = (float)pixel.g/(float)TPixel32::maxChannelValue;
      float destB = (float)pixel.b/(float)TPixel32::maxChannelValue;
      float destA = (float)pixel.m/(float)TPixel32::maxChannelValue;

      { // blend normal and eraze
        float blendNormal = 1.f*(1.f - lockAlpha)*(1.f - colorize);
        float oa = blendNormal*opaque*o;
        float ob = 1.f - oa;
        oa *= alphaEraser;
        destR = oa*colorR + ob*destR;
        destG = oa*colorG + ob*destG;
        destB = oa*colorB + ob*destB;
        destA = oa + ob*destA;
      }

      { // blend lock alpha
        float oa = lockAlpha*opaque*o;
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

        float oa = colorize*opaque*o;
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
  m_rasImage(rasterToQImage(m_ras, false)),
  m_mypaint_surface(m_ras, controller),
  brush(brush)
{
  if (colorTable.isEmpty())
    for(int i = 0; i < 256; ++i)
      colorTable.append(QColor(i, i, i).rgb());
}

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

//----------------------------------------------------------------------------------

void MyPaintToonzBrush::updateDrawing(const TRasterP ras, const TRasterP rasBackup,
                                      const TPixel32 &color, const TRect &bbox) const {
  TRect rasRect    = ras->getBounds();
  TRect targetRect = bbox * rasRect;
  if (targetRect.isEmpty()) return;
  QImage image = rasterToQImage(ras, true);
  QRect qTargetRect(targetRect.x0, targetRect.y0, targetRect.getLx(),
                    targetRect.getLy());

  QImage app(qTargetRect.size(), QImage::Format_ARGB32_Premultiplied);
  QPainter p2(&app);
  p2.setBrush(QColor(color.r, color.g, color.b));
  p2.drawRect(app.rect().adjusted(-1, -1, 0, 0));
  p2.setCompositionMode(QPainter::CompositionMode_DestinationIn);
  p2.drawImage(QPoint(), m_rasImage, qTargetRect);
  p2.end();

  if (ras->getPixelSize() == 4) {
    QPainter p(&image);
    p.setClipRect(qTargetRect);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(qTargetRect, rasterToQImage(rasBackup, true), qTargetRect);
    p.end();

    p.begin(&image);
    p.drawImage(qTargetRect, app, app.rect());
    p.end();
  } else {
    QImage targetImage = rasterToQImage(rasBackup).copy(qTargetRect);
    targetImage        = targetImage.convertToFormat(
        QImage::Format_ARGB32_Premultiplied, colorTable);

    QPainter p(&targetImage);
    p.drawImage(QPoint(), app, app.rect());
    p.end();
    targetImage =
        targetImage.convertToFormat(QImage::Format_Indexed8, colorTable);

    TRasterGR8P targetRas = rasterFromQImage(targetImage);
    ras->copy(targetRas, targetRect.getP00());
  }
}

//----------------------------------------------------------------------------------

void MyPaintToonzBrush::eraseDrawing(const TRasterP ras, const TRasterP rasBackup,
                                     const TRect &bbox, double opacity) const {
  if (!ras) return;

  TRect rasRect    = ras->getBounds();
  TRect targetRect = bbox * rasRect;
  if (targetRect.isEmpty()) return;
  QRect qTargetRect(targetRect.x0, targetRect.y0, targetRect.getLx(),
                    targetRect.getLy());
  if (ras->getPixelSize() == 4) {
    QImage image = rasterToQImage(ras, true);
    QPainter p(&image);
    p.setClipRect(qTargetRect);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(qTargetRect, rasterToQImage(rasBackup, true), qTargetRect);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    p.setOpacity(opacity);
    p.drawImage(qTargetRect, m_rasImage, qTargetRect);
    p.end();
  } else if (ras->getPixelSize() != 4) {
    QImage targetImage = rasterToQImage(rasBackup).copy(qTargetRect);
    targetImage        = targetImage.convertToFormat(
        QImage::Format_ARGB32_Premultiplied, colorTable);

    QImage app(qTargetRect.size(), QImage::Format_ARGB32_Premultiplied);
    QPainter p2(&app);
    p2.setBrush(QColor(255, 255, 255));
    p2.drawRect(app.rect().adjusted(-1, -1, 0, 0));
    p2.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p2.drawImage(QPoint(), m_rasImage, qTargetRect);
    p2.end();

    QPainter p(&targetImage);
    p.setOpacity(opacity);
    p.drawImage(QPoint(), app, app.rect());
    p.end();
    targetImage =
        targetImage.convertToFormat(QImage::Format_Indexed8, colorTable);

    TRasterGR8P targetRas = rasterFromQImage(targetImage);
    ras->copy(targetRas, targetRect.getP00());
  }
}

//----------------------------------------------------------------------------------

void MyPaintToonzBrush::updateDrawing(const TRasterCM32P rasCM,
                                      const TRasterCM32P rasBackupCM,
                                      const TRect &bbox, int styleId,
                                      bool selective) const {
  if (!rasCM) return;

  TRect rasRect    = rasCM->getBounds();
  TRect targetRect = bbox * rasRect;
  if (targetRect.isEmpty()) return;

  rasCM->copy(rasBackupCM->extract(targetRect), targetRect.getP00());
  putOnRasterCM(rasCM->extract(targetRect), m_ras->extract(targetRect), styleId,
                selective);
}

//----------------------------------------------------------------------------------

void MyPaintToonzBrush::eraseDrawing(const TRasterCM32P rasCM,
                                     const TRasterCM32P rasBackupCM,
                                     const TRect &bbox, bool selective,
                                     int selectedStyleId,
                                     const std::wstring &mode) const {
  if (!rasCM) return;

  TRect rasRect    = rasCM->getBounds();
  TRect targetRect = bbox * rasRect;
  if (targetRect.isEmpty()) return;

  rasCM->extract(targetRect)->copy(rasBackupCM->extract(targetRect));
  eraseFromRasterCM(rasCM->extract(targetRect), m_ras->extract(targetRect),
                    selective, selectedStyleId, mode);
}
