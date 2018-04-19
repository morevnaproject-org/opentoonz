#pragma once

#ifndef TASSISTANTSIMAGE_INCLUDED
#define TASSISTANTSIMAGE_INCLUDED

#include "timage.h"
#include "tthreadmessage.h"

#include <QReadLocker>
#include <QWriteLocker>
#include <QReadWriteLock>

#include <string>

#undef DVAPI
#undef DVVAR
#ifdef TASSISTANTSIMAGE_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

//-------------------------------------------------------------------

class TAssistantDesc;
typedef std::vector<TAssistantDesc> TAssistantDescList;

//-------------------------------------------------------------------

class TAssistantDesc {
public:
  std::string type;
  std::vector<TPointD> points;
  TSmartObjectP handler;
};

//-------------------------------------------------------------------

//! An image containing an assistants for painting.

class DVAPI TAssistantsImage final : public TImage {
public:
  class Reader: public QReadLocker {
  private:
    const TAssistantsImage &m_image;
  public:
    Reader(const TAssistantsImage &image):
      QReadLocker(&image.m_rwLock), m_image(image) { }
    const TAssistantsImage& image() const
      { return m_image; }
    const TAssistantDescList& get() const
      { return m_image.m_assistants; }
    const TAssistantDescList& operator*() const
      { return get(); }
    const TAssistantDescList* operator->() const
      { return &get(); }
  };

  class Writer: public QWriteLocker {
  private:
    TAssistantsImage &m_image;
  public:
    Writer(TAssistantsImage &image):
      QWriteLocker(&image.m_rwLock), m_image(image) { }
    TAssistantsImage& image() const
      { return m_image; }
    TAssistantDescList& get() const
      { return m_image.m_assistants; }
    TAssistantDescList& operator*() const
      { return get(); }
    TAssistantDescList* operator->() const
      { return &get(); }
  };

private:
  mutable QReadWriteLock m_rwLock;
  TAssistantDescList m_assistants;

public:
  TAssistantsImage();
  ~TAssistantsImage();

private:
  //! not implemented
  TAssistantsImage(const TAssistantsImage &other);
  TAssistantsImage &operator=(const TAssistantsImage &) { return *this; }

public:
  //! Return the image type
  TImage::Type getType() const override { return TImage::ASSISTANTS; }

  //! Return a clone of image
  TImage* cloneImage() const override;

  //! Return the bbox of the image
  TRectD getBBox() const override;
};

#endif
