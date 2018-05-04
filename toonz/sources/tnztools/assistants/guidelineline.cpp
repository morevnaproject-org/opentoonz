

#include "guidelineline.h"

// TnzCore includes
#include "tgl.h"


//*****************************************************************************************
//    TGuidelineLineBase implementation
//*****************************************************************************************

TGuidelineLineBase::TGuidelineLineBase(const TPointD &p0, const TPointD &p1):
  p0(p0), p1(p1) { }

TPointD
TGuidelineLineBase::calcDirection(const TPointD &p0, const TPointD &p1) {
  TPointD d = p1 - p0;
  double k = norm2(d);
  return k > TConsts::epsilon ? d*(1.0/k) : TPointD();
}

void
TGuidelineLineBase::truncateInfiniteLine(const TRectD &bounds, TPointD &p0, TPointD &p1) {
  TPointD d = p0 - p1;
  TDimensionD size = bounds.getSize();
  if (fabs(d.x)*bounds.y0 > bounds.x0*fabs(d.y)) {
    // horizontal
    if (fabs(d.x) < TConsts::epsilon) return;
    double k = d.y/d.x;
    p1 = TPointD(bounds.x1, p0.y + k*(bounds.x1 - p0.x));
    p0 = TPointD(bounds.x0, p0.y + k*(bounds.x0 - p0.x));
  } else {
    // vertical
    if (fabs(d.y) < TConsts::epsilon) return;
    double k = d.x/d.y;
    p1 = TPointD(p0.x + k*(bounds.y1 - p0.y), bounds.y1);
    p0 = TPointD(p0.x + k*(bounds.y0 - p0.y), bounds.y0);
  }
}

void
TGuidelineLineBase::drawInliniteLine(const TPointD &p0, const TPointD &p1, bool ray, bool active) const {
  TAffine4 modelview, projection;
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview.a);
  glGetDoublev(GL_PROJECTION_MATRIX, projection.a);

  TAffine matrix = (projection*modelview).get2d();
  TPointD pp0 = matrix*p0;
  TPointD pp1 = matrix*p1;
  truncateInfiniteLine(TRectD(-1.0, -1.0, 1.0, 1.0), pp0, pp1);

  double pixelSize = sqrt(tglGetPixelSize2());
  TAffine matrixInv = matrix.inv();
  drawSegment((ray ? p0 : matrixInv*pp0), matrixInv*pp1, pixelSize, active);
}


//*****************************************************************************************
//    TGuidelineLine implementation
//*****************************************************************************************

TGuidelineLine::TGuidelineLine(const TPointD &p0, const TPointD &p1):
  TGuidelineLineBase(p0, p1),
  dir(calcDirection(p0, p1)),
  dist(norm(p1 - p0)) { }

TTrackPoint
TGuidelineLine::transformPoint(const TTrackPoint &point) const {
  TTrackPoint p(point);
  p.position = p0 + dir * std::max(0.0, std::min(dist, ((p.position - p0)*dir)));
  return p;
}

void
TGuidelineLine::draw(bool active) const
  { drawSegment(p0, p1, sqrt(tglGetPixelSize2()), active); }


//*****************************************************************************************
//    TGuidelineInfiniteLine implementation
//*****************************************************************************************

TGuidelineInfiniteLine::TGuidelineInfiniteLine(const TPointD &p0, const TPointD &p1):
  TGuidelineLineBase(p0, p1),
  dir(calcDirection(p0, p1)) { }

TTrackPoint
TGuidelineInfiniteLine::transformPoint(const TTrackPoint &point) const {
  TTrackPoint p(point);
  p.position = p0 + dir * ((p.position - p0)*dir);
  return p;
}

void
TGuidelineInfiniteLine::draw(bool active) const
  { drawInliniteLine(p0, p1, false, active); }


//*****************************************************************************************
//    TGuidelineRay implementation
//*****************************************************************************************

TGuidelineRay::TGuidelineRay(const TPointD &p0, const TPointD &p1):
  TGuidelineLineBase(p0, p1),
  dir(calcDirection(p0, p1)) { }

TTrackPoint
TGuidelineRay::transformPoint(const TTrackPoint &point) const {
  TTrackPoint p(point);
  p.position = p0 + dir * std::max(0.0, ((p.position - p0)*dir));
  return p;
}

void
TGuidelineRay::draw(bool active) const
  { drawInliniteLine(p0, p1, true, active); }

