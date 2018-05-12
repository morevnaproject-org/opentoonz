
// TnzTools includes
#include <tools/tool.h>
#include <tools/toolutils.h>
#include <tools/toolhandle.h>
#include <tools/cursors.h>
#include <tools/assistant.h>

// TnzLib includes
#include <toonz/tapplication.h>
#include <toonz/txshlevelhandle.h>

// TnzCore includes
#include <tgl.h>
#include <tproperty.h>
#include <tmetaimage.h>

// For Qt translation support
#include <QCoreApplication>

#include <map>


//-------------------------------------------------------------------

//=============================================================================
// Edit Assistants Undo
//-----------------------------------------------------------------------------

class EditAssistantsUndo final : public ToolUtils::TToolUndo {
private:
  bool m_isAssistantCreated;
  TMetaObjectP m_metaObject;
  TVariant m_oldData;
  TVariant m_newData;
  size_t m_size;

public:
  EditAssistantsUndo(
    TXshSimpleLevel *level,
    const TFrameId &frameId,
    bool isAssistantCreated,
    TMetaObjectR metaObject,
    TVariant oldData
  ):
    ToolUtils::TToolUndo(level, frameId),
    m_isAssistantCreated(isAssistantCreated),
    m_metaObject(metaObject.getPointer()),
    m_oldData(oldData),
    m_newData(m_metaObject->data()),
    m_size(m_oldData.getMemSize() + m_newData.getMemSize())
  { }

  int getSize() const override
    { return m_size; }
  QString getToolName() override
    { return QString("Edit Assistants Tool"); }

  void undo() const override {
    if (TMetaImage *metaImage = dynamic_cast<TMetaImage*>(m_level->getFrame(m_frameId, true).getPointer())) {
      TMetaImage::Writer writer(*metaImage);
      if (m_isAssistantCreated) {
        for(TMetaObjectRefList::iterator i = writer->begin(); i != writer->end(); ++i)
          if (*i == m_metaObject) { writer->erase(i); break; }
      } else {
        m_metaObject->data() = m_oldData;
        if (TMetaObjectHandler *handler = m_metaObject->getHandler<TMetaObjectHandler>())
          handler->fixData();
      }
      notifyImageChanged();
    }
  }

  void redo() const override {
    if (TMetaImage *metaImage = dynamic_cast<TMetaImage*>(m_level->getFrame(m_frameId, true).getPointer())) {
      TMetaImage::Writer writer(*metaImage);
      m_metaObject->data() = m_newData;
      if (TMetaObjectHandler *handler = m_metaObject->getHandler<TMetaObjectHandler>())
        handler->fixData();
      if (m_isAssistantCreated)
        writer->push_back(TMetaObjectR(m_metaObject.getPointer()));
      notifyImageChanged();
    }
  }
};


//=============================================================================
// Edit Assistants Tool
//-----------------------------------------------------------------------------

class EditAssistantsTool final : public TTool {
  Q_DECLARE_TR_FUNCTIONS(EditAssistantsTool)
public:
  typedef std::map<std::wstring, TStringId> TypeMap;

protected:
  TPropertyGroup m_prop;
  TEnumProperty m_assistantType;
  TypeMap m_localnameToType;
  TStringId m_newAssisnantType;

  bool           m_dragging;
  bool           m_currentAssistantCreated;
  int            m_currentAssistantIndex;
  int            m_currentPointIndex;
  TPointD        m_currentPointOffset;
  TVariant       m_currentAssistantBackup;
  TPointD        m_currentPosition;

public:
  EditAssistantsTool():
    TTool("T_EditAssistants"),
    m_assistantType("AssistantType"),
    m_dragging(),
    m_currentAssistantCreated(),
    m_currentAssistantIndex(-1),
    m_currentPointIndex(-1)
  {
    bind(MetaImage);
    m_prop.bind(m_assistantType);
    updateTranslation();
  }

  ToolType getToolType() const override
    { return TTool::LevelWriteTool; }
  int getCursorId() const override
    { return ToolCursor::StrokeSelectCursor; }
  TPropertyGroup* getProperties(int targetType) override
    { return &m_prop; }
  void onImageChanged() override
    { getViewer()->GLInvalidateAll(); }

  void addAssistantType(const std::string &name, const std::string &typeName) {
    const std::wstring localName = tr(name.c_str()).toStdWString();
    if (m_localnameToType.count(localName)) return;
    m_localnameToType[localName] = TStringId(typeName);
    m_assistantType.addValue(localName);
  }

  void updateTranslation() override {
    m_assistantType.setQStringName(tr("Assistant Type"));
    m_assistantType.deleteAllValues();
    m_localnameToType.clear();
    addAssistantType("--", "");
    addAssistantType("Vanishing Point", "assistantVanishingPoint");
    m_assistantType.setIndex(0);
  }

  bool onPropertyChanged(std::string propertyName) override {
    TypeMap::const_iterator i = m_localnameToType.find(m_assistantType.getValue());
    m_newAssisnantType = i == m_localnameToType.end() ? TStringId() : i->second;
    return true;
  }

  void resetCurrentPoint() {
    m_currentAssistantCreated = false;
    m_currentAssistantIndex = -1;
    m_currentPointIndex = -1;
    m_currentPointOffset = TPointD();
    m_currentAssistantBackup.reset();
  }

  const TAssistant* findCurrentPoint(const TPointD &position) {
    resetCurrentPoint();
    TMetaImage *mi = dynamic_cast<TMetaImage*>(getImage(false));
    if (!mi) return NULL;

    double pixelSize2 = tglGetPixelSize2();
    TMetaImage::Reader reader(*mi);
    const TAssistant *currentAssisntant = NULL;
    for(TMetaObjectRefList::const_iterator i = reader->begin(); i != reader->end(); ++i) {
      if (!*i) continue;
      const TAssistant *assistant = (*i)->getHandler<TAssistant>();
      if (!assistant) continue;
      assistant->deselectAll();
      for(int j = 0; j < assistant->pointsCount() && m_currentAssistantIndex < 0; ++j) {
        const TAssistantPoint &p = assistant->points()[j];
        TPointD offset = p.position - position;
        if (norm2(offset) <= p.radius*p.radius*pixelSize2) {
          m_currentAssistantIndex = i - reader->begin();
          m_currentPointIndex = j;
          m_currentPointOffset = offset;
          currentAssisntant = assistant;
          assistant->selectPoint(j);
          break;
        }
      }
    }
    return currentAssisntant;
  }

  void mouseMove(const TPointD &position, const TInputState &state) override {
    if (m_dragging) return;
    findCurrentPoint(position);
    m_currentPosition = position;
    getViewer()->GLInvalidateAll();
  }

  void leftButtonDown(const TTrackPoint& point, const TTrack&) override {
    m_dragging = true;
    if (m_newAssisnantType) {
      // create assistant
      resetCurrentPoint();
      if (TMetaImage *mi = dynamic_cast<TMetaImage*>(getImage(true))) {
        TMetaImage::Writer writer(*mi);
        TMetaObjectR obj(new TMetaObject(m_newAssisnantType));
        if (TAssistant *assistant = obj->getHandler<TAssistant>()) {
          if (assistant->pointsCount()) {
            assistant->movePoint(0, point.position);
            m_currentAssistantCreated = true;
            m_currentAssistantIndex = (int)writer->size();
            m_currentPointIndex = 0;
            m_currentPointOffset = TPointD();
          }
          writer->push_back(obj);
        }
      }
      m_newAssisnantType.reset();
    } else
    if (const TAssistant *assistant = findCurrentPoint(point.position)) {
      m_currentAssistantBackup = assistant->data();
    }
    m_currentPosition = point.position;
    getViewer()->GLInvalidateAll();
  }

  void leftButtonDrag(const TTrackPoint& point, const TTrack&) override {
    if (m_currentAssistantIndex >= 0)
    if (m_currentPointIndex >= 0)
    if (TMetaImage *mi = dynamic_cast<TMetaImage*>(getImage(true)))
    {
      TMetaImage::Writer writer(*mi);
      if (m_currentAssistantIndex < (int)writer->size())
      if (TMetaObjectR obj = (*writer)[m_currentAssistantIndex])
      if (TAssistant *assistant = obj->getHandler<TAssistant>())
      if (m_currentPointIndex < assistant->pointsCount())
      {
        assistant->movePoint(
          m_currentPointIndex,
          point.position + m_currentPointOffset);
      }
    }
    m_currentPosition = point.position;
    getViewer()->GLInvalidateAll();
  }

  void leftButtonUp(const TTrackPoint &point, const TTrack&) override {
    if (m_currentAssistantIndex >= 0)
    if (m_currentPointIndex >= 0)
    if (TMetaImage *mi = dynamic_cast<TMetaImage*>(getImage(true)))
    {
      TMetaImage::Writer writer(*mi);
      if (m_currentAssistantIndex < (int)writer->size())
      if (TMetaObjectR obj = (*writer)[m_currentAssistantIndex])
      if (TAssistant *assistant = obj->getHandler<TAssistant>())
      if (m_currentPointIndex < assistant->pointsCount())
      {
        assistant->movePoint(
          m_currentPointIndex,
          point.position + m_currentPointOffset);
        assistant->fixData();
        TUndoManager::manager()->add(new EditAssistantsUndo(
          getApplication()->getCurrentLevel()->getLevel()->getSimpleLevel(),
          getCurrentFid(),
          m_currentAssistantCreated,
          obj,
          m_currentAssistantBackup ));
      }
    }
    m_assistantType.setIndex(0);
    notifyImageChanged();
    getApplication()->getCurrentTool()->notifyToolChanged();
    m_currentPosition = point.position;
    getViewer()->GLInvalidateAll();
    m_dragging = false;
  }

  void draw() override {
    TMetaImage *mi = dynamic_cast<TMetaImage*>(getImage(false));
    if (!mi) return;
    TMetaImage::Reader reader(*mi);
    for(TMetaObjectRefList::const_iterator i = reader->begin(); i != reader->end(); ++i)
      if (*i)
        if (const TAssistant *assistant = (*i)->getHandler<TAssistant>())
          assistant->drawEdit(getViewer());
  }
};

//-------------------------------------------------------------------

EditAssistantsTool editAssistantsTool;
