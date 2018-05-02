#pragma once

#ifndef MODIFIERASSISTANTS_INCLUDED
#define MODIFIERASSISTANTS_INCLUDED

// TnzTools includes
#include <tools/assistant.h>
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
//    TModifierAssistants definition
//*****************************************************************************************

class TModifierAssistants: public TInputModifier {
public:
  class Modifier: public TTrackModifier {
  public:
    bool initialized;
    TInputSavePoint::Holder savePoint;
    TGuidelineList guidelines;

    Modifier(TTrackHandler &handler);
    TTrackPoint calcPoint(double originalIndex) override;
  };

public:
  const double sensitiveLength;

  TModifierAssistants();

  void findGuidelines(
    const TPointD &position,
    TGuidelineList &outGuidelines ) const;

  void modifyTrack(
    const TTrack &track,
    const TInputSavePoint::Holder &savePoint,
    TTrackList &outTracks ) override;
  void drawHover(const TPointD &hover) override;
  void drawTrack(const TTrack &track) override;
};


#endif
