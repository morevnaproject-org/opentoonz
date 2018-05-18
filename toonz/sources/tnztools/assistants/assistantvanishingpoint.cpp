

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
public:
  const TStringId m_idPassThrough;
  const TStringId m_idGrid;
  const TStringId m_idPerspective;

protected:
  TAssistantPoint &m_center;
  TAssistantPoint &m_a0;
  TAssistantPoint &m_a1;
  TAssistantPoint &m_b0;
  TAssistantPoint &m_b1;
  TAssistantPoint &m_grid0;
  TAssistantPoint &m_grid1;

public:
  TAssistantVanishingPoint(TMetaObject &object):
    TAssistant(object),
    m_idPassThrough("passThrough"),
    m_idGrid("grid"),
    m_idPerspective("perspective"),
    m_center( addPoint("center", TAssistantPoint::CircleCross) ),
    m_a0    ( addPoint("A0",     TAssistantPoint::Circle, TPointD(-50.0, 0.0)) ),
    m_a1    ( addPoint("A1",     TAssistantPoint::Circle, TPointD(-75.0, 0.0)) ),
    m_b0    ( addPoint("B0",     TAssistantPoint::Circle, TPointD( 50.0, 0.0)) ),
    m_b1    ( addPoint("B1",     TAssistantPoint::Circle, TPointD( 75.0, 0.0)) ),
    m_grid0 ( addPoint("grid0",  TAssistantPoint::CircleGrid) ),
    m_grid1 ( addPoint("grid1",  TAssistantPoint::CircleGrid) )
  {
    addProperty( new TBoolProperty(m_idPassThrough.str(), getPassThrough()) );
    addProperty( new TBoolProperty(m_idGrid.str(), getGrid()) );
    addProperty( new TBoolProperty(m_idPerspective.str(), getPerspective()) );
  }

  static QString getLocalName()
    { return tr("Vanishing Point"); }

  void updateTranslation() const {
    setTranslation(m_idPassThrough, tr("Pass Through"));
    setTranslation(m_idGrid, tr("Grid"));
    setTranslation(m_idPerspective, tr("Perspective"));
  }


  inline bool getPassThrough() const
    { return data()[m_idPassThrough].getBool(); }
  inline bool getGrid() const
    { return data()[m_idGrid].getBool(); }
  inline bool getPerspective() const
    { return data()[m_idPerspective].getBool(); }

  void onDataChanged(const TVariant &value) {
    TAssistant::onDataChanged(value);
    m_grid0.visible = m_grid1.visible = ((const TVariant&)data())[m_idGrid].getBool();
  }

private:
  void fixCenter() {
    if ( !(m_a0.position == m_a1.position)
      && !(m_b0.position == m_b1.position) )
    {
      const TPointD &a = m_a0.position;
      const TPointD &b = m_b0.position;
      const TPointD da = m_a1.position - a;
      const TPointD db = m_b1.position - b;
      const TPointD ab = b - a;
      double k = db.x*da.y - db.y*da.x;
      if (fabs(k) > TConsts::epsilon) {
        double lb = (da.x*ab.y - da.y*ab.x)/k;
        m_center.position.x = lb*db.x + b.x;
        m_center.position.y = lb*db.y + b.y;
      }
    }
  }

  void fixSidePoint(TAssistantPoint &p0, TAssistantPoint &p1, TPointD previousP0) {
    if (p0.position != m_center.position && p0.position != p1.position) {
      TPointD dp0 = p0.position - m_center.position;
      TPointD dp1 = p1.position - previousP0;
      double l0 = norm(dp0);
      double l1 = norm(dp1);
      if (l0 > TConsts::epsilon && l0 + l1 > TConsts::epsilon)
        p1.position = m_center.position + dp0*((l0 + l1)/l0);
    }
  }

  void fixSidePoint(TAssistantPoint &p0, TAssistantPoint &p1)
    { fixSidePoint(p0, p1, p0.position); }

public:
  void onFixPoints() override {
    fixSidePoint(m_a0, m_a1);
    fixSidePoint(m_b0, m_b1);
    fixCenter();
  }

  void onMovePoint(TAssistantPoint &point, const TPointD &position) override {
    if (&point == &getBasePoint())
      { move(position); return; }

    TPointD previous = point.position;
    point.position = position;
    if (&point == &m_a0) {
      fixSidePoint(m_a0, m_a1, previous);
      fixSidePoint(m_b0, m_b1);
    } else
    if (&point == &m_b0) {
      fixSidePoint(m_a0, m_a1);
      fixSidePoint(m_b0, m_b1, previous);
    } else
    if (&point == &m_a1) {
      fixCenter();
      fixSidePoint(m_a0, m_a1);
      fixSidePoint(m_b0, m_b1);
    } else
    if (&point == &m_b1) {
      fixCenter();
      fixSidePoint(m_b0, m_b1);
      fixSidePoint(m_a0, m_a1);
    }
  }

  void getGuidelines(
    const TPointD &position,
    const TAffine &toTool,
    TGuidelineList &outGuidelines ) const override
  {
    if (getPassThrough()) {
      outGuidelines.push_back(TGuidelineP(
        new TGuidelineInfiniteLine(
          getEnabled(),
          getMagnetism(),
          toTool * m_center.position,
          position )));
    } else {
      outGuidelines.push_back(TGuidelineP(
        new TGuidelineRay(
          getEnabled(),
          getMagnetism(),
          toTool * m_center.position,
          position )));
    }
  }

  void draw(TToolViewer *viewer, bool enabled) const override {
    double pixelSize = sqrt(tglGetPixelSize2());
    const TPointD &p = m_center.position;
    TPointD dx(20.0*pixelSize, 0.0);
    TPointD dy(0.0, 10.0*pixelSize);
    drawSegment(p-dx-dy, p+dx+dy, pixelSize, enabled);
    drawSegment(p-dx+dy, p+dx-dy, pixelSize, enabled);
  }

  void drawEdit(TToolViewer *viewer) const override {
    double pixelSize = sqrt(tglGetPixelSize2());
    drawSegment(m_center.position, m_a1.position, pixelSize);
    drawSegment(m_center.position, m_b1.position, pixelSize);
    drawDot(m_a0.position);
    drawDot(m_a1.position);
    drawDot(m_b0.position);
    drawDot(m_b1.position);
    TAssistant::drawEdit(viewer);
  }
};


//*****************************************************************************************
//    Registration
//*****************************************************************************************

static TAssistantTypeT<TAssistantVanishingPoint> assistantVanishingPoint("assistantVanishingPoint");
