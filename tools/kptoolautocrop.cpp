
/*
   Copyright (c) 2003-2004 Clarence Dang <dang@kde.org>
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


#define DEBUG_KP_TOOL_AUTO_CROP 1

#include <qapplication.h>
#include <qbitmap.h>
#include <qimage.h>
#include <qpainter.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kpcommandhistory.h>
#include <kpdocument.h>
#include <kpmainwindow.h>
#include <kppixmapfx.h>
#include <kpselection.h>
#include <kptool.h>
#include <kptoolautocrop.h>
#include <kpviewmanager.h>


kpToolAutoCropBorder::kpToolAutoCropBorder (const QPixmap *pixmapPtr)
    : m_pixmapPtr (pixmapPtr)
{
}

bool kpToolAutoCropBorder::calculate (int isX, int dir)
{
#if DEBUG_KP_TOOL_AUTO_CROP && 1
    kdDebug () << "kpToolAutoCropBorder::calculate() CALLED!" << endl;
#endif
    int maxX = m_pixmapPtr->width () - 1;
    int maxY = m_pixmapPtr->height () - 1;

    QImage image = kpPixmapFX::convertToImage (*m_pixmapPtr);
    if (image.isNull ())
    {
        kdError () << "Border::calculate() could not convert to QImage" << endl;
        return false;
    }

    // (sync both branches)
    if (isX)
    {
        int numCols = 0;
        int startX = (dir > 0) ? 0 : maxX;

        QColor col = kpPixmapFX::getColorAtPixel (image, startX, 0);
        for (int x = startX;
             x >= 0 && x <= maxX;
             x += dir)
        {
            int y;
            for (y = 0; y <= maxY; y++)
            {
                if (!kpTool::colorEq (kpPixmapFX::getColorAtPixel (image, x, y), col))
                    break;
            }

            if (y <= maxY)
                break;
            else
                numCols++;
        }

        if (numCols)
        {
            m_rect = QRect (QPoint (startX, 0),
                            QPoint (startX + (numCols - 1) * dir, maxY)).normalize ();
            m_color = col;
        }
    }
    else
    {
        int numRows = 0;
        int startY = (dir > 0) ? 0 : maxY;

        QColor col = kpPixmapFX::getColorAtPixel (image, 0, startY);
        for (int y = startY;
             y >= 0 && y <= maxY;
             y += dir)
        {
            int x;
            for (x = 0; x <= maxX; x++)
            {
                if (!kpTool::colorEq (kpPixmapFX::getColorAtPixel (image, x, y), col))
                    break;
            }

            if (x <= maxX)
                break;
            else
                numRows++;
        }

        if (numRows)
        {
            m_rect = QRect (QPoint (0, startY),
                            QPoint (maxX, startY + (numRows - 1) * dir)).normalize ();
            m_color = col;
        }
    }

    return true;
}

bool kpToolAutoCropBorder::fillsEntirePixmap () const
{
    return m_rect == m_pixmapPtr->rect ();
}

bool kpToolAutoCropBorder::exists () const
{
    // (will use in an addition so make sure returns 1 or 0)
    return m_rect.isValid () ? 1 : 0;
}

void kpToolAutoCropBorder::invalidate ()
{
    m_rect = QRect ();
    m_color = QColor ();  // transparent
}


bool kpToolAutoCrop (kpMainWindow *mainWindow)
{
#if DEBUG_KP_TOOL_AUTO_CROP
    kdDebug () << "kpToolAutoCrop() CALLED!" << endl;
#endif

    bool nothingToCrop = false;

    if (!mainWindow)
    {
        kdError () << "kpToolAutoCrop() passed NULL mainWindow" << endl;
        return false;
    }

    kpDocument *doc = mainWindow->document ();
    if (!doc)
    {
        kdError () << "kpToolAutoCrop() passed NULL document" << endl;
        return false;
    }

    // OPT: if already pulled selection pixmap, no need to do it again here
    QPixmap pixmap = doc->selection () ? doc->getSelectedPixmap () : *doc->pixmap ();
    if (pixmap.isNull ())
    {
        kdError () << "kptoolAutoCrop() pased NULL pixmap" << endl;
        return false;
    }

    kpViewManager *vm = mainWindow->viewManager ();
    if (!vm)
    {
        kdError () << "kpToolAutoCrop() passed NULL vm" << endl;
        return false;
    }

    kpToolAutoCropBorder leftBorder (&pixmap), rightBorder (&pixmap),
                         topBorder (&pixmap), botBorder (&pixmap);

    // sync: restoreOverrideCursor() for all exit paths
    QApplication::setOverrideCursor (Qt::waitCursor);

    if (!leftBorder.calculate (true/*x*/, +1/*going right*/))
    {
        QApplication::restoreOverrideCursor ();
        return false;
    }

    nothingToCrop = leftBorder.fillsEntirePixmap ();

#if DEBUG_KP_TOOL_AUTO_CROP
    if (nothingToCrop)
        kdDebug () << "\tleft border filled entire pixmap - nothing to crop" << endl;
#endif

    if (!nothingToCrop)
    {
        if (!rightBorder.calculate (true/*x*/, -1/*going left*/) ||
            !topBorder.calculate (false/*y*/, +1/*going down*/) ||
            !botBorder.calculate (false/*y*/, -1/*going up*/))
        {
            QApplication::restoreOverrideCursor ();
            return false;
        }

        int numRegions = leftBorder.exists () +
                         rightBorder.exists () +
                         topBorder.exists () +
                         botBorder.exists ();
        nothingToCrop = !numRegions;

    #if DEBUG_KP_TOOL_AUTO_CROP
        kdDebug () << "\tnumRegions=" << numRegions << endl;
        kdDebug () << "\t\tleft=" << leftBorder.m_rect << endl;
        kdDebug () << "\t\tright=" << rightBorder.m_rect << endl;
        kdDebug () << "\t\ttop=" << topBorder.m_rect << endl;
        kdDebug () << "\t\tbot=" << botBorder.m_rect << endl;
    #endif

        if (numRegions == 2)
        {
        #if DEBUG_KP_TOOL_AUTO_CROP
            kdDebug () << "\t2 regions:" << endl;
        #endif

            // in case e.g. the user pastes a solid, coloured-in rectangle,
            // we favour killing the bottom and right regions
            // (these regions probably contain the unwanted whitespace due
            //  to the doc being bigger than the pasted selection to start with)

            if (leftBorder.exists () && rightBorder.exists () &&
                !kpTool::colorEq (leftBorder.m_color, rightBorder.m_color))
            {
            #if DEBUG_KP_TOOL_AUTO_CROP
                kdDebug () << "\t\tignoring left border" << endl;
            #endif
                leftBorder.invalidate ();
            }
            else if (topBorder.exists () && botBorder.exists () &&
                     !kpTool::colorEq (topBorder.m_color, botBorder.m_color))
            {
            #if DEBUG_KP_TOOL_AUTO_CROP
                kdDebug () << "\t\tignoring top border" << endl;
            #endif
                topBorder.invalidate ();
            }
        #if DEBUG_KP_TOOL_AUTO_CROP
            else
            {
                kdDebug () << "\t\tok" << endl;
            }
        #endif
        }
    }

#if DEBUG_KP_TOOL_AUTO_CROP
    kdDebug () << "\tnothingToCrop=" << nothingToCrop << endl;
#endif

    if (!nothingToCrop)
    {
        mainWindow->addImageOrSelectionCommand (
            new kpToolAutoCropCommand (
                (bool) doc->selection (),
                leftBorder, rightBorder,
                topBorder, botBorder,
                mainWindow));
    }

    QApplication::restoreOverrideCursor ();

    if (nothingToCrop)
    {
        KMessageBox::information (mainWindow,
            i18n ("Autocrop could not find any border to remove."),
            i18n ("Nothing to Autocrop"),
            "DoNotAskAgain_NothingToAutoCrop");
    }

    return true;
}


kpToolAutoCropCommand::kpToolAutoCropCommand (bool actOnSelection,
                                              const kpToolAutoCropBorder &leftBorder,
                                              const kpToolAutoCropBorder &rightBorder,
                                              const kpToolAutoCropBorder &topBorder,
                                              const kpToolAutoCropBorder &botBorder,
                                              kpMainWindow *mainWindow)
    : m_actOnSelection (actOnSelection),
      m_leftBorder (leftBorder),
      m_rightBorder (rightBorder),
      m_topBorder (topBorder),
      m_botBorder (botBorder),
      m_mainWindow (mainWindow)
{
    kpDocument *doc = document ();
    if (!doc)
    {
        kdError () << "kpToolAutoCropCommand::<ctor>() without doc" << endl;
        m_oldWidth = 0;
        m_oldHeight = 0;
        return;
    }

    m_oldWidth = doc->width (m_actOnSelection);
    m_oldHeight = doc->height (m_actOnSelection);
}

// public virtual [base KCommand]
QString kpToolAutoCropCommand::name () const
{
    return i18n ("Autocrop");
}

kpToolAutoCropCommand::~kpToolAutoCropCommand ()
{
}


// private
kpDocument *kpToolAutoCropCommand::document () const
{
    return m_mainWindow ? m_mainWindow->document () : 0;
}

// private
kpViewManager *kpToolAutoCropCommand::viewManager () const
{
    return m_mainWindow ? m_mainWindow->viewManager () : 0;
}


// public virtual [base KCommand]
void kpToolAutoCropCommand::execute ()
{
    if (!m_contentsRect.isValid ())
        m_contentsRect = contentsRect ();

    kpDocument *doc = document ();
    if (!doc)
        return;

    QPixmap pixmapWithoutBorder =
        kpTool::neededPixmap (*doc->pixmap (m_actOnSelection),
                              m_contentsRect);

    kpViewManager *vm = viewManager ();
    if (vm)
        vm->setQueueUpdates ();

    doc->setPixmap (m_actOnSelection, pixmapWithoutBorder);

    if (m_actOnSelection)
    {
        kpSelection *sel = doc->selection ();
        if (!sel)
            return;

        sel->moveBy (m_contentsRect.x (), m_contentsRect.y ());
    }

    if (vm)
        vm->restoreQueueUpdates ();
}

// public virtual [base KCommand]
void kpToolAutoCropCommand::unexecute ()
{
#if DEBUG_KP_TOOL_AUTO_CROP && 1
    kdDebug () << "kpToolAutoCropCommand::unexecute()" << endl;
#endif

    kpDocument *doc = document ();
    if (!doc)
        return;

    QPixmap pixmap (m_oldWidth, m_oldHeight);
    QBitmap maskBitmap;

    // restore the position of the centre image
    kpPixmapFX::setPixmapAt (&pixmap, m_contentsRect,
                             *doc->pixmap (m_actOnSelection));

    // draw the borders

    QPainter painter (&pixmap);
    QPainter maskPainter;

    const kpToolAutoCropBorder *borders [] =
    {
        &m_leftBorder, &m_rightBorder,
        &m_topBorder, &m_botBorder,
        0
    };

    for (const kpToolAutoCropBorder **b = borders; *b; b++)
    {
        if ((*b)->exists ())
        {
            QColor col = (*b)->m_color;
        #if DEBUG_KP_TOOL_AUTO_CROP && 1
            kdDebug () << "\tdrawing border " << (*b)->m_rect
                       << " rgb=" << (int *) col.rgb () /* %X hack */ << endl;
        #endif

            if (kpTool::isColorOpaque (col))
            {
                painter.setPen (col);
                painter.setBrush (col);

                painter.drawRect ((*b)->m_rect);
            }
            else
            {
                if (maskBitmap.isNull ())
                {
                    // TODO: dangerous when a painter is active on pixmap?
                    maskBitmap = kpPixmapFX::getNonNullMask (pixmap);
                    maskPainter.begin (&maskBitmap);
                }

                maskPainter.setPen (Qt::color0/*transparent*/);
                maskPainter.setBrush (Qt::color0/*transparent*/);

                maskPainter.drawRect ((*b)->m_rect);
            }
        }
    }

    if (maskPainter.isActive ())
        maskPainter.end ();

    painter.end ();

    if (!maskBitmap.isNull ())
        pixmap.setMask (maskBitmap);


    kpViewManager *vm = viewManager ();
    if (vm)
        vm->setQueueUpdates ();

    if (m_actOnSelection)
    {
        kpSelection *sel = doc->selection ();
        if (!sel)
            return;

        sel->moveBy (-m_contentsRect.x (), -m_contentsRect.y ());
    }

    doc->setPixmap (m_actOnSelection, pixmap);

    if (vm)
        vm->restoreQueueUpdates ();
}


// private
QRect kpToolAutoCropCommand::contentsRect () const
{
    QPixmap *pixmap = document ()->pixmap (m_actOnSelection);

    QPoint topLeft (m_leftBorder.exists () ?
                        m_leftBorder.m_rect.right () + 1 :
                        0,
                    m_topBorder.exists () ?
                        m_topBorder.m_rect.bottom () + 1 :
                        0);
    QPoint botRight (m_rightBorder.exists () ?
                         m_rightBorder.m_rect.left () - 1 :
                         pixmap->width () - 1,
                     m_botBorder.exists () ?
                         m_botBorder.m_rect.top () - 1 :
                         pixmap->height () - 1);

    return QRect (topLeft, botRight);
}
