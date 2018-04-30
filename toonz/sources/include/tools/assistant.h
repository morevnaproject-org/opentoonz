#pragma once

#ifndef ASSISTANT_INCLUDED
#define ASSISTANT_INCLUDED

// TnzTools includes
#include <tools/track.h>

// TnzLib includes

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
class TGuideline;

typedef TSmartPointerT<TGuideline> TGuidelineP;
typedef std::vector<TGuidelineP> TGuidelineList;

//===================================================================

//*****************************************************************************************
//    TGuideline definition
//*****************************************************************************************

class DVAPI TGuideline final : public TSmartObject {
public:
  virtual TTrackPoint transformPoint(const TTrackPoint &point) const
    { return point; }
  virtual void draw(TToolViewer *viewer, bool active) const
    { }
  void draw(TToolViewer *viewer) const
    { draw(viewer, false); }

  double calcTrackWeight(const TTrack &track, const TAffine &affine) const;
  static TGuidelineP findBest(const TGuidelineList &guidelines, const TTrack &track, const TAffine &affine);
};


//*****************************************************************************************
//    TAssistant definition
//*****************************************************************************************

class DVAPI TAssistant final : public TMetaObjectHandler {
public:
  // TODO: handle data changes

  virtual void getGuidelines(const TPointD &position, TGuidelineList &outGuidelines) { }
  virtual void draw(TToolViewer *viewer) { }
  virtual void drawEdit(TToolViewer *viewer, int currentPointIndex) { }
};


#endif
