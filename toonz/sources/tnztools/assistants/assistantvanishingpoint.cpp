

#include "assistantvanishingpoint.h"
#include "guidelineline.h"

// TnzCore includes
#include "tgl.h"



//*****************************************************************************************
//    Registration
//*****************************************************************************************

static TMetaObject::Registrator<TAssistantVanishingPoint> assistantVanishingPoint("assistantVanishingPoint");


//*****************************************************************************************
//    TAssistantVanishingPoint implementation
//*****************************************************************************************

TAssistantVanishingPoint::TAssistantVanishingPoint(TMetaObject &object):
  TAssistant(object)
{
  m_points.push_back(TAssistantPoint(
    TAssistantPoint::CircleCross ));
}

void
TAssistantVanishingPoint::getGuidelines(
  const TPointD &position,
  const TAffine &toTool,
  TGuidelineList &outGuidelines ) const
{
  outGuidelines.push_back(TGuidelineP(
    new TGuidelineInfiniteLine(
      toTool*m_points.front().position,
      position )));
}

void
TAssistantVanishingPoint::draw(TToolViewer *viewer) const {
  double pixelSize = sqrt(tglGetPixelSize2());
  const TPointD &p = m_points.front().position;
  TPointD dx(20.0*pixelSize, 0.0);
  TPointD dy(0.0, 10.0*pixelSize);
  drawSegment(p-dx-dy, p+dx+dy, pixelSize);
  drawSegment(p-dx+dy, p+dx-dy, pixelSize);
}
