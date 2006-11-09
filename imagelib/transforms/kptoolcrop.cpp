
/*
   Copyright (c) 2003-2006 Clarence Dang <dang@kde.org>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define DEBUG_KP_TOOL_CROP 0


#include <kptoolcrop.h>

#include <qpixmap.h>

#include <kdebug.h>
#include <klocale.h>

#include <kpcolor.h>
#include <kpcommandhistory.h>
#include <kpdocument.h>
#include <kpmainwindow.h>
#include <kpselection.h>
#include <kptoolclearcommand.h>
#include <kptoolresizescale.h>
#include <kptoolselection.h>
#include <kpviewmanager.h>


kpSelection selectionBorderAndMovedTo0_0 (const kpSelection &sel)
{
    kpSelection borderSel = sel;

    borderSel.setPixmap (QPixmap ());  // only interested in border
    borderSel.moveTo (QPoint (0, 0));

    return borderSel;
}


//
// kpToolCropSetImageCommand
//

class kpToolCropSetImageCommand : public kpCommand
{
public:
    kpToolCropSetImageCommand (kpMainWindow *mainWindow);
    virtual ~kpToolCropSetImageCommand ();

    /* (uninteresting child of macro cmd) */
    virtual QString name () const { return QString::null; }

    virtual int size () const
    {
        return kpPixmapFX::pixmapSize (m_oldPixmap) +
               kpPixmapFX::selectionSize (m_fromSelection) +
               kpPixmapFX::pixmapSize (m_pixmapIfFromSelectionDoesntHaveOne);
    }

    virtual void execute ();
    virtual void unexecute ();

protected:
    kpColor m_backgroundColor;
    QPixmap m_oldPixmap;
    kpSelection m_fromSelection;
    QPixmap m_pixmapIfFromSelectionDoesntHaveOne;
};


kpToolCropSetImageCommand::kpToolCropSetImageCommand (kpMainWindow *mainWindow)
    : kpCommand (mainWindow),
      m_backgroundColor (mainWindow->backgroundColor ()),
      m_fromSelection (*mainWindow->document ()->selection ()),
      m_pixmapIfFromSelectionDoesntHaveOne (
        m_fromSelection.pixmap () ?
            QPixmap () :
            mainWindow->document ()->getSelectedPixmap ())
{
}

kpToolCropSetImageCommand::~kpToolCropSetImageCommand ()
{
}


// public virtual [base kpCommand]
void kpToolCropSetImageCommand::execute ()
{
#if DEBUG_KP_TOOL_CROP
    kDebug () << "kpToolCropSetImageCommand::execute()" << endl;
#endif

    viewManager ()->setQueueUpdates ();
    {
        m_oldPixmap = kpPixmapFX::getPixmapAt (*document ()->pixmap (),
            QRect (0, 0, m_fromSelection.width (), m_fromSelection.height ()));


        //
        // Original rounded rectangle selection:
        //
        //      T/---\      ...............
        //      | TT |      T = Transparent
        //      T\__/T      ...............
        //
        // After Crop Outside the Selection, the _image_ becomes:
        //
        //      Bttttt
        //      ttTTtt      T,t = Transparent
        //      BttttB      B = Background Colour
        //
        // The selection pixmap stays the same.
        //

        QPixmap newDocPixmap (m_fromSelection.width (), m_fromSelection.height ());
        kpPixmapFX::fill (&newDocPixmap, m_backgroundColor);

    #if DEBUG_KP_TOOL_CROP
        kDebug () << "\tsel: rect=" << m_fromSelection.boundingRect ()
                   << " pm=" << m_fromSelection.pixmap ()
                   << endl;
    #endif
        QPixmap selTransparentPixmap;

        if (m_fromSelection.pixmap ())
        {
            selTransparentPixmap = m_fromSelection.transparentPixmap ();
        #if DEBUG_KP_TOOL_CROP
            kDebug () << "\thave pixmap; rect="
                       << selTransparentPixmap.rect ()
                       << endl;
        #endif
        }
        else
        {
            selTransparentPixmap = m_pixmapIfFromSelectionDoesntHaveOne;
        #if DEBUG_KP_TOOL_CROP
            kDebug () << "\tno pixmap in sel - get it; rect="
                       << selTransparentPixmap.rect ()
                       << endl;
        #endif
        }

        kpPixmapFX::paintMaskTransparentWithBrush (&newDocPixmap,
            QPoint (0, 0),
            m_fromSelection.maskForOwnType ());

        kpPixmapFX::paintPixmapAt (&newDocPixmap,
            QPoint (0, 0),
            selTransparentPixmap);


        document ()->setPixmapAt (newDocPixmap, QPoint (0, 0));
        document ()->selectionDelete ();


        Q_ASSERT (mainWindow ()->tool ());
        m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
    }
    viewManager ()->restoreQueueUpdates ();
}

// public virtual [base kpCommand]
void kpToolCropSetImageCommand::unexecute ()
{
#if DEBUG_KP_TOOL_CROP
    kDebug () << "kpToolCropSetImageCommand::unexecute()" << endl;
#endif

    viewManager ()->setQueueUpdates ();
    {
        document ()->setPixmapAt (m_oldPixmap, QPoint (0, 0));
        m_oldPixmap = QPixmap ();

    #if DEBUG_KP_TOOL_CROP
        kDebug () << "\tsel: rect=" << m_fromSelection.boundingRect ()
                   << " pm=" << m_fromSelection.pixmap ()
                   << endl;
    #endif
        document ()->setSelection (m_fromSelection);

        if (mainWindow ()->tool ())
            m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
    }
    viewManager ()->restoreQueueUpdates ();
}


//
// kpToolCropCommand
//


class kpToolCropCommand : public kpMacroCommand
{
public:
    kpToolCropCommand (kpMainWindow *mainWindow);
    virtual ~kpToolCropCommand ();
};


kpToolCropCommand::kpToolCropCommand (kpMainWindow *mainWindow)
    : kpMacroCommand (i18n ("Set as Image"), mainWindow)
{
#if DEBUG_KP_TOOL_CROP
    kDebug () << "kpToolCropCommand::<ctor>()" << endl;
#endif

    Q_ASSERT (mainWindow &&
        mainWindow->document () &&
        mainWindow->document ()->selection ());

    kpSelection *sel = mainWindow->document ()->selection ();


#if DEBUG_KP_TOOL_CROP
    kDebug () << "\tsel: w=" << sel->width ()
               << " h=" << sel->height ()
               << " <- resizing doc to these dimen" << endl;
#endif

    // (must resize doc _before_ kpToolCropSetImageCommand in case doc
    //  needs to gets bigger - else pasted down pixmap may not fit)
    addCommand (
        new kpToolResizeScaleCommand (
            false/*act on doc, not sel*/,
            sel->width (), sel->height (),
            kpToolResizeScaleCommand::Resize,
            mainWindow));


    if (sel->isText ())
    {
    #if DEBUG_KP_TOOL_CROP
        kDebug () << "\tisText" << endl;
        kDebug () << "\tclearing doc with trans cmd" << endl;
    #endif
        addCommand (
            new kpToolClearCommand (
                false/*act on doc*/,
                kpColor::Transparent,
                mainWindow));

    #if DEBUG_KP_TOOL_CROP
        kDebug () << "\tmoving sel to (0,0) cmd" << endl;
    #endif
        kpToolSelectionMoveCommand *moveCmd =
            new kpToolSelectionMoveCommand (
                QString::null/*uninteresting child of macro cmd*/,
                mainWindow);
        moveCmd->moveTo (QPoint (0, 0), true/*move on exec, not now*/);
        moveCmd->finalize ();
        addCommand (moveCmd);
    }
    else
    {
    #if DEBUG_KP_TOOL_CROP
        kDebug () << "\tis pixmap sel" << endl;
        kDebug () << "\tcreating SetImage cmd" << endl;
    #endif
        addCommand (new kpToolCropSetImageCommand (mainWindow));

    #if 0
        addCommand (
            new kpToolSelectionCreateCommand (
                QString::null/*uninteresting child of macro cmd*/,
                selectionBorderAndMovedTo0_0 (*sel),
                mainWindow));
    #endif
    }
}

kpToolCropCommand::~kpToolCropCommand ()
{
}


void kpToolCrop (kpMainWindow *mainWindow)
{
    kpDocument *doc = mainWindow->document ();
    Q_ASSERT (doc);

    kpSelection *sel = doc->selection ();
    Q_ASSERT (sel);


    bool selWasText = sel->isText ();
    kpSelection borderSel = selectionBorderAndMovedTo0_0 (*sel);


    mainWindow->addImageOrSelectionCommand (
        new kpToolCropCommand (mainWindow),
        true/*add create cmd*/,
        false/*don't add pull cmd*/);


    if (!selWasText)
    {
        mainWindow->commandHistory ()->addCommand (
            new kpToolSelectionCreateCommand (
                i18n ("Selection: Create"),
                borderSel,
                mainWindow));
    }
}
