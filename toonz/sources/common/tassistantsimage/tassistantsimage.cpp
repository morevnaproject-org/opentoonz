

#include "tassistantsimage.h"

//---------------------------------------------------------

TAssistantsImage::TAssistantsImage()
  { }

//---------------------------------------------------------

TAssistantsImage::TAssistantsImage(const TAssistantsImage &other):
  m_assistants(*Reader(other))
  { }

//---------------------------------------------------------

TAssistantsImage::~TAssistantsImage()
  { }

//---------------------------------------------------------

TImage*
TAssistantsImage::cloneImage() const
  { return new TAssistantsImage(*this); }

//---------------------------------------------------------

TRectD
TAssistantsImage::getBBox() const
  { return TRectD(); }

//---------------------------------------------------------
