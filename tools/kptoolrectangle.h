
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

#ifndef __kptoolrectangle_h__
#define __kptoolrectangle_h__

#include <qbrush.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qrect.h>
#include <kcommand.h>
#include <kptool.h>

class kpMainWindow;
class kpToolWidgetFillStyle;
class kpToolWidgetLineStyle;
class kpToolWidgetLineWidth;
class kpViewManager;

class kpToolRectangle : public kpTool
{
Q_OBJECT

public:
    kpToolRectangle (kpMainWindow *);
    virtual ~kpToolRectangle ();

    // it turns out that these shapes are all really the same thing
    // (same options, same feel) - the only real difference is the
    // drawing functions (a one line change)
    enum Mode {Rectangle, RoundedRectangle, Ellipse};
    void setMode (Mode mode);

    virtual bool careAboutModifierState () const { return true; }

    virtual void begin ();
    virtual void end ();

    virtual void beginDraw ();
    virtual void draw (const QPoint &, const QPoint &, const QRect &);
    virtual void cancelDraw ();
    virtual void endDraw (const QPoint &, const QRect &);

private slots:
    void updatePens ();
    void updateBrushes ();

    virtual void slotForegroundColorChanged (const QColor &);
    virtual void slotBackgroundColorChanged (const QColor &);
    
private:
    Mode m_mode;
    
    kpToolWidgetLineStyle *m_toolWidgetLineStyle;
    kpToolWidgetLineWidth *m_toolWidgetLineWidth;
    kpToolWidgetFillStyle *m_toolWidgetFillStyle;

    QPen pen (int mouseButton) const;
    QPen m_pen [2];
    
    QBrush brush (int mouseButton) const;
    QBrush m_brush [2];
    
    void applyModifiers ();
    QPoint m_toolRectangleStartPoint, m_toolRectangleEndPoint;
    QRect m_toolRectangleRect;
};

class kpToolRectangleCommand : public KCommand
{
public:
    kpToolRectangleCommand (kpDocument *document, kpViewManager *viewManager,
                            kpToolRectangle::Mode mode,
                            const QPen &pen, const QBrush &brush,
                            const QRect &rect,
                            const QPoint &startPoint, const QPoint &endPoint);
    virtual ~kpToolRectangleCommand ();

    virtual void execute ();
    virtual void unexecute ();

    virtual QString name () const;

private:
    kpDocument *m_document;
    kpViewManager *m_viewManager;
    kpToolRectangle::Mode m_mode;
    QPen m_pen;
    QBrush m_brush;
    QRect m_rect;
    QPoint m_startPoint, m_endPoint;
    QPixmap *m_oldPixmapPtr;
};

#endif  // __kptoolrectangle_h__
