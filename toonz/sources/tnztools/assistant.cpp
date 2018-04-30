
#include <tools/assistant.h>

#include <limits>


//************************************************************************
//    TGuideline implementation
//************************************************************************

double
TGuideline::calcTrackWeight(const TTrack &track, const TAffine &affine) const {
  if (track.empty() < 1)
    return std::numeric_limits<double>::infinity();

  const double snapLenght = 20.0;
  const double snapScale = 1.0;
  const double maxLenght = 20.0*snapLenght*snapScale;

  double sumWeight = 0.0;
  double sumLength = 0.0;
  double sumDeviation = 0.0;

  TPointD prev = affine*track[0].position;
  for(int i = 0; i < track.size(); ++i) {
    const TTrackPoint &tp = track[i];
    TPointD p = affine*tp.position;
    double length = tdistance(p, prev);
    sumLength += length;

    double midStepLength = sumLength - 0.5*length;
    if (midStepLength > TTrack::epsilon) {
      double weight = length*logNormalDistribuitionUnscaled(midStepLength, snapLenght, snapScale);
      sumWeight += weight;

      TTrackPoint ntp = transformPoint(tp);
      double deviation = tdistance(affine*ntp.position, p);
      sumDeviation += weight*deviation;
    }
    prev = p;
  }
  if (sumWeight < TTrack::epsilon)
    return std::numeric_limits<double>::infinity();
  return sumDeviation/sumWeight;
}

//---------------------------------------------------------------------------------------------------

TGuidelineP
TGuideline::findBest(const TGuidelineList &guidelines, const TTrack &track, const TAffine &affine) {
  double bestWeight = 0.0;
  TGuidelineP best;
  for(TGuidelineList::const_iterator i = guidelines.begin(); i != guidelines.end(); ++i) {
    double weight = (*i)->calcTrackWeight(track, affine);
    if (!best || weight < bestWeight)
      { bestWeight = weight; best = *i; }
  }
  return best;
}


//************************************************************************
//    TAssistant implementation
//************************************************************************

// TODO:
