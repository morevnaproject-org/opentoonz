#pragma once

#ifndef KEYSTATE_INCLUDED
#define KEYSTATE_INCLUDED

// TnzTools includes
#include <tools/tooltimer.h>

// TnzCore includes
#include <tcommon.h>
#include <tsmartpointer.h>

// std includes
#include <map>
#include <set>
#include <algorithm>
#include <cmath>


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

//    Base Classee Declarations


class DVAPI TKeyStateBase : public TSmartObject {
  DECLARE_CLASS_CODE
};

class DVAPI TKeyHistoryBase : public TSmartObject {
  DECLARE_CLASS_CODE
};

//===================================================================


//*****************************************************************************************
//    TKeyState definition
//*****************************************************************************************

template<typename T>
class TKeyStateT : public TKeyStateBase {
public:
  typedef T Type;
  typedef TSmartPointerT<TKeyStateT> Pointer;

  struct Holder {
    Pointer state;
    TTimerTicks ticks;
    double timeOffset;

    explicit Holder(const Pointer &state, TTimerTicks ticks = 0, double timeOffset = 0.0):
      state(state), ticks(ticks), timeOffset(timeOffset) { }

    Pointer find(const Type &value) const
      { return state ? state->find(value) : Pointer(); }
    bool isEmpty() const
      { return !state || state->isEmpty(); }
    bool isPressed(const Type &value) const
      { return find(value); }
    double howLongPressed(const Type &value)
      { return howLongPressed(find(value), ticks, timeOffset); }

    static double howLongPressed(const Pointer &state, long ticks, double timeOffset) {
      return state
           ? std::max(TToolTimer::step, (ticks - state->ticks)*TToolTimer::step + timeOffset)
           : 0.0;
    }
  };

  static const Type none;
  static const Pointer empty;

  const Pointer previous;
  const TTimerTicks ticks;
  const Type value;

private:
  TKeyStateT(const Pointer &previous, TTimerTicks ticks, const Type &value):
    previous(previous), ticks(ticks), value(value) { }

  Pointer makeChainWithout(const Pointer &state) {
    return state == this || !previous ? previous
         : Pointer(new TKeyStateT(previous->makeChainWithout(state), ticks, value));
  }

public:
  TKeyStateT(): ticks(0), value(none) { }

  Pointer find(const Type &value) {
    return value == none        ? Pointer()
         : value == this->value ? this
         : previous             ? previous->find(value)
         :                        Pointer();
  }

  Pointer change(bool press, const Type &value, TTimerTicks ticks) {
    if (value == none)
      return Pointer(this);
    if (ticks <= this->ticks)
      ticks = this->ticks + 1;

    Pointer p = find(value);
    if (press) {
      if (p) return Pointer(this);
      return Pointer(new TKeyStateT((isEmpty() ? Pointer() : Pointer(this)), ticks, value));
    }

    if (!p) return Pointer(this);
    Pointer chain = makeChainWithout(p);
    return chain ? chain : Pointer(new TKeyStateT());
  }

  bool isEmpty()
    { return value == none && (!previous || previous->isEmpty()); }
  bool isPressed(const Type &value)
    { return find(value); }

  Pointer pressed(const Type &value, long ticks)
    { return change(true, value, ticks); }
  Pointer released(const Type &value, long ticks)
    { return change(false, value, ticks); }
};


template<typename T>
const typename TKeyStateT<T>::Type TKeyStateT<T>::none = typename TKeyStateT<T>::Type();

template<typename T>
const typename TKeyStateT<T>::Pointer TKeyStateT<T>::empty = new TKeyStateT<T>();


//*****************************************************************************************
//    TKeyHistory definition
//*****************************************************************************************

template<typename T>
class TKeyHistoryT : public TKeyHistoryBase {
public:
  typedef T Type;
  typedef TSmartPointerT<TKeyHistoryT> Pointer;
  typedef TKeyStateT<Type> State;
  typedef typename TKeyStateT<Type>::Pointer StatePointer;
  typedef typename TKeyStateT<Type>::Holder StateHolder;

  class Holder {
  private:
    Pointer history_;
    TTimerTicks ticks_;
    double timeOffset_;
    TTimerTicks heldTicks;

  public:
    Holder():
      ticks_(), timeOffset_(), heldTicks() { }
    Holder(const Pointer &history, TTimerTicks ticks, double timeOffset = 0.0):
      ticks_(), timeOffset_(), heldTicks()
      { set(history, ticks, timeOffset); }
    Holder(const Holder &other):
      ticks_(), timeOffset_(), heldTicks()
      { set(other); }
    ~Holder()
      { reset(); }

    Holder& operator= (const Holder &other)
      { set(other); return *this; }

    void set(const Pointer &history, TTimerTicks ticks, double timeOffset = 0.0) {
      if (history_) history_->releaseTicks(heldTicks);
      history_ = history;
      ticks_ = ticks;
      timeOffset_ = timeOffset;
      heldTicks = (history_ ? history_->holdTicks(ticks_) : 0);
    }
    void set(const Holder &other)
      { set(other.history(), other.ticks(), other.timeOffset()); }
    void reset()
      { set(Pointer(), 0); }

    Pointer history() const
      { return history_; }
    TTimerTicks ticks() const
      { return ticks_; }
    double timeOffset() const
      { return timeOffset_; }

    Holder offset(double timeOffset) const {
      return fabs(timeOffset) < TToolTimer::epsilon ? *this
           : Holder(history, ticks, this->timeOffset + timeOffset);
    }

    StateHolder get(double time) const {
      TTimerTicks dticks = (TTimerTicks)ceil(TToolTimer::frequency*(time + timeOffset));
      StatePointer state = history_->get(ticks + dticks);
      return StateHolder(state, ticks, timeOffset + time);
    }
  };

private:
  std::map<TTimerTicks, StatePointer> states;
  std::multiset<TTimerTicks> locks;

  void autoRemove() {
    TTimerTicks ticks = locks.empty()
                      ? states.rbegin()->first
                      : *locks.begin();
    while(true) {
      typename std::map<TTimerTicks, StatePointer>::iterator i = states.begin();
      ++i;
      if (i == states.end() || (!i->second->isEmpty() && i->first >= ticks)) break;
      states.erase(i);
    }
  }

  TTimerTicks holdTicks(TTimerTicks ticks)
    { return *locks.insert(std::max(ticks, states.begin()->first)); }
  void releaseTicks(TTimerTicks heldTicks)
    { locks.erase(heldTicks); autoRemove(); }

  StatePointer get(TTimerTicks ticks) {
    typename std::map<TTimerTicks, StatePointer>::iterator i = states.upper_bound(ticks);
    return i == states.begin() ? i->second : (--i)->second;
  }

public:
  TKeyHistoryT()
    { states[TTimerTicks()] = StatePointer(new State()); }

  StatePointer current() const
    { return states.rbegin()->second; }

  StatePointer change(bool press, Type value, TTimerTicks ticks)  {
    StatePointer state = current()->change(press, value, ticks);
    if (state != current() && state->ticks > states.rbegin()->first)
      states[state->ticks] = state;
    autoRemove();
    return current();
  }

  StatePointer pressed(Type value, TTimerTicks ticks)
    { return change(true, value, ticks); }

  StatePointer released(Type value, TTimerTicks ticks)
    { return change(false, value, ticks); }
};


#endif
