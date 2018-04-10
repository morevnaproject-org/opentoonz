

#include <tools/modifiers/modifiersegmentation.h>
#include <algorithm>


//*****************************************************************************************
//    TModifierSegmentation implementation
//*****************************************************************************************


TModifierSegmentation::TModifierSegmentation(double precision):
  precision(std::max(TTrack::epsilon, precision)),
  precisionSqr(this->precision * this->precision)
{ }


void
TModifierSegmentation::addSegments(
  TTrack &track,
  const TTrackPoint &p0,
  const TTrackPoint &p1,
  int level)
{
  static const int maxRecursion = 10;
  TPointD d = p1.position - p0.position;

  if (level >= maxRecursion || d.x*d.x + d.y*d.y <= precisionSqr) {
    track.push_back(p1);
    return;
  }

  TTrackPoint p = track.modifier->calcPoint(0.5*(p0.originalIndex + p1.originalIndex));
  addSegments(track, p0, p, level + 1);
  addSegments(track, p, p1, level + 1);
}


void
TModifierSegmentation::modifyTrack(
  const TTrackP &track,
  const TInputSavePoint::Holder &savePoint,
  TTrackList &outTracks )
{
  if (!track->handler) {
    track->handler = new TTrackHandler(*track);
    track->handler->tracks.push_back(
      new TTrack(
        new TTrackModifier(*track->handler) ));
  }

  if (!track->changed() || track->handler->tracks.empty())
    return;

  TTrack &subTrack = *track->handler->tracks.front();
  outTracks.push_back(track->handler->tracks.front());

  // remove points
  int start = track->size() - track->pointsAdded;
  if (start < 0) start = 0;
  int subStart = subTrack.floorIndex(subTrack.indexByOriginalIndex(start));
  if (subStart < 0) subStart = 0;
  if (subStart < subTrack.size() && subTrack[subStart].originalIndex + TTrack::epsilon < start)
    ++subStart;

  while(subStart > 0 && subTrack[subStart-1].originalIndex + TTrack::epsilon >= start)
    --subStart;
  if (subStart < subTrack.size()) {
    subTrack.pointsRemoved += subTrack.size() - subStart;
    subTrack.truncate(subStart);
  }

  // add points
  TTrackPoint p0 = subTrack.modifier->calcPoint(start - 1);
  for(int i = start; i < track->size(); ++i) {
    TTrackPoint p1 = subTrack.modifier->calcPoint(i);
    addSegments(subTrack, p0, p1);
    p0 = p1;
  }
  subTrack.pointsAdded += subTrack.size() - subStart;

  track->pointsRemoved = 0;
  track->pointsAdded = 0;
}
