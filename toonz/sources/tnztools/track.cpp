

#include <tools/track.h>


//*****************************************************************************************
//    Class definitions
//*****************************************************************************************

const double TTrack::epsilon = 1e-9;
TTrack::Id TTrack::lastId = 0;

DEFINE_CLASS_CODE(TTrackHandler, 130)
DEFINE_CLASS_CODE(TTrackModifier, 131)
DEFINE_CLASS_CODE(TTrack, 132)


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
  DeviceId deviceId,
  TouchId touchId,
  const TKeyHistoryT<Qt::Key>::Holder &keyHistory,
  const TKeyHistoryT<Qt::MouseButton>::Holder &buttonHistory
):
  id(++lastId),
  deviceId(deviceId),
  touchId(touchId),
  keyHistory(keyHistory),
  buttonHistory(buttonHistory),
  wayPointsRemoved(),
  wayPointsAdded()
  { }

TTrack::TTrack(const TTrackModifierP &modifier):
  id(++lastId),
  deviceId(modifier->original.deviceId),
  touchId(modifier->original.touchId),
  keyHistory(modifier->original.keyHistory),
  buttonHistory(modifier->original.buttonHistory),
  wayPointsRemoved(),
  wayPointsAdded()
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
  points_.push_back(point);
  if (size() == 1) return;

  const TTrackPoint &prev = *(points_.end() - 2);
  TTrackPoint &p = points_.back();

  if (p.originalIndex < prev.originalIndex)
      p.originalIndex = prev.originalIndex;
  if (p.time < prev.time)
      p.time = prev.time;

  TPointD d = p.position - prev.position;
  p.length = prev.length + sqrt(d.x*d.x + d.y*d.y);
}

void
TTrack::pop_back(int count) {
  if (count > (int)size()) count = size();
  if (count <= 0) return;
  points_.erase(points_.end() - count, points_.end());
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
