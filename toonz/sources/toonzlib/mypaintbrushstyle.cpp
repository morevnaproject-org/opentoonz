
#include <streambuf>

#include <QStandardPaths>

#include "tfilepath_io.h"
#include "timage_io.h"
#include "trop.h"
#include "tsystem.h"
#include "tvectorimage.h"
#include "toonz/toonzscene.h"

#include "toonz/mypaintbrushstyle.h"


//*************************************************************************************
//    TMyPaintBrushStyle  implementation
//*************************************************************************************

TMyPaintBrushStyle::TMyPaintBrushStyle()
  { }

//-----------------------------------------------------------------------------

TMyPaintBrushStyle::TMyPaintBrushStyle(const TFilePath &path) {
  loadBrush(path);
}

//-----------------------------------------------------------------------------

TMyPaintBrushStyle::TMyPaintBrushStyle(const TMyPaintBrushStyle &other):
  TColorStyle(other),
  m_path     (other.m_path),
  m_fullpath (other.m_fullpath),
  m_brush    (other.m_brush),
  m_preview  (other.m_preview),
  m_color    (other.m_color)
  { }


//-----------------------------------------------------------------------------

TMyPaintBrushStyle::~TMyPaintBrushStyle()
  { }

//-----------------------------------------------------------------------------

TColorStyle& TMyPaintBrushStyle::copy(const TColorStyle &other) {
  const TMyPaintBrushStyle *otherBrushStyle =
    dynamic_cast<const TMyPaintBrushStyle*>(&other);
  if (otherBrushStyle) {
    m_path     = otherBrushStyle->m_path;
    m_fullpath = otherBrushStyle->m_fullpath;
    m_brush    = otherBrushStyle->m_brush;
    m_preview  = otherBrushStyle->m_preview;
  }
  assignBlend(other, other, 0.0);
  return *this;
}

//-----------------------------------------------------------------------------

QString TMyPaintBrushStyle::getDescription() const
  { return "MyPaintBrushStyle"; }

//-----------------------------------------------------------------------------

std::string TMyPaintBrushStyle::getBrushType()
  { return "myb"; }

//-----------------------------------------------------------------------------

TFilePathSet TMyPaintBrushStyle::getBrushesDirs() {
  TFilePathSet paths;
  paths.push_back(m_libraryDir + "brushes");
  QStringList genericLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
  for(QStringList::iterator i = genericLocations.begin(); i != genericLocations.end(); ++i)
    paths.push_back(TFilePath(*i) + "mypaint" + "brushes");
  return paths;
}

//-----------------------------------------------------------------------------

TFilePath TMyPaintBrushStyle::decodePath(const TFilePath &path) const {
  if (path.isAbsolute())
    return path;

  if (m_currentScene) {
    TFilePath p = m_currentScene->decodeFilePath(path);
    TFileStatus fs(p);
    if (fs.doesExist() && !fs.isDirectory())
      return p;
  }

  TFilePathSet paths = getBrushesDirs();
  for(TFilePathSet::iterator i = paths.begin(); i != paths.end(); ++i) {
    TFilePath p = *i + path;
    TFileStatus fs(p);
    if (fs.doesExist() && !fs.isDirectory())
      return p;
  }

  return path;
}

//-----------------------------------------------------------------------------

void TMyPaintBrushStyle::loadBrush(const TFilePath &path) {
  m_path = path;
  m_fullpath = decodePath(path);
  m_brush.fromDefaults();

  Tifstream is(m_fullpath);
  if (is) {
    std::string str;
    str.assign( std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>() );
    m_brush.fromString(str);
  }

  TFilePath preview_path = m_fullpath.getParentDir() + (m_fullpath.getWideName() + L"_prev.png");
  TImageReader::load(preview_path, m_preview);

  invalidateIcon();
}

//-----------------------------------------------------------------------------

void TMyPaintBrushStyle::makeIcon(const TDimension &d) {
  TFilePath path = m_fullpath.getParentDir() + (m_fullpath.getWideName() + L"_prev.png");
  if (!m_preview) {
    m_icon = TRaster32P(d);
    m_icon->fill(TPixel32::Red);
  } else
  if (m_preview->getSize() == d) {
    m_icon = m_preview;
  } else {
    m_icon = TRaster32P(d);
    double sx = (double)d.lx/(double)m_preview->getLx();
    double sy = (double)d.ly/(double)m_preview->getLy();
    TRop::resample(m_icon, m_preview, TScale(sx, sy));
  }
}

//------------------------------------------------------------

void TMyPaintBrushStyle::loadData(TInputStreamInterface &is) {
  std::string path;
  is >> path;
  is >> m_color;
  loadBrush(TFilePath(path));
}

//-----------------------------------------------------------------------------

void TMyPaintBrushStyle::saveData(TOutputStreamInterface &os) const {
  std::wstring wstr = m_path.getWideString();
  std::string str;
  str.assign(wstr.begin(), wstr.end());
  os << str;
  os << m_color;
}

//-----------------------------------------------------------------------------

namespace {
  TColorStyle::Declaration mypaintBrushStyle(new TMyPaintBrushStyle());
}
