#pragma once

#ifndef MYPAINTBRUSHSTYLE_H
#define MYPAINTBRUSHSTYLE_H

#include "mypaint.h"

// TnzCore includes
#include "imagestyles.h"

#undef DVAPI
#undef DVVAR

#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

//**********************************************************************************
//    TMyPaintBrushStyle declaration
//**********************************************************************************

class DVAPI TMyPaintBrushStyle final : public TColorStyle, TImageStyle {
private:
  TFilePath m_path;
  TFilePath m_fullpath;
  mypaint::Brush m_brush;
  TRasterP m_preview;
  TPixel32 m_color;

  TFilePath decodePath(const TFilePath &path) const;
  void loadBrush(const TFilePath &path);

public:
  TMyPaintBrushStyle();
  TMyPaintBrushStyle(const TFilePath &path);
  TMyPaintBrushStyle(const TMyPaintBrushStyle &other);
  ~TMyPaintBrushStyle();

  TColorStyle *clone() const override
    { return new TMyPaintBrushStyle(*this); }

  TColorStyle &copy(const TColorStyle &other) override;

  static std::string getBrushType();
  static TFilePathSet getBrushesDirs();

  const TFilePath& getPath() const
    { return m_path; }
  const mypaint::Brush& getBrush() const
    { return m_brush; }
  const TRasterP& getPreview() const
    { return m_preview; }

  TStrokeProp* makeStrokeProp(const TStroke * /* stroke */) override
    { return 0; }
  TRegionProp* makeRegionProp(const TRegion * /* region */) override
    { return 0; }
  bool isRegionStyle() const override
    { return false; }
  bool isStrokeStyle() const override
    { return false; }

  bool hasMainColor() const override
    { return true; }
  TPixel32 getMainColor() const override
    { return m_color; }
  void setMainColor(const TPixel32 &color) override
    { m_color = color; }

  int getTagId() const override
    { return 4001; }

  QString getDescription() const override;

protected:
  void makeIcon(const TDimension &d) override;
  void loadData(TInputStreamInterface &) override;
  void saveData(TOutputStreamInterface &) const override;
};

#endif
