
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


#include <qfile.h>

#include <dcopclient.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kimageio.h>
#include <klocale.h>

// for srand
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <kpdefs.h>
#include <kpmainwindow.h>

#include <kolourpaintlicense.h>


static const KCmdLineOptions cmdLineOptions [] =
{
    {"+[file]", I18N_NOOP ("Image file to open"), 0},
    KCmdLineLastOption
};


int main (int argc, char *argv [])
{
    KAboutData aboutData
    (
        "kolourpaint",
        I18N_NOOP("KolourPaint"),
        "1.1",  // SYNC: with VERSION
        I18N_NOOP("Paint Program for KDE"),
        KAboutData::License_Custom,
        "Copyright (c) 2003-2004 Clarence Dang",
        0,
        "http://kolourpaint.sf.net/"
    );

    // this is _not_ the same as KAboutData::License_BSD
    aboutData.setLicenseText (kpLicenseText);

    aboutData.addAuthor ("Clarence Dang", I18N_NOOP ("Maintainer"), "dang@kde.org");
    aboutData.addAuthor ("Thurston Dang", I18N_NOOP ("Chief Investigator"));

    aboutData.addCredit ("Waldo Bastian");
    aboutData.addCredit ("Stephan Binner");
    aboutData.addCredit ("Rob Buis");
    aboutData.addCredit ("Lucijan Busch");
    aboutData.addCredit ("Christoph Eckert");
    aboutData.addCredit ("David Faure");
    aboutData.addCredit ("Wilco Greven");
    aboutData.addCredit ("Werner Joss");
    aboutData.addCredit ("Stephan Kulow");
    aboutData.addCredit ("Anders Lund");
    aboutData.addCredit ("Dirk Mueller");
    aboutData.addCredit ("Ralf Nolden");
    aboutData.addCredit ("Boudewijn Rempt");
    aboutData.addCredit ("Dirk Schönberger");
    aboutData.addCredit ("Peter Simonsson");


    KCmdLineArgs::init (argc, argv, &aboutData);
    KCmdLineArgs::addCmdLineOptions (cmdLineOptions);

    KApplication app;


    // mainly for changing wallpaper :)
    DCOPClient *client = app.dcopClient ();
    if (!client->attach ())
        kdError () << "Could not contact DCOP server" << endl;

    // mainly for the Spraycan Tool
    srand ((unsigned int) (getpid () + getppid ()));

    // access more formats
    KImageIO::registerFormats ();


    // Qt says this is necessary but I don't think it is...
    QObject::connect (&app, SIGNAL (lastWindowClosed ()),
                      &app, SLOT (quit ()));


    if (app.isRestored ())
        RESTORE (kpMainWindow)
    else
    {
        kpMainWindow *mainWindow;
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs ();

        if (args->count () >= 1)
        {
            for (int i = 0; i < args->count (); i++)
            {
                mainWindow = new kpMainWindow (args->url (i));
                mainWindow->show ();
            }
        }
        else
        {
            mainWindow = new kpMainWindow ();
            mainWindow->show ();
        }

        args->clear ();
    }

    return app.exec ();
}
