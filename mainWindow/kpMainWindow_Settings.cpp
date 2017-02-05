
/*
   Copyright (c) 2003-2007 Clarence Dang <dang@kde.org>
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


#include "mainWindow/kpMainWindow.h"
#include "kpMainWindowPrivate.h"
#include "kpLogCategories.h"

#include <kactioncollection.h>
#include <KSharedConfig>
#include <kconfiggroup.h>
#include <kshortcutsdialog.h>
#include <kstandardaction.h>
#include <ktogglefullscreenaction.h>
#include <KLocalizedString>

#include "kpDefs.h"
#include "document/kpDocument.h"
#include "tools/kpToolAction.h"
#include "widgets/toolbars/kpToolToolBar.h"
#include "environments/tools/kpToolEnvironment.h"

//---------------------------------------------------------------------

// private
void kpMainWindow::setupSettingsMenuActions ()
{
    KActionCollection *ac = actionCollection ();


    // Settings/Toolbars |> %s
    setStandardToolBarMenuEnabled (true);

    // Settings/Show Statusbar
    createStandardStatusBarAction ();


    d->actionFullScreen = KStandardAction::fullScreen (this, SLOT (slotFullScreen ()),
                                                      this/*window*/, ac);


    d->actionShowPath = ac->add<KToggleAction> ("settings_show_path");
    d->actionShowPath->setText (i18n ("Show &Path"));
    connect(d->actionShowPath, SIGNAL(triggered(bool) ), SLOT (slotShowPathToggled ()));
    slotEnableSettingsShowPath ();

    KToggleAction *action = ac->add<KToggleAction>("settings_draw_antialiased");
    action->setText(i18n("Draw Anti-Aliased"));
    action->setChecked(kpToolEnvironment::drawAntiAliased);
    connect(action, SIGNAL(triggered(bool)), SLOT(slotDrawAntiAliasedToggled(bool)));

    d->actionKeyBindings = KStandardAction::keyBindings (this, SLOT (slotKeyBindings ()), ac);

    KStandardAction::configureToolbars(this, SLOT(configureToolbars()), actionCollection());

    enableSettingsMenuDocumentActions (false);
}

//---------------------------------------------------------------------

// private
void kpMainWindow::enableSettingsMenuDocumentActions (bool /*enable*/)
{
}

//---------------------------------------------------------------------

// private slot
void kpMainWindow::slotFullScreen ()
{
    KToggleFullScreenAction::setFullScreen( this, d->actionFullScreen->isChecked ());
}

//---------------------------------------------------------------------

// private slot
void kpMainWindow::slotEnableSettingsShowPath ()
{
#if DEBUG_KP_MAIN_WINDOW
    qCDebug(kpLogMainWindow) << "kpMainWindow::slotEnableSettingsShowPath()";
#endif

    const bool enable = (d->document && !d->document->url ().isEmpty ());

    d->actionShowPath->setEnabled (enable);
    d->actionShowPath->setChecked (enable && d->configShowPath);
}

//---------------------------------------------------------------------

// private slot
void kpMainWindow::slotShowPathToggled ()
{
#if DEBUG_KP_MAIN_WINDOW
    qCDebug(kpLogMainWindow) << "kpMainWindow::slotShowPathToggled()";
#endif

    d->configShowPath = d->actionShowPath->isChecked ();

    slotUpdateCaption ();


    KConfigGroup cfg (KSharedConfig::openConfig (), kpSettingsGroupGeneral);

    cfg.writeEntry (kpSettingShowPath, d->configShowPath);
    cfg.sync ();
}

//---------------------------------------------------------------------

void kpMainWindow::slotDrawAntiAliasedToggled(bool on)
{
    kpToolEnvironment::drawAntiAliased = on;

    KConfigGroup cfg(KSharedConfig::openConfig(), kpSettingsGroupGeneral);

    cfg.writeEntry(kpSettingDrawAntiAliased, kpToolEnvironment::drawAntiAliased);
    cfg.sync();
}

//---------------------------------------------------------------------

// private slot
void kpMainWindow::slotKeyBindings ()
{
    toolEndShape ();

    if (KShortcutsDialog::configure (actionCollection (),
            KShortcutsEditor::LetterShortcutsAllowed,
            this))
    {
        // TODO: PROPAGATE: thru mainWindow's and interprocess
    }
}

//---------------------------------------------------------------------
