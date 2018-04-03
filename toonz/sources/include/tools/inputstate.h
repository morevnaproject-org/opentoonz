#pragma once

#ifndef INPUTSTATE_INCLUDED
#define INPUTSTATE_INCLUDED

// TnzTools includes
#include <tools/tooltimer.h>
#include <tools/keyhistory.h>

// TnzCore includes
#include <tcommon.h>
#include <tsmartpointer.h>

// Qt includes
#include <Qt>

// std includes
#include <map>


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


//*****************************************************************************************
//    TInputState definition
//*****************************************************************************************

class TInputState {
public:
  typedef qint64 DeviceId;
  typedef long long TouchId;

  typedef Qt::Key Key;
  typedef TKeyHistoryT<Key> KeyHistory;
  typedef KeyHistory::State KeyState;

  typedef Qt::MouseButton Button;
  typedef TKeyHistoryT<Button> ButtonHistory;
  typedef ButtonHistory::State ButtonState;
  typedef std::map<DeviceId, ButtonHistory::Pointer> ButtonHistoryMap;

private:
  TTimerTicks ticks;
  KeyHistory::Pointer keyHistory_;
  mutable ButtonHistoryMap buttonHistories_;

  void touch(TTimerTicks ticks);

public:
  TInputState();
  ~TInputState();

  inline KeyHistory::Pointer keyHistory() const
    { return keyHistory(); }
  inline KeyState::Pointer keyState() const
    { return keyHistory()->current(); }

  inline void keyEvent(bool press, Key key, TTimerTicks ticks)
    { touch(ticks); keyHistory()->change(press, key, this->ticks); }
  inline void keyPress(Key key, TTimerTicks ticks)
    { keyEvent(true, key, ticks); }
  inline void keyRelease(Key key, TTimerTicks ticks)
    { keyEvent(false, key, ticks); }

  inline KeyState::Pointer keyFind(Key key) const
    { return keyState()->find(key); }
  inline bool isKeyPressed(Key key) const
    { return keyFind(key); }
  inline double howLongKeyPressed(Key key, TTimerTicks ticks, double timeOffset = 0.0)
    { return KeyState::Holder::howLongPressed(keyFind(key), ticks, timeOffset); }
  inline double howLongKeyPressed(Key key)
    { return howLongKeyPressed(key, ticks); }

  ButtonHistory::Pointer buttonHistory(DeviceId device) const;

  inline const ButtonHistoryMap buttonHistories() const
    { return buttonHistories_; }
  inline ButtonState::Pointer buttonState(DeviceId device) const
    { return buttonHistory(device)->current(); }

  inline void buttonEvent(bool press, DeviceId device, Button button, TTimerTicks ticks)
    { touch(ticks); buttonHistory(device)->change(press, button, this->ticks); }
  inline void buttonPress(DeviceId device, Button button, TTimerTicks ticks)
    { buttonEvent(true, device, button, ticks); }
  inline void buttonRelease(DeviceId device, Button button, TTimerTicks ticks)
    { buttonEvent(false, device, button, ticks); }
  inline void buttonEvent(bool press, Button button, TTimerTicks ticks)
    { buttonEvent(press, 0, button, ticks); }
  inline void buttonPress(Button button, TTimerTicks ticks)
    { buttonEvent(true, button, ticks); }
  inline void buttonRelease(Button button, TTimerTicks ticks)
    { buttonEvent(false, button, ticks); }

  inline ButtonState::Pointer buttonFind(DeviceId device, Button button)
    { return buttonState(device)->find(button); }
  inline bool isButtonPressed(DeviceId device, Button button)
    { return buttonFind(device, button); }
  inline double howLongButtonPressed(DeviceId device, Button button, TTimerTicks ticks, double timeOffset = 0.0)
    { return ButtonState::Holder::howLongPressed(buttonFind(device, button), ticks, timeOffset); }
  inline double howLongButtonPressed(DeviceId device, Button button)
    { return howLongButtonPressed(device, button, ticks); }

  inline ButtonState::Pointer buttonFindDefault(Button button)
    { return buttonFind(DeviceId(), button); }
  inline bool isButtonPressedDefault(Button button)
    { return isButtonPressed(DeviceId(), button); }
  inline double howLongButtonPressedDefault(Button button, TTimerTicks ticks, double timeOffset = 0.0)
    { return howLongButtonPressed(DeviceId(), button, ticks, timeOffset); }
  inline double howLongButtonPressedDefault(Button button)
    { return howLongButtonPressedDefault(button, ticks); }

  ButtonState::Pointer buttonFindAny(Button button, DeviceId &outDevice);

  inline ButtonState::Pointer buttonFindAny(Button button)
    { DeviceId device; return buttonFindAny(button, device); }
  inline bool isButtonPressedAny(Button button)
    { return buttonFindAny(button); }
  inline double howLongButtonPressedAny(Button button, TTimerTicks ticks, double timeOffset = 0.0)
    { return ButtonState::Holder::howLongPressed(buttonFindAny(button), ticks, timeOffset); }
  inline double howLongButtonPressedAny(Button button)
    { return howLongButtonPressedAny(button, ticks); }

  inline KeyState::Holder keyStateHolder(TTimerTicks ticks, double timeOffset = 0.0) const
    { return KeyState::Holder(keyState(), ticks, timeOffset); }
  inline KeyState::Holder keyStateHolder()
    { return keyStateHolder(ticks); }
  inline KeyHistory::Holder keyHistoryHolder(TTimerTicks ticks, double timeOffset = 0.0)
    { return KeyHistory::Holder(keyHistory(), ticks, timeOffset); }
  inline KeyHistory::Holder keyHistoryHolder()
    { return keyHistoryHolder(ticks); }

  inline ButtonState::Holder buttonStateHolder(DeviceId device, TTimerTicks ticks, double timeOffset = 0.0)
    { return ButtonState::Holder(buttonState(device), ticks, timeOffset); }
  inline ButtonState::Holder buttonStateHolder(DeviceId device)
    { return buttonStateHolder(device, ticks); }
  inline ButtonHistory::Holder buttonHistoryHolder(DeviceId device, long ticks, double timeOffset = 0.0)
    { return ButtonHistory::Holder(buttonHistory(device), ticks, timeOffset); }
  inline ButtonHistory::Holder buttonHistoryHolder(DeviceId device)
    { return buttonHistoryHolder(device, ticks); }
};


#endif
