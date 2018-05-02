
#include <tools/assistant.h>

#include <tgl.h>

#include <limits>


//************************************************************************
//    TGuideline implementation
//************************************************************************

double
TGuideline::calcTrackWeight(const TTrack &track, const TAffine &toScreen, bool &outLongEnough) const {
  outLongEnough = false;
  if (track.size() < 2)
    return std::numeric_limits<double>::infinity();

  const double snapLenght = 20.0;
  const double snapScale = 1.0;
  const double maxLength = 20.0*snapLenght*snapScale;

  double sumWeight = 0.0;
  double sumLength = 0.0;
  double sumDeviation = 0.0;

  TPointD prev = toScreen*track[0].position;
  for(int i = 0; i < track.size(); ++i) {
    const TTrackPoint &tp = track[i];
    TPointD p = toScreen*tp.position;
    double length = tdistance(p, prev);
    sumLength += length;

    double midStepLength = sumLength - 0.5*length;
    if (midStepLength > TTrack::epsilon) {
      double weight = length*logNormalDistribuitionUnscaled(midStepLength, snapLenght, snapScale);
      sumWeight += weight;

      TTrackPoint ntp = transformPoint(tp);
      double deviation = tdistance(toScreen*ntp.position, p);
      sumDeviation += weight*deviation;
    }
    prev = p;

    if (sumLength >= maxLength)
      { outLongEnough = true; break; }
  }
  return sumWeight > TTrack::epsilon
       ? sumDeviation/sumWeight
       : std::numeric_limits<double>::infinity();
}

//---------------------------------------------------------------------------------------------------

TGuidelineP
TGuideline::findBest(const TGuidelineList &guidelines, const TTrack &track, const TAffine &toScreen, bool &outLongEnough) {
  outLongEnough = true;
  double bestWeight = 0.0;
  TGuidelineP best;
  for(TGuidelineList::const_iterator i = guidelines.begin(); i != guidelines.end(); ++i) {
    double weight = (*i)->calcTrackWeight(track, toScreen, outLongEnough);
    if (!best || weight < bestWeight)
      { bestWeight = weight; best = *i; }
  }
  return best;
}


//************************************************************************
//    TAssistant implementation
//************************************************************************

TAssistant::TAssistant(TMetaObject &object):
  TMetaObjectHandler(object),
  m_idPoints("points"),
  m_idX("x"),
  m_idY("y")
{ }

//---------------------------------------------------------------------------------------------------

const TPointD&
TAssistant::blank() {
  static TPointD point;
  return point;
}

//---------------------------------------------------------------------------------------------------

void
TAssistant::fixPoints(int index, const TPointD &position)
  { onFixPoints(); }

//---------------------------------------------------------------------------------------------------

void
TAssistant::movePoint(int index, const TPointD &position)
  { if (index >= 0 && index < (int)m_points.size()) onMovePoint(index, position); }

//---------------------------------------------------------------------------------------------------

void
TAssistant::setPointSelection(int index, bool selected) {
  if (index >= 0 && index < pointsCount())
    m_points[index].selected = selected;
}

//---------------------------------------------------------------------------------------------------

void
TAssistant::onDataChanged(const TVariant &value) {
  const TVariant& pointsData = data()[m_idPoints];
  TVariantPathEntry entry;

  if (&value == &data() || &value == &pointsData)
    onAllDataChanged();
  else
  if (pointsData.getChildPathEntry(value, entry) && entry.isIndex()) {
    const TVariant& pointData = pointsData[entry];
    TPointD position = TPointD(
      pointData[m_idX].getDouble(),
      pointData[m_idY].getDouble() );
    movePoint(entry.index(), position);
  }
}

//---------------------------------------------------------------------------------------------------

void
TAssistant::onAllDataChanged() {
  const TVariant& pointsData = data()[m_idPoints];
  for(int i = 0; i < pointsCount(); ++i) {
    const TVariant& pointData = pointsData[i];
    m_points[i].position = TPointD(
      pointData[m_idX].getDouble(),
      pointData[m_idY].getDouble() );
  }
}

//---------------------------------------------------------------------------------------------------

//! fix positions of all points
void
TAssistant::onFixPoints()
  { }

//---------------------------------------------------------------------------------------------------

void
TAssistant::onMovePoint(int index, const TPointD &position)
  { m_points[index].position = position; }

//---------------------------------------------------------------------------------------------------

void
TAssistant::onFixData() {
  TVariant& pointsData = data()[m_idPoints];
  for(int i = 0; i < pointsCount(); ++i) {
    TVariant& pointData = pointsData[i];
    pointData[m_idX].setDouble( m_points[i].position.x );
    pointData[m_idY].setDouble( m_points[i].position.y );
  }
}

//---------------------------------------------------------------------------------------------------

void
TAssistant::drawPoint(const TAssistantPoint &point, double pixelSize) const {
  double radius = 10.0;
  double crossSize = 1.2*radius;

  double colorBlack[4] = { 0.0, 0.0, 0.0, 0.5 };
  double colorGray[4]  = { 0.5, 0.5, 0.5, 0.5 };
  double colorWhite[4] = { 1.0, 1.0, 1.0, 0.5 };

  if (point.selected) {
    colorBlack[2] = 1.0;
    colorGray[2] = 1.0;
  }

  glPushAttrib(GL_ALL_ATTRIB_BITS);

  tglEnableBlending();
  tglEnableLineSmooth(true, 0.5);

  if (point.type == TAssistantPoint::CircleFill) {
    glColor4dv(colorGray);
    tglDrawDisk(point.position, radius*pixelSize);
  }

  if (point.type == TAssistantPoint::CircleCross) {
    TPointD dp(0.5*pixelSize, 0.5*pixelSize);
    TPointD dx(pixelSize*crossSize, 0.0);
    TPointD dy(0.0, pixelSize*crossSize);

    glColor4dv(colorWhite);
    tglDrawSegment(point.position - dx + dp, point.position + dx + dp);
    tglDrawSegment(point.position - dy + dp, point.position + dy + dp);
    glColor4dv(colorBlack);
    tglDrawSegment(point.position - dx - dp, point.position + dx - dp);
    tglDrawSegment(point.position - dy - dp, point.position + dy - dp);
  }

  glColor4dv(colorWhite);
  tglDrawCircle(point.position, (radius + 0.5)*pixelSize);
  glColor4dv(colorBlack);
  tglDrawCircle(point.position, (radius - 0.5)*pixelSize);

  glPopAttrib();
}

//---------------------------------------------------------------------------------------------------

void
TAssistant::getGuidelines(const TPointD &position, const TAffine &toTool, TGuidelineList &outGuidelines) const
  { }

//---------------------------------------------------------------------------------------------------

void
TAssistant::draw(TToolViewer *viewer) const
  { }

//---------------------------------------------------------------------------------------------------

void
TAssistant::drawEdit(TToolViewer *viewer) const {
  // paint all points
  double pixelSize = sqrt(tglGetPixelSize2());
  for(int i = 0; i < pointsCount(); ++i)
    drawPoint(m_points[i], pixelSize);
}

//---------------------------------------------------------------------------------------------------
