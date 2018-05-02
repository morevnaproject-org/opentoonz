

#include <tools/modifiers/modifiertangents.h>


//*****************************************************************************************
//    TModifierTangents::Modifier implementation
//*****************************************************************************************


TTrackPoint
TModifierTangents::Modifier::calcPoint(double originalIndex) {
  double frac;
  int i0 = original.floorIndex(originalIndex, &frac);
  int i1 = original.ceilIndex(originalIndex);
  TTrackPoint p = i0 < 0 ? TTrackPoint()
    : TTrack::interpolationSpline(
        original[i0],
        original[i1],
        i0 < (int)tangents.size() ? tangents[i0] : TTrackTangent(),
        i1 < (int)tangents.size() ? tangents[i1] : TTrackTangent(),
        frac );
  p.originalIndex = originalIndex;
  return p;
}


//*****************************************************************************************
//    TModifierTangents implementation
//*****************************************************************************************


void
TModifierTangents::modifyTrack(
  const TTrack &track,
  const TInputSavePoint::Holder &savePoint,
  TTrackList &outTracks )
{
  if (!track.handler) {
    track.handler = new TTrackHandler(track);
    track.handler->tracks.push_back(
      new TTrack(
        new Modifier(*track.handler) ));
  }

  if (track.handler->tracks.empty())
    return;

  TTrack &subTrack = *track.handler->tracks.front();
  Modifier *modifier = dynamic_cast<Modifier*>(subTrack.modifier.getPointer());
  if (!modifier)
    return;

  outTracks.push_back(track.handler->tracks.front());

  if ( !track.changed()
    && track.size() == subTrack.size()
    && track.size() == (int)modifier->tangents.size() )
      return;

  if (!track.changed() && subTrack.size() == track.size() - 1) {
    // add temporary point
    modifier->tangents.push_back(TTrackTangent());
    subTrack.push_back(track.back());
  } else {
    // apply permanent changes

    // remove points
    int start = track.size() - track.pointsAdded;
    if (start < 0) start = 0;
    if (start > 1) --start;
    subTrack.truncate(start);
    TTrackTangent lastTangent =
        start < (int)modifier->tangents.size() ? modifier->tangents[start]
      : modifier->tangents.empty() ? TTrackTangent()
      : modifier->tangents.back();
    modifier->tangents.resize(start, lastTangent);

    // add first point
    int index = start;
    if (index == 0) {
      modifier->tangents.push_back(TTrackTangent());
      subTrack.push_back(track.back());
      ++index;
    }

    // add points with tangents
    if (track.size() > 2) {
      while(index < track.size() - 1) {
        const TTrackPoint &p0 = track[index-1];
        const TTrackPoint &p1 = track[index];
        const TTrackPoint &p2 = track[index+1];
        double dt = p2.time - p0.time;
        double k = dt > TTrack::epsilon ? (p1.time - p0.time)/dt : 0.0;
        TTrackTangent tangent(
          (p2.position - p0.position)*k,
          (p2.pressure - p0.pressure)*k,
          (p2.tilt - p0.tilt)*k );
        modifier->tangents.push_back(tangent);
        subTrack.push_back(p1);
        ++index;
      }
    }

    track.resetChanges();

    // release previous key point
    modifier->savePoint.reset();

    if (track.finished()) {
      // finish
      modifier->tangents.push_back(TTrackTangent());
      subTrack.push_back(track.back());
    } else {
      // save key point
      modifier->savePoint = savePoint;
    }
  }
}

