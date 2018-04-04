

#include <tools/inputstate.h>


//*****************************************************************************************
//    TInputState static members
//*****************************************************************************************

TInputState::TInputState():
  m_ticks(),
  m_keyHistory(new KeyHistory())
  { }

TInputState::~TInputState()
  { }

void
TInputState::touch(TTimerTicks ticks) {
  if (m_ticks < ticks)
    m_ticks = ticks;
  else
    ++m_ticks;
}

TInputState::ButtonHistory::Pointer
TInputState::buttonHistory(DeviceId device) const {
  ButtonHistory::Pointer &history = m_buttonHistories[device];
  if (!history) history = new ButtonHistory();
  return history;
}

TInputState::ButtonState::Pointer
TInputState::buttonFindAny(Button button, DeviceId &outDevice) {
  for(ButtonHistoryMap::const_iterator i = m_buttonHistories.begin(); i != m_buttonHistories.end(); ++i) {
    ButtonState::Pointer state = i->second->current()->find(button);
    if (state) {
      outDevice = i->first;
      return state;
    }
  }
  outDevice = DeviceId();
  return ButtonState::Pointer();
}
