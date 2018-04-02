#pragma once

#ifndef TRACK_INCLUDED
#define TRACK_INCLUDED

// TnzTools includes
#include <tools/inputstate.h>

// TnzCore includes
#include <tcommon.h>
#include <tgeometry.h>

// Qt headers
#include <Qt>

// std includes
#include <vector>
#include <algorithm>


#undef DVAPI
#undef DVVAR
#ifdef TNZTOOLS_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif


//===================================================================

//    Forward Declarations

class TTrack;
class TTrackHandler;
class TTrackModifier;
typedef TSmartPointerT<TTrack> TTrackP;
typedef TSmartPointerT<TTrackHandler> TTrackHandlerP;
typedef TSmartPointerT<TTrackModifier> TTrackModifierP;

//===================================================================


//*****************************************************************************************
//    TTrackPoint definition
//*****************************************************************************************

struct TTrackPoint {
public:
  TPointD position;
  double pressure;
  TPointD tilt;

  double originalIndex;
  double time;
  double length;

  bool final;

  explicit TTrackPoint(
    const TPointD &position = TPointD(),
    double pressure = 0.5,
    const TPointD &tilt = TPointD(),
    double originalIndex = 0.0,
    double time = 0.0,
    double length = 0.0,
    bool final = false
  ):
    position(position),
    pressure(pressure),
    tilt(tilt),
    originalIndex(originalIndex),
    time(time),
    length(length),
    final(final)
  { }
};


//*****************************************************************************************
//    TTrackHandler definition
//*****************************************************************************************

class DVAPI TTrackHandler : public TSmartObject {
  DECLARE_CLASS_CODE
public:
  TSmartObject &owner;
  TTrack &original;
  std::vector<TTrackP> tracks;
  TTrackHandler(TSmartObject &owner, TTrack &original):
    owner(owner), original(original) { }
};


//*****************************************************************************************
//    TTrackModifier definition
//*****************************************************************************************

class DVAPI TTrackModifier : public TSmartObject {
  DECLARE_CLASS_CODE
public:
    TTrackHandler &handler;
    TTrack &original;
    const double timeOffset;

    explicit TTrackModifier(TTrackHandler &handler, double timeOffset = 0.0):
      handler(handler), original(handler.original), timeOffset(timeOffset) { }
    virtual TTrackPoint calcPoint(double originalIndex);
};


//*****************************************************************************************
//    TTrack definition
//*****************************************************************************************

class DVAPI TTrack : public TSmartObject {
  DECLARE_CLASS_CODE

public:
  typedef long long Id;
  typedef long long TouchId;

  static const double epsilon;

private:
  static Id lastId;

public:
  const Id id;
  const TInputState::DeviceId deviceId;
  const TouchId touchId;
  const TInputState::KeyHistory::Holder keyHistory;
  const TInputState::ButtonHistory::Holder buttonHistory;
  const TTrackModifierP modifier;

  TTrackHandlerP handler;
  int wayPointsRemoved;
  int wayPointsAdded;

private:
  std::vector<TTrackPoint> points_;
  const TTrackPoint none;

public:

  explicit TTrack(
    TInputState::DeviceId deviceId = TInputState::DeviceId(),
    TouchId touchId = TouchId(),
    const TInputState::KeyHistory::Holder &keyHistory = TInputState::KeyHistory::Holder(),
    const TInputState::ButtonHistory::Holder &buttonHistory = TInputState::ButtonHistory::Holder()
  );

  explicit TTrack(const TTrackModifierP &modifier);

  inline TTrack* original() const
    { return modifier ? &modifier->original : NULL; }
  inline double timeOffset() const
    { return modifier ? modifier->timeOffset : 0.0; }
  inline TTimerTicks ticks() const
    { return keyHistory.ticks(); }
  inline bool changed() const
    { return wayPointsAdded != 0 || wayPointsRemoved != 0; }

  const TTrack* root() const;
  TTrack* root();
  int level() const;

  inline int clampIndex(int index) const
    { return std::min(std::max(index, 0), size() - 1); }
  inline int floorIndexNoClamp(double index) const
    { return (int)floor(index + epsilon); }
  inline int floorIndex(double index) const
    { return clampIndex(floorIndexNoClamp(index)); }
  inline int ceilIndexNoClamp(double index) const
    { return (int)ceil(index - epsilon); }
  inline int ceilIndex(double index) const
    { return clampIndex(ceilIndexNoClamp(index)); }

  int floorIndex(double index, double &outFrac) const;

  inline const TTrackPoint& floorPoint(double index, double &outFrac) const
    { return point(floorIndex(index, outFrac)); }
  inline const TTrackPoint& floorPoint(double index) const
    { return point(floorIndex(index)); }
  inline const TTrackPoint& ceilPoint(double index) const
    { return point(ceilIndex(index)); }

  inline const TTrackPoint& point(int index) const
    { return empty() ? none : points_[clampIndex(index)]; }

  inline int size() const
    { return (int)points_.size(); }
  inline bool empty() const
    { return points_.empty(); }
  inline const TTrackPoint& front() const
    { return point(0); }
  inline const TTrackPoint& back() const
    { return point(size() - 1); }
  inline bool finished() const
    { return !points_.empty() && back().final; }
  inline const TTrackPoint& operator[] (int index) const
    { return point(index); }
  inline const std::vector<TTrackPoint>& points() const
    { return points_; }

  void push_back(const TTrackPoint &point);
  void pop_back(int count = 1);

  inline void truncate(int count)
    { pop_back(size() - count); }


private:
  template<double TTrackPoint::*Field>
  double binarySearch(double value) const {
    // points_[a].value <= value < points_[b].value
    if (points_.empty()) return 0.0;
    int a = 0;
    double aa = points_[a].*Field;
    if (value - aa <= 0.5*epsilon) return (double)a;
    int b = (int)points_.size() - 1;
    double bb = points_[b].*Field;
    if (bb - value <= 0.5*epsilon) return (double)b;
    while(true) {
      int c = (a + b)/2;
      if (a == c) break;
      double cc = points_[c].*Field;
      if (cc - value > 0.5*epsilon)
        { b = c; bb = cc; } else { a = c; aa = cc; }
    }
    return bb - aa >= 0.5*epsilon ? (double)a + (value - aa)/(bb - aa) : (double)a;
  }

public:
  inline double indexByOriginalIndex(double originalIndex) const
    { return binarySearch<&TTrackPoint::originalIndex>(originalIndex); }
  inline double indexByTime(double time) const
    { return binarySearch<&TTrackPoint::time>(time); }
  inline double indexByLength(double length) const
    { return binarySearch<&TTrackPoint::length>(length); }

  inline double originalIndexByIndex(double index) const {
    double frac;
    const TTrackPoint &p0 = floorPoint(index, frac);
    const TTrackPoint &p1 = ceilPoint(index);
    return interpolationLinear(p0.originalIndex, p1.originalIndex, frac);
  }
  inline double timeByIndex(double index) const {
    double frac;
    const TTrackPoint &p0 = floorPoint(index, frac);
    const TTrackPoint &p1 = ceilPoint(index);
    return interpolationLinear(p0.time, p1.time, frac);
  }
  inline double lengthByIndex(double index) const {
    double frac;
    const TTrackPoint &p0 = floorPoint(index, frac);
    const TTrackPoint &p1 = ceilPoint(index);
    return interpolationLinear(p0.length, p1.length, frac);
  }

  TTrackPoint calcPoint(double index) const;
  TPointD calcTangent(double index, double distance = 0.1) const;

  inline TTrackPoint interpolateLinear(double index) const {
    double frac;
    const TTrackPoint &p0 = floorPoint(index, frac);
    const TTrackPoint &p1 = ceilPoint(index);
    return interpolationLinear(p0, p1, frac);
  }

  template<typename T>
  static inline T interpolationLinear(const T &p0, const T &p1, double l)
    { return p0*(1.0 - l) + p1*l; }

  static inline TTrackPoint interpolationLinear(const TTrackPoint &p0, const TTrackPoint &p1, double l) {
    if (l <= epsilon) return p0;
    if (l >= 1.0 - epsilon) return p1;
    return TTrackPoint(
      interpolationLinear(p0.position      , p1.position      , l),
      interpolationLinear(p0.pressure      , p1.pressure      , l),
      interpolationLinear(p0.tilt          , p1.tilt          , l),
      interpolationLinear(p0.originalIndex , p1.originalIndex , l),
      interpolationLinear(p0.time          , p1.time          , l),
      interpolationLinear(p0.length        , p1.length        , l) );
  }
};

#endif
