

#include "fullcolorbrushtool.h"

// TnzTools includes
#include "tools/tool.h"
#include "tools/cursors.h"
#include "tools/toolutils.h"
#include "tools/toolhandle.h"
#include "tools/tooloptions.h"

#include "mypainttoonzbrush.h"

// TnzQt includes
#include "toonzqt/dvdialog.h"

// TnzLib includes
#include "toonz/tpalettehandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/tobjecthandle.h"
#include "toonz/ttileset.h"
#include "toonz/ttilesaver.h"
#include "toonz/strokegenerator.h"
#include "toonz/tstageobject.h"
#include "toonz/palettecontroller.h"
#include "toonz/mypaintbrushstyle.h"

// TnzCore includes
#include "tgl.h"
#include "tproperty.h"
#include "trasterimage.h"
#include "tenv.h"
#include "tpalette.h"
#include "trop.h"
#include "tstream.h"
#include "tstroke.h"
#include "timagecache.h"
#include "tpixelutils.h"

// Qt includes
#include <QCoreApplication>  // Qt translation support

//----------------------------------------------------------------------------------

TEnv::IntVar FullcolorBrushMinSize("FullcolorBrushMinSize", 1);
TEnv::IntVar FullcolorBrushMaxSize("FullcolorBrushMaxSize", 5);
TEnv::IntVar FullcolorPressureSensitivity("FullcolorPressureSensitivity", 1);
TEnv::DoubleVar FullcolorBrushHardness("FullcolorBrushHardness", 100);
TEnv::DoubleVar FullcolorMinOpacity("FullcolorMinOpacity", 100);
TEnv::DoubleVar FullcolorMaxOpacity("FullcolorMaxOpacity", 100);

//----------------------------------------------------------------------------------

#define CUSTOM_WSTR L"<custom>"

//----------------------------------------------------------------------------------

namespace {

class FullColorBrushUndo final : public ToolUtils::TFullColorRasterUndo {
  TPoint m_offset;
  QString m_id;

public:
  FullColorBrushUndo(TTileSetFullColor *tileSet, TXshSimpleLevel *level,
                     const TFrameId &frameId, bool isFrameCreated,
                     const TRasterP &ras, const TPoint &offset)
      : ToolUtils::TFullColorRasterUndo(tileSet, level, frameId, isFrameCreated,
                                        false, 0)
      , m_offset(offset) {
    static int counter = 0;

    m_id = QString("FullColorBrushUndo") + QString::number(counter++);
    TImageCache::instance()->add(m_id.toStdString(), TRasterImageP(ras));
  }

  ~FullColorBrushUndo() { TImageCache::instance()->remove(m_id); }

  void redo() const override {
    insertLevelAndFrameIfNeeded();

    TRasterImageP image = getImage();
    TRasterP ras        = image->getRaster();

    TRasterImageP srcImg =
        TImageCache::instance()->get(m_id.toStdString(), false);
    ras->copy(srcImg->getRaster(), m_offset);

    TTool::getApplication()->getCurrentXsheet()->notifyXsheetChanged();
    notifyImageChanged();
  }

  int getSize() const override {
    return sizeof(*this) + ToolUtils::TFullColorRasterUndo::getSize();
  }

  QString getToolName() override { return QString("Raster Brush Tool"); }
  int getHistoryType() override { return HistoryType::BrushTool; }
};

}  // namespace

//************************************************************************
//    FullColor Brush Tool implementation
//************************************************************************

FullColorBrushTool::FullColorBrushTool(std::string name)
    : TTool(name)
    , m_thickness("Size", 1, 100, 1, 5, false)
    , m_pressure("Pressure", true)
    , m_opacity("Opacity", 0, 100, 100, 100, true)
    , m_hardness("Hardness:", 0, 100, 100)
    , m_preset("Preset:")
    , m_styleId(0)
    , m_minThick(0)
    , m_maxThick(0)
    , m_toonz_brush(0)
    , m_tileSet(0)
    , m_tileSaver(0)
    , m_notifier(0)
    , m_presetsLoaded(false)
    , m_firstTime(true) {
  bind(TTool::RasterImage | TTool::EmptyTarget);

  m_prop.bind(m_thickness);
  m_prop.bind(m_hardness);
  m_prop.bind(m_opacity);
  m_prop.bind(m_pressure);
  m_prop.bind(m_preset);
  m_preset.setId("BrushPreset");
}

//---------------------------------------------------------------------------------------------------

ToolOptionsBox *FullColorBrushTool::createOptionsBox() {
  TPaletteHandle *currPalette =
      TTool::getApplication()->getPaletteController()->getCurrentLevelPalette();
  ToolHandle *currTool = TTool::getApplication()->getCurrentTool();
  return new BrushToolOptionsBox(0, this, currPalette, currTool);
}

//---------------------------------------------------------------------------------------------------

void FullColorBrushTool::onCanvasSizeChanged() {
  onDeactivate();
  setWorkAndBackupImages();
}

//---------------------------------------------------------------------------------------------------

void FullColorBrushTool::updateTranslation() {
  m_thickness.setQStringName(tr("Size"));
  m_pressure.setQStringName(tr("Pressure"));
  m_opacity.setQStringName(tr("Opacity"));
  m_hardness.setQStringName(tr("Hardness:"));
  m_preset.setQStringName(tr("Preset:"));
}

//---------------------------------------------------------------------------------------------------

void FullColorBrushTool::onActivate() {
  if (!m_notifier) m_notifier = new FullColorBrushToolNotifier(this);

  updateCurrentColor();

  if (m_firstTime) {
    m_firstTime = false;
    m_thickness.setValue(
        TIntPairProperty::Value(FullcolorBrushMinSize, FullcolorBrushMaxSize));
    m_pressure.setValue(FullcolorPressureSensitivity ? 1 : 0);
    m_opacity.setValue(
        TDoublePairProperty::Value(FullcolorMinOpacity, FullcolorMaxOpacity));
    m_hardness.setValue(FullcolorBrushHardness);
  }

  setWorkAndBackupImages();
}

//--------------------------------------------------------------------------------------------------

void FullColorBrushTool::onDeactivate() {
  if (m_mousePressed) leftButtonUp(m_mousePos, m_mouseEvent);
  m_workRaster = TRaster32P();
  m_backUpRas  = TRasterP();
}

//--------------------------------------------------------------------------------------------------

void FullColorBrushTool::updateWorkAndBackupRasters(const TRect &rect) {
  if (rect.isEmpty()) return;

  TRasterImageP ri = TImageP(getImage(false, 1));
  if (!ri) return;

  TRasterP ras = ri->getRaster();

  const int denominator = 8;
  TRect enlargedRect = rect + m_lastRect;
  int dx = (enlargedRect.getLx()-1)/denominator+1;
  int dy = (enlargedRect.getLy()-1)/denominator+1;

  if (m_lastRect.isEmpty()) {
    enlargedRect.x0 -= dx;
    enlargedRect.y0 -= dy;
    enlargedRect.x1 += dx;
    enlargedRect.y1 += dy;

    TRect _rect = enlargedRect*ras->getBounds();
    if (_rect.isEmpty()) return;

    m_workRaster->extract(_rect)->copy(ras->extract(_rect));
    m_backUpRas->extract(_rect)->copy(ras->extract(_rect));
  } else {
    if (enlargedRect.x0 < m_lastRect.x0) enlargedRect.x0 -= dx;
    if (enlargedRect.y0 < m_lastRect.y0) enlargedRect.y0 -= dy;
    if (enlargedRect.x1 > m_lastRect.x1) enlargedRect.x1 += dx;
    if (enlargedRect.y1 > m_lastRect.y1) enlargedRect.y1 += dy;

    TRect _rect = enlargedRect*ras->getBounds();
    if (_rect.isEmpty()) return;

    TRect _lastRect = m_lastRect * ras->getBounds();
    QList<TRect> rects = ToolUtils::splitRect(_rect, _lastRect);
    for (int i = 0; i < rects.size(); i++) {
      m_workRaster->extract(rects[i])->copy(ras->extract(rects[i]));
      m_backUpRas->extract(rects[i])->copy(ras->extract(rects[i]));
    }
  }

  m_lastRect = enlargedRect;
}

//--------------------------------------------------------------------------------------------------

bool FullColorBrushTool::askRead(const TRect &rect) {
  return askWrite(rect);
}

//--------------------------------------------------------------------------------------------------

bool FullColorBrushTool::askWrite(const TRect &rect) {
  if (rect.isEmpty()) return true;
  m_strokeRect += rect;
  m_strokeSegmentRect += rect;
  updateWorkAndBackupRasters(rect);
  m_tileSaver->save(rect);
  return true;
}

//--------------------------------------------------------------------------------------------------

bool FullColorBrushTool::preLeftButtonDown() {
  touchImage();

  if (m_isFrameCreated) setWorkAndBackupImages();

  return true;
}

//---------------------------------------------------------------------------------------------------

void FullColorBrushTool::leftButtonDown(const TPointD &pos,
                                        const TMouseEvent &e) {
  TPointD previousBrushPos = m_brushPos;
  m_brushPos = m_mousePos = pos;
  m_mousePressed          = true;
  m_mouseEvent            = e;
  Viewer *viewer          = getViewer();
  if (!viewer) return;

  TRasterImageP ri = (TRasterImageP)getImage(true);
  if (!ri) ri      = (TRasterImageP)touchImage();

  if (!ri) return;

  /* update color here since the current style might be switched with numpad
   * shortcut keys */
  updateCurrentColor();

  TRasterP ras = ri->getRaster();

  if (!(m_workRaster && m_backUpRas)) setWorkAndBackupImages();

  m_workRaster->lock();

  TPointD rasCenter = ras->getCenterD();
  TPointD point(pos + rasCenter);
  double pressure = e.m_pressure/255.0;

  m_tileSet       = new TTileSetFullColor(ras->getSize());
  m_tileSaver     = new TTileSaverFullColor(ras, m_tileSet);

  mypaint::Brush mypaintBrush;
  applyToonzBrushSettings(mypaintBrush);
  m_toonz_brush = new MyPaintToonzBrush(m_workRaster, *this, mypaintBrush);

  m_strokeRect.empty();
  m_strokeSegmentRect.empty();
  m_toonz_brush->beginStroke();
  m_toonz_brush->strokeTo(point, pressure, restartBrushTimer());
  TRect updateRect = m_strokeSegmentRect*ras->getBounds();
  if (!updateRect.isEmpty())
    ras->extract(updateRect)->copy(m_workRaster->extract(updateRect));

  TPointD thickOffset(m_maxThick*0.5, m_maxThick*0.5);
  TRectD invalidateRect = convert(m_strokeSegmentRect) - rasCenter;
  invalidateRect += TRectD(m_brushPos - thickOffset, m_brushPos + thickOffset);
  invalidateRect += TRectD(previousBrushPos - thickOffset, previousBrushPos + thickOffset);
  invalidate(invalidateRect.enlarge(2.0));
}

//-------------------------------------------------------------------------------------------------------------

void FullColorBrushTool::leftButtonDrag(const TPointD &pos,
                                        const TMouseEvent &e) {
  TPointD previousBrushPos = m_brushPos;
  m_brushPos = m_mousePos = pos;
  m_mouseEvent            = e;
  TRasterImageP ri        = (TRasterImageP)getImage(true);
  if (!ri) return;

  TRasterP ras = ri->getRaster();
  TPointD rasCenter = ras->getCenterD();
  TPointD point(pos + rasCenter);
  double pressure = e.m_pressure/255.0;

  m_strokeSegmentRect.empty();
  m_toonz_brush->strokeTo(point, pressure, restartBrushTimer());
  TRect updateRect = m_strokeSegmentRect*ras->getBounds();
  if (!updateRect.isEmpty())
    ras->extract(updateRect)->copy(m_workRaster->extract(updateRect));

  TPointD thickOffset(m_maxThick*0.5, m_maxThick*0.5);
  TRectD invalidateRect = convert(m_strokeSegmentRect) - rasCenter;
  invalidateRect += TRectD(m_brushPos - thickOffset, m_brushPos + thickOffset);
  invalidateRect += TRectD(previousBrushPos - thickOffset, previousBrushPos + thickOffset);
  invalidate(invalidateRect.enlarge(2.0));
}

//---------------------------------------------------------------------------------------------------------------

void FullColorBrushTool::leftButtonUp(const TPointD &pos,
                                      const TMouseEvent &e) {
  TPointD previousBrushPos = m_brushPos;
  m_brushPos = m_mousePos = pos;

  TRasterImageP ri = (TRasterImageP)getImage(true);
  if (!ri) return;

  TRasterP ras = ri->getRaster();
  TPointD rasCenter = ras->getCenterD();
  TPointD point(pos + rasCenter);
  double pressure = e.m_pressure/255.0;

  m_strokeSegmentRect.empty();
  m_toonz_brush->strokeTo(point, pressure, restartBrushTimer());
  m_toonz_brush->endStroke();
  TRect updateRect = m_strokeSegmentRect*ras->getBounds();
  if (!updateRect.isEmpty())
    ras->extract(updateRect)->copy(m_workRaster->extract(updateRect));

  TPointD thickOffset(m_maxThick*0.5, m_maxThick*0.5);
  TRectD invalidateRect = convert(m_strokeSegmentRect) - rasCenter;
  invalidateRect += TRectD(m_brushPos - thickOffset, m_brushPos + thickOffset);
  invalidateRect += TRectD(previousBrushPos - thickOffset, previousBrushPos + thickOffset);
  invalidate(invalidateRect.enlarge(2.0));

  if (m_toonz_brush) {
    delete m_toonz_brush;
    m_toonz_brush = 0;
  }

  m_lastRect.empty();
  m_workRaster->unlock();

  if (m_tileSet->getTileCount() > 0) {
    delete m_tileSaver;
    TTool::Application *app   = TTool::getApplication();
    TXshLevel *level          = app->getCurrentLevel()->getLevel();
    TXshSimpleLevelP simLevel = level->getSimpleLevel();
    TFrameId frameId          = getCurrentFid();
    TRasterP subras           = ras->extract(m_strokeRect)->clone();
    TUndoManager::manager()->add(
        new FullColorBrushUndo(m_tileSet, simLevel.getPointer(), frameId,
                               m_isFrameCreated, subras, m_strokeRect.getP00()));
  }

  notifyImageChanged();
  m_strokeRect.empty();
  m_mousePressed = false;
}

//---------------------------------------------------------------------------------------------------------------

void FullColorBrushTool::mouseMove(const TPointD &pos, const TMouseEvent &e) {
  struct Locals {
    FullColorBrushTool *m_this;

    void setValue(TIntPairProperty &prop,
                  const TIntPairProperty::Value &value) {
      prop.setValue(value);

      m_this->onPropertyChanged(prop.getName());
      TTool::getApplication()->getCurrentTool()->notifyToolChanged();
    }

    void addMinMax(TIntPairProperty &prop, double add) {
      const TIntPairProperty::Range &range = prop.getRange();

      TIntPairProperty::Value value = prop.getValue();
      value.second =
          tcrop<double>(value.second + add, range.first, range.second);
      value.first = tcrop<double>(value.first + add, range.first, range.second);

      setValue(prop, value);
    }

    void addMinMaxSeparate(TIntPairProperty &prop, double min, double max) {
      if (min == 0.0 && max == 0.0) return;
      const TIntPairProperty::Range &range = prop.getRange();

      TIntPairProperty::Value value = prop.getValue();
      value.first += min;
      value.second += max;
      if (value.first > value.second) value.first = value.second;
      value.first  = tcrop<double>(value.first, range.first, range.second);
      value.second = tcrop<double>(value.second, range.first, range.second);

      setValue(prop, value);
    }

  } locals = {this};

  // if (e.isAltPressed() && !e.isCtrlPressed()) {
  // const TPointD &diff = pos - m_mousePos;
  // double add = (fabs(diff.x) > fabs(diff.y)) ? diff.x : diff.y;

  // locals.addMinMax(m_thickness, int(add));
  //} else
  if (e.isCtrlPressed() && e.isAltPressed()) {
    const TPointD &diff = pos - m_mousePos;
    double max          = diff.x / 2;
    double min          = diff.y / 2;

    locals.addMinMaxSeparate(m_thickness, int(min), int(max));
  } else {
    m_brushPos = pos;
  }

  m_mousePos = pos;
  invalidate();
}

//-------------------------------------------------------------------------------------------------------------

void FullColorBrushTool::draw() {
  if (TRasterImageP ri = TRasterImageP(getImage(false))) {
    TRasterP ras = ri->getRaster();

    glColor3d(1.0, 0.0, 0.0);

    tglDrawCircle(m_brushPos, (m_minThick + 1) * 0.5);
    tglDrawCircle(m_brushPos, (m_maxThick + 1) * 0.5);
  }
}

//--------------------------------------------------------------------------------------------------------------

void FullColorBrushTool::onEnter() {
  TImageP img = getImage(false);
  TRasterImageP ri(img);
  if (ri) {
    m_minThick = m_thickness.getValue().first;
    m_maxThick = m_thickness.getValue().second;
  } else {
    m_minThick = 0;
    m_maxThick = 0;
  }

  updateCurrentColor();
}

//----------------------------------------------------------------------------------------------------------

void FullColorBrushTool::onLeave() {
  m_minThick = 0;
  m_maxThick = 0;
}

//----------------------------------------------------------------------------------------------------------

TPropertyGroup *FullColorBrushTool::getProperties(int targetType) {
  if (!m_presetsLoaded) initPresets();

  return &m_prop;
}

//----------------------------------------------------------------------------------------------------------

void FullColorBrushTool::onImageChanged() { setWorkAndBackupImages(); }

//----------------------------------------------------------------------------------------------------------

void FullColorBrushTool::setWorkAndBackupImages() {
  TRasterImageP ri = (TRasterImageP)getImage(false, 1);
  if (!ri) return;

  TRasterP ras   = ri->getRaster();
  TDimension dim = ras->getSize();

  if (!m_workRaster || m_workRaster->getLx() > dim.lx ||
      m_workRaster->getLy() > dim.ly)
    m_workRaster = TRaster32P(dim);

  if (!m_backUpRas || m_backUpRas->getLx() > dim.lx ||
      m_backUpRas->getLy() > dim.ly ||
      m_backUpRas->getPixelSize() != ras->getPixelSize())
    m_backUpRas = ras->create(dim.lx, dim.ly);

  m_strokeRect.empty();
  m_lastRect.empty();
}

//------------------------------------------------------------------

bool FullColorBrushTool::onPropertyChanged(std::string propertyName) {
  m_minThick = m_thickness.getValue().first;
  m_maxThick = m_thickness.getValue().second;
  if (propertyName == "Hardness:" || propertyName == "Thickness" ||
      propertyName == "Size") {
    TRectD rect(m_brushPos - TPointD(m_maxThick + 2, m_maxThick + 2),
                m_brushPos + TPointD(m_maxThick + 2, m_maxThick + 2));
    invalidate(rect);
  }
  /*if(propertyName == "Hardness:" || propertyName == "Opacity:")
setWorkAndBackupImages();*/
  FullcolorBrushMinSize        = m_minThick;
  FullcolorBrushMaxSize        = m_maxThick;
  FullcolorPressureSensitivity = m_pressure.getValue();
  FullcolorBrushHardness       = m_hardness.getValue();
  FullcolorMinOpacity          = m_opacity.getValue().first;
  FullcolorMaxOpacity          = m_opacity.getValue().second;

  if (propertyName == "Preset:") {
    loadPreset();
    getApplication()->getCurrentTool()->notifyToolChanged();
    return true;
  }

  if (m_preset.getValue() != CUSTOM_WSTR) {
    m_preset.setValue(CUSTOM_WSTR);
    getApplication()->getCurrentTool()->notifyToolChanged();
  }

  return true;
}

//------------------------------------------------------------------

void FullColorBrushTool::initPresets() {
  if (!m_presetsLoaded) {
    // If necessary, load the presets from file
    m_presetsLoaded = true;
    m_presetsManager.load(TEnv::getConfigDir() + "brush_raster.txt");
  }

  // Rebuild the presets property entries
  const std::set<BrushData> &presets = m_presetsManager.presets();

  m_preset.deleteAllValues();
  m_preset.addValue(CUSTOM_WSTR);

  std::set<BrushData>::const_iterator it, end = presets.end();
  for (it = presets.begin(); it != end; ++it) m_preset.addValue(it->m_name);
}

//----------------------------------------------------------------------------------------------------------

void FullColorBrushTool::loadPreset() {
  const std::set<BrushData> &presets = m_presetsManager.presets();
  std::set<BrushData>::const_iterator it;

  it = presets.find(BrushData(m_preset.getValue()));
  if (it == presets.end()) return;

  const BrushData &preset = *it;

  try  // Don't bother with RangeErrors
  {
    m_thickness.setValue(
        TIntPairProperty::Value(std::max((int)preset.m_min, 1), preset.m_max));
    m_hardness.setValue(preset.m_hardness, true);
    m_opacity.setValue(
        TDoublePairProperty::Value(preset.m_opacityMin, preset.m_opacityMax));
    m_pressure.setValue(preset.m_pressure);
  } catch (...) {
  }
}

//------------------------------------------------------------------

void FullColorBrushTool::addPreset(QString name) {
  // Build the preset
  BrushData preset(name.toStdWString());

  preset.m_min        = m_thickness.getValue().first;
  preset.m_max        = m_thickness.getValue().second;
  preset.m_hardness   = m_hardness.getValue();
  preset.m_opacityMin = m_opacity.getValue().first;
  preset.m_opacityMax = m_opacity.getValue().second;
  preset.m_pressure   = m_pressure.getValue();

  // Pass the preset to the manager
  m_presetsManager.addPreset(preset);

  // Reinitialize the associated preset enum
  initPresets();

  // Set the value to the specified one
  m_preset.setValue(preset.m_name);
}

//------------------------------------------------------------------

void FullColorBrushTool::removePreset() {
  std::wstring name(m_preset.getValue());
  if (name == CUSTOM_WSTR) return;

  m_presetsManager.removePreset(name);
  initPresets();

  // No parameter change, and set the preset value to custom
  m_preset.setValue(CUSTOM_WSTR);
}

//------------------------------------------------------------------

void FullColorBrushTool::updateCurrentColor() {
  TTool::Application *app = getApplication();
  if (app->getCurrentObject()->isSpline()) {
    m_currentColor = TPixel32::Red;
    return;
  }
  TPalette *plt = app->getCurrentPalette()->getPalette();
  if (!plt) return;

  int style               = app->getCurrentLevelStyleIndex();
  TColorStyle *colorStyle = plt->getStyle(style);
  m_currentColor          = colorStyle->getMainColor();
}

//------------------------------------------------------------------

double FullColorBrushTool::restartBrushTimer() {
  double dtime = m_brushTimer.nsecsElapsed()*1e-9;
  m_brushTimer.restart();
  return dtime;
}

//------------------------------------------------------------------

void FullColorBrushTool::applyClassicToonzBrushSettings(mypaint::Brush &mypaintBrush) {
  const double precision = 1e-5;

  bool   pressure     = m_pressure.getValue();
  double minThickness = 0.5*m_thickness.getValue().first;
  double maxThickness = 0.5*m_thickness.getValue().second;
  double minOpacity   = 0.01*m_opacity.getValue().first;
  double maxOpacity   = 0.01*m_opacity.getValue().second;
  double hardness     = 0.01*m_hardness.getValue();

  TPixelD color  = PixelConverter<TPixelD>::from(m_currentColor);
  double  colorH = 0.0;
  double  colorS = 0.0;
  double  colorV = 0.0;
  RGB2HSV(color.r, color.g, color.b, &colorH, &colorS, &colorV);

  if (!pressure) {
    minThickness = maxThickness;
    minOpacity = maxOpacity;
  }

  // avoid log(0)
  if (minThickness < precision)
    minThickness = precision;
  if (maxThickness < precision)
    maxThickness = precision;

  mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_HARDNESS, 0.5*hardness + 0.5);
  mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_COLOR_H,  colorH/360.0);
  mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_COLOR_S,  colorS);
  mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_COLOR_V,  colorV);
  mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS, 3.0 + hardness*12.0);

  // thickness may be dynamic
  if (minThickness + precision >= maxThickness) {
    mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC, log(maxThickness));
    mypaintBrush.setMappingN(
        MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC,
        MYPAINT_BRUSH_INPUT_PRESSURE,
        0 );
  } else {
    mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC, 0.0);
    mypaintBrush.setMappingN(
        MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC,
        MYPAINT_BRUSH_INPUT_PRESSURE,
        2 );
    mypaintBrush.setMappingPoint(
        MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC,
        MYPAINT_BRUSH_INPUT_PRESSURE,
        0, 0.0, log(minThickness));
    mypaintBrush.setMappingPoint(
        MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC,
        MYPAINT_BRUSH_INPUT_PRESSURE,
        1, 1.0, log(maxThickness));
  }

  // opacity may be dynamic
  if (minOpacity + precision >= maxOpacity) {
    mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY, maxOpacity);
    mypaintBrush.setMappingN(
        MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY,
        MYPAINT_BRUSH_INPUT_PRESSURE,
        0 );
  } else {
    mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY, 0.0);
    mypaintBrush.setMappingN(
        MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY,
        MYPAINT_BRUSH_INPUT_PRESSURE,
        2 );
    mypaintBrush.setMappingPoint(
        MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY,
        MYPAINT_BRUSH_INPUT_PRESSURE,
        0, 0.0, minOpacity);
    mypaintBrush.setMappingPoint(
        MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY,
        MYPAINT_BRUSH_INPUT_PRESSURE,
        1, 1.0, maxOpacity);
  }
}

void FullColorBrushTool::applyToonzBrushSettings(mypaint::Brush &mypaintBrush) {
  TTool::Application *app = getApplication();
  int styleIndex = app->getCurrentLevelStyleIndex();
  TPalette *plt = app->getCurrentPalette()->getPalette();
  TColorStyle *colorStyle = plt ? plt->getStyle(styleIndex) : 0;
  TMyPaintBrushStyle *mypaintStyle = dynamic_cast<TMyPaintBrushStyle*>(colorStyle);

  if (mypaintStyle) {
    const double precision = 1e-5;

    double maxThickness = 0.5*m_thickness.getValue().second;
    double maxOpacity   = 0.01*m_opacity.getValue().second;

    TPixelD color  = PixelConverter<TPixelD>::from(m_currentColor);
    double  colorH = 0.0;
    double  colorS = 0.0;
    double  colorV = 0.0;
    RGB2HSV(color.r, color.g, color.b, &colorH, &colorS, &colorV);

    // avoid log(0)
    if (maxThickness < precision)
      maxThickness = precision;

    mypaintBrush.fromBrush(mypaintStyle->getBrush());

    mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC, log(maxThickness));
    mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_OPAQUE, maxOpacity);
    mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_COLOR_H,  colorH/360.0);
    mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_COLOR_S,  colorS);
    mypaintBrush.setBaseValue(MYPAINT_BRUSH_SETTING_COLOR_V,  colorV);
  } else {
    applyClassicToonzBrushSettings(mypaintBrush);
  }
}


//==========================================================================================================

FullColorBrushToolNotifier::FullColorBrushToolNotifier(FullColorBrushTool *tool)
    : m_tool(tool) {
  TTool::Application *app = m_tool->getApplication();
  TXshLevelHandle *levelHandle;
  if (app) levelHandle = app->getCurrentLevel();
  bool ret             = false;
  if (levelHandle) {
    bool ret = connect(levelHandle, SIGNAL(xshCanvasSizeChanged()), this,
                       SLOT(onCanvasSizeChanged()));
    assert(ret);
  }
}

//==========================================================================================================

FullColorBrushTool fullColorPencil("T_Brush");
