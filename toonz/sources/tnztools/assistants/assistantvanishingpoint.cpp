

#include "guidelineline.h"

// TnzTools includes
#include <tools/assistant.h>

// TnzCore includes
#include <tgl.h>


//*****************************************************************************************
//    TAssistantVanishingPoint implementation
//*****************************************************************************************

class DVAPI TAssistantVanishingPoint final : public TAssistant {
  Q_DECLARE_TR_FUNCTIONS(TAssistantVanishingPoint)
protected:
  TAssistantPoint &m_pointCenter;

public:
  TAssistantVanishingPoint(TMetaObject &object):
    TAssistant(object),
    m_pointCenter( addPoint("center", TAssistantPoint::CircleCross) )
  { }

  static QString getLocalName()
    { return tr("Vanishing Point"); }

  void getGuidelines(
    const TPointD &position,
    const TAffine &toTool,
    TGuidelineList &outGuidelines ) const override
  {
    outGuidelines.push_back(TGuidelineP(
      new TGuidelineInfiniteLine(
        getEnabled(),
        getMagnetism(),
        toTool * m_pointCenter.position,
        position )));
  }

  void draw(TToolViewer *viewer, bool enabled) const override {
    double pixelSize = sqrt(tglGetPixelSize2());
    const TPointD &p = m_pointCenter.position;
    TPointD dx(20.0*pixelSize, 0.0);
    TPointD dy(0.0, 10.0*pixelSize);
    drawSegment(p-dx-dy, p+dx+dy, pixelSize, enabled);
    drawSegment(p-dx+dy, p+dx-dy, pixelSize, enabled);
  }
};

//*****************************************************************************************
//    Registration
//*****************************************************************************************

static TAssistantTypeT<TAssistantVanishingPoint> assistantVanishingPoint("assistantVanishingPoint");
