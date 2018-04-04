

#include <tools/track.h>


//*****************************************************************************************
//    Static fields
//*****************************************************************************************

const double TTrack::epsilon = 1e-9;
TTrack::Id TTrack::m_lastId = 0;


//*****************************************************************************************
//    TTrackModifier implemantation
//*****************************************************************************************

TTrackPoint
TTrackModifier::calcPoint(double originalIndex) {
  TTrackPoint p = original.calcPoint(originalIndex);
  p.originalIndex = originalIndex;
  return p;
}


//*****************************************************************************************
//    TTrack implemantation
//*****************************************************************************************

TTrack::TTrack(
  TInputState::DeviceId deviceId,
  TInputState::TouchId touchId,
  const TInputState::KeyHistory::Holder &keyHistory,
  const TInputState::ButtonHistory::Holder &buttonHistory,
  bool hasPressure,
  bool hasTilt
):
  id(++m_lastId),
  deviceId(deviceId),
  touchId(touchId),
  keyHistory(keyHistory),
  buttonHistory(buttonHistory),
  hasPressure(hasPressure),
  hasTilt(hasTilt),
  pointsRemoved(),
  pointsAdded()
  { }

TTrack::TTrack(const TTrackModifierP &modifier):
  id(++m_lastId),
  deviceId(modifier->original.deviceId),
  touchId(modifier->original.touchId),
  keyHistory(modifier->original.keyHistory),
  buttonHistory(modifier->original.buttonHistory),
  hasPressure(modifier->original.hasPressure),
  hasTilt(modifier->original.hasTilt),
  pointsRemoved(),
  pointsAdded()
  { }

const TTrack*
TTrack::root() const
  { return original() ? original()->root() : this; }

TTrack*
TTrack::root()
  { return original() ? original()->root() : this; }

int
TTrack::level() const
  { return original() ? original()->level() + 1 : 0; }

int
TTrack::floorIndex(double index, double &outFrac) const {
  int i = (int)floor(index + epsilon);
  if (i > size() - 1)
    { outFrac = 0.0; return size() - 1; }
  if (i < 0)
    { outFrac = 0.0; return 0; }
  outFrac = std::max(0.0, index - (double)i);
  return i;
}

void
TTrack::push_back(const TTrackPoint &point) {
  m_points.push_back(point);
  if (size() == 1) return;

  const TTrackPoint &prev = *(m_points.rbegin() + 1);
  TTrackPoint &p = m_points.back();

  // fix originalIndex
  if (p.originalIndex < prev.originalIndex)
      p.originalIndex = prev.originalIndex;

  // fix time
  p.time = std::max(p.time, prev.time + TToolTimer::step);

  // calculate length
  TPointD d = p.position - prev.position;
  p.length = prev.length + sqrt(d.x*d.x + d.y*d.y);
}

void
TTrack::pop_back(int count) {
  if (count > (int)size()) count = size();
  if (count <= 0) return;
  m_points.erase(m_points.end() - count, m_points.end());
}


TTrackPoint
TTrack::calcPoint(double index) const {
  return modifier
       ? modifier->calcPoint( originalIndexByIndex(index) )
       : interpolateLinear(index);
}

TPointD
TTrack::calcTangent(double index, double distance) const {
  double minDistance = 10.0*epsilon;
  if (distance < minDistance) distance = minDistance;
  TTrackPoint p = calcPoint(index);
  TTrackPoint pp = calcPoint(indexByLength(p.length - distance));
  TPointD dp = p.position - pp.position;
  double lenSqr = dp.x*dp.x + dp.y*dp.y;
  return lenSqr > epsilon*epsilon ? dp*sqrt(1.0/lenSqr) : TPointD();
}

double
TTrack::rootIndexByIndex(double index) const {
  return modifier
       ? modifier->original.rootIndexByIndex( originalIndexByIndex(index) )
       : index;
}

TTrackPoint
TTrack::calcRootPoint(double index) const {
  return modifier
       ? modifier->original.calcRootPoint( originalIndexByIndex(index) )
       : calcPoint(index);
}
