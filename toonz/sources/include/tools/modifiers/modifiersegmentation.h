#pragma once

#ifndef MODIFIERSEGMENTATION_INCLUDED
#define MODIFIERSEGMENTATION_INCLUDED

// TnzTools includes
#include <tools/inputmanager.h>

// std includes
#include <algorithm>


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
//    TModifierSegmentation definition
//*****************************************************************************************

class TModifierSegmentation: public TInputModifier {
public:
  const double precision;
  const double precisionSqr;

private:
  void addSegments(TTrack &track, const TTrackPoint &p0, const TTrackPoint &p1, int level = 0);

public:
  TModifierSegmentation(double precision = 1.0);

  void modifyTrack(
    const TTrack &track,
    const TInputSavePoint::Holder &savePoint,
    TTrackList &outTracks ) override;
};


#endif
