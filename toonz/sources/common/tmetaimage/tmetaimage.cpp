

#include <tmetaimage.h>


//---------------------------------------------------------

TMetaObject::TMetaObject(const TStringId &typeName):
  m_handler(),
  m_data(*this)
    { setType(typeName); }

//---------------------------------------------------------

TMetaObject::TMetaObject(const std::string &typeName):
  m_handler(),
  m_data(*this)
    { setType(typeName); }

//---------------------------------------------------------

TMetaObject::~TMetaObject()
  { resetType(); }

//---------------------------------------------------------

void
TMetaObject::setType(const TStringId &name) {
  if (m_type != name) {
    if (m_handler) delete m_handler;
    m_type = name;
    Registry::const_iterator i = registry().find(m_type);
    m_handler = i == registry().end() ? NULL : i->second(*this);
    onVariantChanged(m_data);
  }
}

//---------------------------------------------------------

void
TMetaObject::onVariantChanged(const TVariant &value)
  { if (m_handler) m_handler->dataChanged(value); }

//---------------------------------------------------------

TMetaObject::Registry&
TMetaObject::registry() {
  static Registry registry;
  return registry;
}

//---------------------------------------------------------

void
TMetaObject::registerType(const TStringId &typeName, Fabric fabric) {
  if (registry().count(typeName))
    std::cerr << "warning: type of TMetaObject are already registered: " << typeName.str() << std::endl;
  registry()[typeName] = fabric;
}

//---------------------------------------------------------

void
TMetaObject::unregisterType(const TStringId &typeName) {
  if (!registry().count(typeName))
    std::cerr << "warning: trying to unregister non-registered type of TMetaObject: " << typeName.str() << std::endl;
  registry().erase(typeName);
}

//---------------------------------------------------------

TMetaImage::TMetaImage()
  { }

//---------------------------------------------------------

TMetaImage::TMetaImage(const TMetaImage &other):
  m_objects(*Reader(other))
  { }

//---------------------------------------------------------

TMetaImage::~TMetaImage()
  { }

//---------------------------------------------------------

TImage*
TMetaImage::cloneImage() const
  { return new TMetaImage(*this); }

//---------------------------------------------------------

TRectD
TMetaImage::getBBox() const
  { return TRectD(); }

//---------------------------------------------------------
