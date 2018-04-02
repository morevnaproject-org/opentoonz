

#include <tools/inputstate.h>


//*****************************************************************************************
//    TInputState static members
//*****************************************************************************************

TInputState::TInputState():
  ticks(),
  keyHistory_(new KeyHistory())
  { }

TInputState::~TInputState()
  { }

void
TInputState::touch(TTimerTicks ticks) {
  if (this->ticks < ticks)
    this->ticks = ticks;
  else
    ++this->ticks;
}

TInputState::ButtonHistory::Pointer
TInputState::buttonHistory(DeviceId device) const {
  ButtonHistory::Pointer &history = buttonHistories_[device];
  if (!history) history = new ButtonHistory();
  return history;
}

TInputState::ButtonState::Pointer
TInputState::buttonFindAny(Button button, DeviceId &outDevice) {
  for(ButtonHistoryMap::const_iterator i = buttonHistories_.begin(); i != buttonHistories_.end(); ++i) {
    ButtonState::Pointer state = i->second->current()->find(button);
    if (state) {
      outDevice = i->first;
      return state;
    }
  }
  outDevice = DeviceId();
  return ButtonState::Pointer();
}
