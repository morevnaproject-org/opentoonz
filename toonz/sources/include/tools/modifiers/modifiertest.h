#pragma once

#ifndef MODIFIERTEST_INCLUDED
#define MODIFIERTEST_INCLUDED

// TnzTools includes
#include <tools/inputmanager.h>


#undef DVAPI
#undef DVVAR
#ifdef TNZTOOLS_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif


//===================================================================

//*****************************************************************************************
//    TModifierTest definition
//*****************************************************************************************

class TModifierTest: public TInputModifier {
public:
  class Handler: public TTrackHandler {
  public:
    std::vector<double> angles;
    Handler(TTrack &original): TTrackHandler(original) { }
  };

  class Modifier: public TTrackModifier {
  public:
    double angle;
    double radius;
    double speed;

    Modifier(TTrackHandler &handler, double angle, double radius, double speed = 0.25);
    TTrackPoint calcPoint(double originalIndex) override;
  };

public:
  const int count;
  const double radius;

  TModifierTest();

  void modifyTrack(
    const TTrackP &track,
    const TInputSavePoint::Holder &savePoint,
    TTrackList &outTracks ) override;
};


#endif
