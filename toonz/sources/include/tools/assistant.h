#pragma once

#ifndef ASSISTANT_INCLUDED
#define ASSISTANT_INCLUDED

// TnzTools includes
#include <tools/track.h>

// TnzLib includes

// TnzCore includes
#include <tassistantsimage.h>
#include <tsmartpointer.h>
#include <tgeometry.h>

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
typedef TSmartPointerT<TAssistant> TAssistantP;
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

class DVAPI TAssistant final : public TSmartObject {
public:
  typedef TAssistant* (*Fabric)();
  typedef std::map<std::string, Fabric> Registry;

  template<typename T>
  class Registrator {
  public:
    typedef T Type;
    static TAssistant* fabric() { return new Type(); }
    Registrator(const std::string &name) { getRegistry()[name] = fabric; }
  };

  static Registry& getRegistry();
  static TAssistant* create(const std::string &name);
  static TAssistantP wrap(TAssistantDesc &desc);

  virtual void assign(TAssistantDesc &desc)
    { }
  virtual void onMovePoint(
    TAssistantDesc &desc,
    int index,
    const TPointD &position ) { desc[index] = position; }
  virtual void getGuidelines(
    const TAssistantDesc &desc,
    const TPointD &position,
    TGuidelineList &outGuidelines ) { }
  virtual void draw(
    const TAssistantDesc &desc,
    TToolViewer *viewer ) { }
  virtual void drawEdit(
    const TAssistantDesc &desc,
    TToolViewer *viewer,
    int currentPointIndex ) { }
};


#endif
