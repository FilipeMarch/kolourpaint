
/* This file is part of the KolourPaint project
   Copyright (c) 2003 Clarence Dang <dang@kde.org>
   All rights reserved.
   
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the names of the copyright holders nor the names of
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpoint.h>
#include <qrect.h>

#include <kdebug.h>
#include <klocale.h>
#include <knuminput.h>

#include <kpdefs.h>
#include <kpdocument.h>
#include <kpmainwindow.h>
#include <kptoolresizescale.h>


/*
 * kpToolResizeScaleCommand
 */

kpToolResizeScaleCommand::kpToolResizeScaleCommand (kpDocument *document, kpViewManager *viewManager,
                                                    int newWidth, int newHeight,
                                                    bool scaleToFit)
    : m_document (document), m_viewManager (viewManager),
      m_newWidth (newWidth), m_newHeight (newHeight),
      m_scaleToFit (scaleToFit)
{
    m_oldWidth = document->width ();
    m_oldHeight = document->height ();

    m_isLosslessScale = scaleToFit &&
                          (m_newWidth / m_oldWidth * m_oldWidth == m_newWidth) &&
                          (m_newHeight / m_oldHeight * m_oldHeight == m_newHeight);
}

// virtual
QString kpToolResizeScaleCommand::name () const
{
    QString name;

    if (m_scaleToFit)
        name = i18n ("Scale");
    else
        name = i18n ("Resize");

    return i18n ("%1 from %2 x %3 to %4 x %5")
            .arg (name)
            .arg (m_oldWidth).arg (m_oldHeight)
            .arg (m_newWidth).arg (m_newHeight);
}

kpToolResizeScaleCommand::~kpToolResizeScaleCommand ()
{
}

void kpToolResizeScaleCommand::execute ()
{
    if (m_oldWidth == m_newWidth && m_oldHeight == m_oldHeight)
        return;

    if (!m_scaleToFit)
    {
        if (m_newWidth < m_oldWidth)
        {
            m_oldRightPixmap = m_document->getPixmapAt (
                QRect (m_newWidth, 0,
                       m_oldWidth - m_newWidth, m_oldHeight));
        }

        if (m_newHeight < m_oldHeight)
        {
            m_oldBottomPixmap = m_document->getPixmapAt (
                QRect (0, m_newHeight,
                       m_newWidth, m_oldHeight - m_newHeight));
        }

        m_document->resize (m_newWidth, m_newHeight);
    }
    else
    {
        if (!m_isLosslessScale)
            m_oldPixmap = *m_document->pixmap ();

        m_document->scale (m_newWidth, m_newHeight);
    }
}

void kpToolResizeScaleCommand::unexecute ()
{
    if (!m_scaleToFit)
    {
        m_document->resize (m_oldWidth, m_oldHeight,
                            false /* don't paint new areas white */);

        if (m_newWidth < m_oldWidth)
            m_document->setPixmapAt (m_oldRightPixmap, QPoint (m_newWidth, 0));

        if (m_newHeight < m_oldHeight)
            m_document->setPixmapAt (m_oldBottomPixmap, QPoint (0, m_newHeight));
    }
    else
    {
        if (!m_isLosslessScale)
            m_document->setPixmap (m_oldPixmap);
        else
            m_document->scale (m_oldWidth, m_oldHeight);
    }
}


/*
 * kpToolResizeScaleDialog
 */

kpToolResizeScaleDialog::kpToolResizeScaleDialog (kpMainWindow *mainWindow)
    : KDialogBase ((QWidget *) mainWindow, 0/*name*/, true/*modal*/, i18n ("Resize / Scale Image"),
                   KDialogBase::Ok | KDialogBase::Cancel)
{
    kpDocument *document = mainWindow->document ();
    m_oldWidth = document->width ();
    m_oldHeight = document->height ();

    QWidget *page = new QWidget (this);
    setMainWidget (page);

    QGridLayout *lay = new QGridLayout (page, 6, 6, marginHint (), spacingHint ());

    QLabel *lbCurrentWidth = new QLabel (i18n ("Current Width: "), page);
    lay->addWidget (lbCurrentWidth, 0, 0);

    QLabel *lbCurrentWidthVal = new QLabel (QString::number (m_oldWidth), page);
    lbCurrentWidthVal->setAlignment (AlignRight | AlignVCenter | ExpandTabs);  // SYNC
    lay->addWidget (lbCurrentWidthVal, 0, 1);

    QLabel *lbCurrentHeight = new QLabel (i18n ("Current Height: "), page);
    lay->addWidget (lbCurrentHeight, 0, 3);

    QLabel *lbCurrentHeightVal = new QLabel (QString::number (m_oldHeight), page);
    lbCurrentHeightVal->setAlignment (AlignRight | AlignVCenter | ExpandTabs);  // SYNC
    lay->addWidget (lbCurrentHeightVal, 0, 4);

    QLabel *lbWidth = new QLabel (i18n ("&Width: "), page);
    lay->addWidget (lbWidth, 1, 0);

    m_inpWidthVal = new KIntNumInput (page);
    m_inpWidthVal->setValue (document->width ());
    lay->addWidget (m_inpWidthVal, 1, 1);

    lbWidth->setBuddy (m_inpWidthVal);

    QLabel *lbHeight = new QLabel (i18n ("&Height: "), page);
    lay->addWidget (lbHeight, 1, 3);

    m_inpHeightVal = new KIntNumInput (page);
    m_inpHeightVal->setValue (document->height ());
    lay->addWidget (m_inpHeightVal, 1, 4);

    m_inpWidthPercentVal = new KDoubleNumInput (page);
    m_inpWidthPercentVal->setPrecision (2);
    m_inpWidthPercentVal->setValue (100);
    lay->addWidget (m_inpWidthPercentVal, 2, 1);

    lbHeight->setBuddy (m_inpHeightVal);

    QLabel *lbWidthPercent = new QLabel (i18n ("%"), page);
    lay->addWidget (lbWidthPercent, 2, 2);

    m_inpHeightPercentVal = new KDoubleNumInput (page);
    m_inpHeightPercentVal->setPrecision (2);
    m_inpHeightPercentVal->setValue (100);
    lay->addWidget (m_inpHeightPercentVal, 2, 4);

    QLabel *lbHeightPercent = new QLabel (i18n ("%"), page);
    lay->addWidget (lbHeightPercent, 2, 5);

    m_cbScaleToFit = new QCheckBox (i18n ("&Scale contents to new size"), page);
    lay->addMultiCellWidget (m_cbScaleToFit, 4, 4, 0, 5);

    m_cbLockAspectRatio = new QCheckBox (i18n ("Keep &aspect ratio"), page);
    lay->addMultiCellWidget (m_cbLockAspectRatio, 5, 5, 0, 5);

    connect (m_inpWidthVal, SIGNAL (valueChanged (int)), SLOT (slotWidthChanged (int)));
    connect (m_inpHeightVal, SIGNAL (valueChanged (int)), SLOT (slotHeightChanged (int)));

    connect (m_inpWidthPercentVal, SIGNAL (valueChanged (double)), SLOT (slotWidthPercentChanged (double)));
    connect (m_inpHeightPercentVal, SIGNAL (valueChanged (double)), SLOT (slotHeightPercentChanged (double)));

    connect (m_cbLockAspectRatio, SIGNAL (toggled (bool)), SLOT (slotLockAspectRatioToggled (bool)));

    m_dontAdjustAspectRatio = false;
    enableButtonOK (false);  // currently noop
}

kpToolResizeScaleDialog::~kpToolResizeScaleDialog ()
{
}


void kpToolResizeScaleDialog::slotWidthChanged (int width)
{
    // update %
    m_inpWidthPercentVal->blockSignals (false);
    m_inpWidthPercentVal->setValue (double (width) * 100.0 / double (m_oldWidth));
    m_inpWidthPercentVal->blockSignals (false);

    widthFitHeightToAspectRatio (width);
    enableButtonOK (!isNoop ());
}

void kpToolResizeScaleDialog::slotHeightChanged (int height)
{
    // update %
    m_inpHeightPercentVal->blockSignals (true);
    m_inpHeightPercentVal->setValue (double (height) * 100.0 / double (m_oldHeight));
    m_inpHeightPercentVal->blockSignals (false);

    heightFitWidthToAspectRatio (height);
    enableButtonOK (!isNoop ());
}

void kpToolResizeScaleDialog::slotWidthPercentChanged (double widthPercent)
{
    // update width val
    m_inpWidthVal->blockSignals (true);
    m_inpWidthVal->setValue (int (ceil (double (m_oldWidth) * double (widthPercent) / 100.0)));
    m_inpWidthVal->blockSignals (false);

    widthFitHeightToAspectRatio (imageWidth ());
    enableButtonOK (!isNoop ());
}

void kpToolResizeScaleDialog::slotHeightPercentChanged (double heightPercent)
{
    // update height val
    m_inpHeightVal->blockSignals (true);
    m_inpHeightVal->setValue (int (ceil (double (m_oldHeight) * double (heightPercent) / 100.0)));
    m_inpHeightVal->blockSignals (false);

    heightFitWidthToAspectRatio (imageHeight ());
    enableButtonOK (!isNoop ());
}

void kpToolResizeScaleDialog::slotLockAspectRatioToggled (bool on)
{
    if (on)
    {
        m_dontAdjustAspectRatio = false;
        widthFitHeightToAspectRatio (imageWidth ());
    }
}


// not really a mutex but :)
#define KP_ASPECT_MUTEX_BEGIN if (!m_dontAdjustAspectRatio && m_cbLockAspectRatio->isChecked ()) \
                              { \
                                  m_dontAdjustAspectRatio = true
#define KP_ASPECT_MUTEX_END   m_dontAdjustAspectRatio = false; }

void kpToolResizeScaleDialog::widthFitHeightToAspectRatio (int width)
{
KP_ASPECT_MUTEX_BEGIN;
    // update height
    // m_oldWidth / m_oldHeight = width / height
    // height * m_oldWidth / m_oldHeight = width
    // height = width * m_oldHeight / m_oldWidth
    m_inpHeightVal->setValue (int (ceil (double (width) * double (m_oldHeight) / double (m_oldWidth))));
KP_ASPECT_MUTEX_END;
}

void kpToolResizeScaleDialog::heightFitWidthToAspectRatio (int height)
{
KP_ASPECT_MUTEX_BEGIN;
    // update width
    // m_oldWidth / m_oldHeight = width / height
    // width = m_oldWidth * height / m_oldHeight
    m_inpWidthVal->setValue (int (ceil (double (m_oldWidth) * double (height) / double (m_oldHeight))));
KP_ASPECT_MUTEX_END;
}


int kpToolResizeScaleDialog::imageWidth () const
{
    return m_inpWidthVal->value ();
}

int kpToolResizeScaleDialog::imageHeight () const
{
    return m_inpHeightVal->value ();
}

bool kpToolResizeScaleDialog::scaleToFit () const
{
    return m_cbScaleToFit->isChecked ();
}

bool kpToolResizeScaleDialog::isNoop () const
{
    return imageWidth () == m_oldWidth && imageHeight () == m_oldHeight;
}

#include <kptoolresizescale.moc>
