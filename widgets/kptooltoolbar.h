
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


#ifndef __kptooltoolbar_h__
#define __kptooltoolbar_h__

#include <qvaluevector.h>
#include <ktoolbar.h>

class QBoxLayout;
class QButton;
class QButtonGroup;
class QWidget;
class QGridLayout;

class kpMainWindow;
class kpTool;

class kpToolWidgetBrush;
class kpToolWidgetEraserSize;
class kpToolWidgetFillStyle;
class kpToolWidgetLineStyle;
class kpToolWidgetLineWidth;
class kpToolWidgetSpraycanSize;

class kpToolToolBar : public KToolBar
{
Q_OBJECT

public:
    kpToolToolBar (kpMainWindow *mainWindow, int colsOrRows = 2, const char *name = 0);
    virtual ~kpToolToolBar ();

    void registerTool (kpTool *tool);
    void unregisterTool (kpTool *tool);
    void unregisterAllTools ();

    kpTool *tool () const;
    void selectTool (kpTool *tool);
    
    kpTool *previousTool () const;
    void selectPreviousTool ();
    
    void hideAllToolWidgets ();
    // could this be cleaner (the tools have to access them individually somehow)?
    kpToolWidgetBrush *toolWidgetBrush () const { return m_toolWidgetBrush; }
    kpToolWidgetEraserSize *toolWidgetEraserSize () const { return m_toolWidgetEraserSize; }
    kpToolWidgetFillStyle *toolWidgetFillStyle () const { return m_toolWidgetFillStyle; }
    kpToolWidgetLineStyle *toolWidgetLineStyle () const { return m_toolWidgetLineStyle; }
    kpToolWidgetLineWidth *toolWidgetLineWidth () const { return m_toolWidgetLineWidth; }
    kpToolWidgetSpraycanSize *toolWidgetSpraycanSize () const { return m_toolWidgetSpraycanSize; }

signals:
    void sigToolSelected (kpTool *tool);  // tool may be 0

private slots:
    void slotToolSelected ();

public slots:
    virtual void setOrientation (Qt::Orientation o);

private:
    void addButton (QButton *button, Qt::Orientation o, int num);

    Qt::Orientation m_lastDockedOrientation;
    bool m_lastDockedOrientationSet;
    int m_vertCols;

    QButtonGroup *m_buttonGroup;
    QWidget *m_baseWidget;
    QBoxLayout *m_baseLayout;
    QGridLayout *m_toolLayout;

    kpToolWidgetBrush *m_toolWidgetBrush;
    kpToolWidgetEraserSize *m_toolWidgetEraserSize;
    kpToolWidgetFillStyle *m_toolWidgetFillStyle;
    kpToolWidgetLineStyle *m_toolWidgetLineStyle;
    kpToolWidgetLineWidth *m_toolWidgetLineWidth;
    kpToolWidgetSpraycanSize *m_toolWidgetSpraycanSize;

    struct kpButtonToolPair
    {
        kpButtonToolPair (QButton *button, kpTool *tool)
            : m_button (button), m_tool (tool)
        {
        }
        
        kpButtonToolPair ()
            : m_button (0), m_tool (0)
        {
        }
        
        QButton *m_button;
        kpTool *m_tool;
    };

    QValueVector <kpButtonToolPair> m_buttonToolPairs;
    
    kpTool *m_previousTool, *m_currentTool;
};

#endif  // __kptooltoolbar_h__
