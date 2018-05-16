#pragma once

#ifndef GUIDELINELINE_INCLUDED
#define GUIDELINELINE_INCLUDED

// TnzTools includes
#include <tools/assistant.h>

#include <tgl.h>


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

//*****************************************************************************************
//    TGuidelineLineBase definition
//*****************************************************************************************

class DVAPI TGuidelineLineBase : public TGuideline {
public:
  const TPointD p0;
  const TPointD p1;

  TGuidelineLineBase(double magnetism, const TPointD &p0, const TPointD &p1);
  void drawInliniteLine(const TPointD &p0, const TPointD &p1, bool ray, bool active) const;
  static TPointD calcDirection(const TPointD &p0, const TPointD &p1);
  static void truncateInfiniteLine(const TRectD &bounds, TPointD &p0, TPointD &p1);
};


//*****************************************************************************************
//    TGuidelineLine definition
//*****************************************************************************************

class DVAPI TGuidelineLine : public TGuidelineLineBase {
public:
  const TPointD dir;
  const double dist;

  TGuidelineLine(double magnetism, const TPointD &p0, const TPointD &p1);
  TTrackPoint transformPoint(const TTrackPoint &point) const override;
  void draw(bool active) const override;
};


//*****************************************************************************************
//    TGuidelineInfiniteLine definition
//*****************************************************************************************

class DVAPI TGuidelineInfiniteLine : public TGuidelineLineBase {
public:
  const TPointD dir;

  TGuidelineInfiniteLine(double magnetism, const TPointD &p0, const TPointD &p1);
  TTrackPoint transformPoint(const TTrackPoint &point) const override;
  void draw(bool active) const override;
};


//*****************************************************************************************
//    TGuidelineRay definition
//*****************************************************************************************

class DVAPI TGuidelineRay : public TGuidelineLineBase {
public:
  const TPointD dir;

  TGuidelineRay(double magnetism, const TPointD &p0, const TPointD &p1);
  TTrackPoint transformPoint(const TTrackPoint &point) const override;
  void draw(bool active) const override;
};

#endif
