#pragma once

#ifndef ASSISTANTVANISHINGPOINT_INCLUDED
#define ASSISTANTVANISHINGPOINT_INCLUDED

// TnzTools includes
#include <tools/assistant.h>


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
//    TAssistantVanishingPoint definition
//*****************************************************************************************

class DVAPI TAssistantVanishingPoint final : public TAssistant {
public:
  TAssistantVanishingPoint(TMetaObject &object);
  virtual void getGuidelines(const TPointD &position, const TAffine &toTool, TGuidelineList &outGuidelines) const override;
  virtual void draw(TToolViewer *viewer) const override;
};

#endif
