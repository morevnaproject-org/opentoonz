

#include <tools/modifiers/modifierassistants.h>

// TnzTools includes
#include <tools/tool.h>

// TnzLib includes
#include <toonz/tapplication.h>
#include <toonz/txshlevelhandle.h>
#include <toonz/txsheethandle.h>
#include <toonz/txsheet.h>
#include <toonz/tframehandle.h>

// TnzCore includes
#include <tmetaimage.h>


//*****************************************************************************************
//    TModifierAssistants::Modifier implementation
//*****************************************************************************************


TModifierAssistants::Modifier::Modifier(TTrackHandler &handler):
  TTrackModifier(handler),
  initialized()
{ }


TTrackPoint
TModifierAssistants::Modifier::calcPoint(double originalIndex) {
  TTrackPoint p = TTrackModifier::calcPoint(originalIndex);
  return guidelines.empty() > 0 ? p : guidelines.front()->transformPoint(p);
}


//*****************************************************************************************
//    TModifierAssistants implementation
//*****************************************************************************************


TModifierAssistants::TModifierAssistants():
  sensitiveLength(50.0) { }


void
TModifierAssistants::findGuidelines(const TPointD &position, TGuidelineList &outGuidelines) const {
  if (TInputManager *manager = getManager())
  if (TApplication *application = manager->getApplication())
  if (TTool *tool = manager->getTool())
  if (TFrameHandle *frameHandle = application->getCurrentFrame())
  if (TXsheetHandle *XsheetHandle = application->getCurrentXsheet())
  if (TXsheet *Xsheet = XsheetHandle->getXsheet())
  {
    int frame = frameHandle->getFrame();
    int count = Xsheet->getColumnCount();
    TAffine worldToTrack = manager->worldToTool();

    for(int i = 0; i < count; ++i)
      if (TImageP image = Xsheet->getCell(frame, i).getImage(false))
      if (image->getType() == TImage::META)
      if (TMetaImage *metaImage = dynamic_cast<TMetaImage*>(image.getPointer()))
      {
        TAffine imageToTrack = tool->getColumnMatrix(i)
                             * worldToTrack;
        TMetaImage::Reader reader(*metaImage);

        for(TMetaObjectRefList::const_iterator i = reader->begin(); i != reader->end(); ++i)
          if (*i)
          if (const TAssistant *assistant = (*i)->getHandler<TAssistant>())
            assistant->getGuidelines(position, imageToTrack, outGuidelines);
      }
  }
}


void
TModifierAssistants::modifyTrack(
  const TTrack &track,
  const TInputSavePoint::Holder &savePoint,
  TTrackList &outTracks )
{
  if (!track.handler) {
    track.handler = new TTrackHandler(track);
    Modifier *modifier = new Modifier(*track.handler);
    findGuidelines(track[0].position, modifier->guidelines);

    track.handler->tracks.push_back(new TTrack(modifier));

    if ((int)modifier->guidelines.size() > 1) {
      modifier->savePoint = savePoint;
      outTracks.push_back(track.handler->tracks.front());
      return;
    }
  }

  outTracks.push_back(track.handler->tracks.front());
  TTrack &subTrack = *track.handler->tracks.front();
  if (!track.changed()) return;
  if (Modifier *modifier = dynamic_cast<Modifier*>(subTrack.modifier.getPointer())) {
    // remove points
    int start = track.size() - track.pointsAdded;
    if (start < 0) start = 0;

    if ((int)modifier->guidelines.size() > 1 && modifier->savePoint.available()) {
      // select guideline
      bool longEnough = false;
      if (TInputManager *manager = getManager()) {
        if (TToolViewer *viewer = manager->getViewer()) {
          TAffine trackToScreen = manager->toolToWorld()
                                * viewer->get3dViewMatrix().get2d().inv();
          TGuidelineP guideline = TGuideline::findBest(modifier->guidelines, track, trackToScreen, longEnough);
          if (guideline != modifier->guidelines.front())
            for(int i = 1; i < (int)modifier->guidelines.size(); ++i)
              if (modifier->guidelines[i] == guideline) {
                std::swap(modifier->guidelines[i], modifier->guidelines.front());
                start = 0;
                break;
              }
        }
      }
      modifier->savePoint.setLock(!longEnough);
    } else {
      modifier->savePoint.reset();
    }

    // add points
    subTrack.truncate(start);
    for(int i = start; i < track.size(); ++i)
      subTrack.push_back( modifier->calcPoint(i) );
  }
  track.resetChanges();
}


void
TModifierAssistants::drawHover(const TPointD &hover) {
  TGuidelineList guidelines;
  findGuidelines(hover, guidelines);
  for(TGuidelineList::const_iterator i = guidelines.begin(); i != guidelines.end(); ++i)
    (*i)->draw();
}


void
TModifierAssistants::drawTrack(const TTrack &track) {
  if (!track.handler) return;
  TTrack &subTrack = *track.handler->tracks.front();
  if (Modifier *modifier = dynamic_cast<Modifier*>(subTrack.modifier.getPointer())) {
    const TGuidelineList &guidelines = modifier->guidelines;
    if (!guidelines.empty()) {
      guidelines.front()->draw(true);
      for(TGuidelineList::const_iterator i = guidelines.begin() + 1; i != guidelines.end(); ++i)
        (*i)->draw();
    }
  }
}
