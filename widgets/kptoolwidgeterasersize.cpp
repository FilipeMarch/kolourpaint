
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


#include <qbitmap.h>
#include <qpainter.h>

#include <klocale.h>

#include <kptoolwidgeterasersize.h>

static int eraserSizes [] = {5, 9, 17, 29};
static const int numEraserSizes = int (sizeof (eraserSizes) / sizeof (eraserSizes [0]));

kpToolWidgetEraserSize::kpToolWidgetEraserSize (QWidget *parent)
    : kpToolWidgetBase (parent)
{
    setInvertSelectedPixmap ();

    m_cursorPixmaps = new QPixmap [numEraserSizes];
    QPixmap *pixmap = m_cursorPixmaps;
    
    for (int i = 0; i < numEraserSizes; i++)
    {
        int s = eraserSizes [i];
        
        pixmap->resize (s, s);
        pixmap->fill (Qt::white);
        
        QPainter painter;

        painter.begin (pixmap);
        painter.setPen (Qt::black);
        painter.setBrush (Qt::black);
        painter.drawRect (0, 0, s, s);
        painter.end ();
        
        QBitmap mask (pixmap->width (), pixmap->height ());
        mask.fill (Qt::color1);
        pixmap->setMask (mask);

        addOption (*pixmap, i18n ("%1x%2").arg (s).arg (s)/*tooltip*/);
        if (i >= 2)
            startNewOptionRow ();
        
        pixmap++;
    }

    relayoutOptions ();
    setSelected (0, 0);
}

kpToolWidgetEraserSize::~kpToolWidgetEraserSize ()
{
    delete [] m_cursorPixmaps;
}

int kpToolWidgetEraserSize::eraserSize () const
{
    return eraserSizes [selected ()];
}

QPixmap kpToolWidgetEraserSize::cursorPixmap (const QColor &color) const
{
    QPixmap pixmap = m_cursorPixmaps [selected ()];
    
    QPainter painter (&pixmap);
    painter.setPen (Qt::black);
    painter.setBrush (color);
    painter.drawRect (0, 0, pixmap.width (), pixmap.height ());
    painter.end ();

    return pixmap;
}

// virtual protected slot
void kpToolWidgetEraserSize::setSelected (int row, int col)
{
    kpToolWidgetBase::setSelected (row, col);
    emit eraserSizeChanged (eraserSize ());
};

#include <kptoolwidgeterasersize.moc>
