#pragma once

#ifndef ASSISTANT_INCLUDED
#define ASSISTANT_INCLUDED

// TnzTools includes
#include <tools/track.h>

// TnzCore includes
#include <tsmartpointer.h>
#include <tgeometry.h>
#include <tmetaimage.h>

// std includes
#include <vector>
#include <string>
#include <map>


#undef DVAPI
#undef DVVAR
#ifdef TNZTOOLS_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif


//==============================================================

//  Forward declarations

class TToolViewer;
class TAssistant;
class TAssistantPoint;
class TGuideline;

typedef TSmartPointerT<TGuideline> TGuidelineP;
typedef std::vector<TGuidelineP> TGuidelineList;
typedef std::vector<TAssistantPoint> TAssistantPointList;

//===================================================================

//*****************************************************************************************
//    TGuideline definition
//*****************************************************************************************

class DVAPI TGuideline : public TSmartObject {
public:
  virtual TTrackPoint transformPoint(const TTrackPoint &point) const
    { return point; }
  virtual void draw(bool active) const
    { }
  void draw() const
    { draw(false); }

  void drawSegment(const TPointD &p0, const TPointD &p1, double pixelSize, bool active) const;

  double calcTrackWeight(const TTrack &track, const TAffine &toScreen, bool &outLongEnough) const;
  static TGuidelineP findBest(const TGuidelineList &guidelines, const TTrack &track, const TAffine &toScreen, bool &outLongEnough);
};


//*****************************************************************************************
//    TAssistantPoint definition
//*****************************************************************************************

class DVAPI TAssistantPoint {
public:
  enum Type {
    Circle,
    CircleFill,
    CircleCross
  };

  Type type;
  TPointD position;
  mutable bool selected;
  double radius;

  explicit TAssistantPoint(Type type = Circle, const TPointD &position = TPointD());
  TAssistantPoint(Type type, const TPointD &position, double radius);
};

//*****************************************************************************************
//    TAssistant definition
//*****************************************************************************************

class DVAPI TAssistant : public TMetaObjectHandler {
protected:
  const TStringId m_idPoints;
  const TStringId m_idX;
  const TStringId m_idY;

  TAssistantPointList m_points;

public:
  TAssistant(TMetaObject &object);

  static const TPointD& blank();

  inline const TAssistantPointList& points() const
    { return m_points; }
  inline const int pointsCount() const
    { return (int)m_points.size(); }

  void fixPoints();
  void movePoint(int index, const TPointD &position);
  void setPointSelection(int index, bool selected) const;

  inline void selectPoint(int index) const
    { setPointSelection(index, true); }
  inline void deselectPoint(int index) const
    { setPointSelection(index, false); }
  inline void selectAll() const
    { for(int i = 0; i < pointsCount(); ++i) setPointSelection(i, false); }
  inline void deselectAll() const
    { for(int i = 0; i < pointsCount(); ++i) setPointSelection(i, false); }

protected:
  //! called when part of variant data changed
  void onDataChanged(const TVariant &value) override;
  //! load object data from variant
  virtual void onAllDataChanged();
  //! fix positions of all points (as like as all points moved)
  virtual void onFixPoints();
  //! try to move point
  virtual void onMovePoint(int index, const TPointD &position);
  //! save object data to variant
  virtual void onFixData();

  void drawSegment(const TPointD &p0, const TPointD &p1, double pixelSize) const;
  void drawPoint(const TAssistantPoint &point, double pixelSize) const;

public:
  virtual void getGuidelines(const TPointD &position, const TAffine &toTool, TGuidelineList &outGuidelines) const;
  virtual void draw(TToolViewer *viewer) const;
  virtual void drawEdit(TToolViewer *viewer) const;
};

#endif
