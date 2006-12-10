
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


#define DEBUG_KP_DOCUMENT 0


#include <kpdocument.h>

#include <math.h>

#include <qcolor.h>
#include <qbitmap.h>
#include <qbrush.h>
#include <qfile.h>
#include <qimage.h>
#include <qlist.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qrect.h>
#include <qsize.h>
#include <qmatrix.h>

#include <kdebug.h>
#include <kglobal.h>
#include <kimageio.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>  // TODO: isn't this in KIO?
#include <ktemporaryfile.h>

#include <kpcolor.h>
#include <kpcolortoolbar.h>
#include <kpdefs.h>
#include <kpdocumentsaveoptions.h>
#include <kpdocumentmetainfo.h>
#include <kpEffectReduceColors.h>
#include <kpmainwindow.h>
#include <kppixmapfx.h>
#include <kpselection.h>
#include <kptool.h>
#include <kptooltoolbar.h>
#include <kpviewmanager.h>


struct kpDocumentPrivate
{
    kpDocumentPrivate ()
    {
    }
};


kpDocument::kpDocument (int w, int h, kpMainWindow *mainWindow)
    : m_constructorWidth (w), m_constructorHeight (h),
      m_mainWindow (mainWindow),
      m_isFromURL (false),
      m_savedAtLeastOnceBefore (false),
      m_saveOptions (new kpDocumentSaveOptions ()),
      m_metaInfo (new kpDocumentMetaInfo ()),
      m_modified (false),
      m_selection (0),
      m_oldWidth (-1), m_oldHeight (-1),
      d (new kpDocumentPrivate ())
{
#if DEBUG_KP_DOCUMENT && 0
    kDebug () << "kpDocument::kpDocument (" << w << "," << h << ")" << endl;
#endif

    m_pixmap = new QPixmap (w, h);
    m_pixmap->fill (Qt::white);
}

kpDocument::~kpDocument ()
{
    delete d;

    delete m_pixmap;

    delete m_saveOptions;
    delete m_metaInfo;

    delete m_selection;
}


kpMainWindow *kpDocument::mainWindow () const
{
    return m_mainWindow;
}

void kpDocument::setMainWindow (kpMainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
}


/*
 * File I/O
 */

// public static
QPixmap kpDocument::getPixmapFromFile (const KUrl &url, bool suppressDoesntExistDialog,
                                       QWidget *parent,
                                       kpDocumentSaveOptions *saveOptions,
                                       kpDocumentMetaInfo *metaInfo)
{
#if DEBUG_KP_DOCUMENT
    kDebug () << "kpDocument::getPixmapFromFile(" << url << "," << parent << ")" << endl;
#endif

    if (saveOptions)
        *saveOptions = kpDocumentSaveOptions ();

    if (metaInfo)
        *metaInfo = kpDocumentMetaInfo ();


    QString tempFile;
    if (url.isEmpty () || !KIO::NetAccess::download (url, tempFile, parent))
    {
        if (!suppressDoesntExistDialog)
        {
            KMessageBox::sorry (parent,
                                i18n ("Could not open \"%1\".",
                                      kpDocument::prettyFilenameForURL (url)));
        }

        return QPixmap ();
    }


    // sync: remember to "KIO::NetAccess::removeTempFile (tempFile)" in all exit paths

#if 0
    QString detectedMimeType = KImageIO::mimeType (tempFile);
#else  // COMPAT: this is wrong - should be what QImage::QImage() loaded file as
    KMimeType::Ptr detectedMimeTypePtr = KMimeType::findByFileContent (tempFile);
    QString detectedMimeType =
        detectedMimeTypePtr != KMimeType::defaultMimeTypePtr () ?
            detectedMimeTypePtr->name () :
            QString::null;
#endif
    if (saveOptions)
        saveOptions->setMimeType (detectedMimeType);

#if DEBUG_KP_DOCUMENT
    kDebug () << "\ttempFile=" << tempFile << endl;
    kDebug () << "\tmimetype=" << detectedMimeType << endl;
    kDebug () << "\tsrc=" << url.path () << endl;
    // COMPAT kDebug () << "\tmimetype of src=" << KImageIO::mimeType (url.path ()) << endl;
#endif

    if (detectedMimeType.isEmpty ())
    {
        KMessageBox::sorry (parent,
                            i18n ("Could not open \"%1\" - unknown mimetype.",
                                  kpDocument::prettyFilenameForURL (url)));
        KIO::NetAccess::removeTempFile (tempFile);
        return QPixmap ();
    }


    QImage image (tempFile);
    if (image.isNull ())
    {
        KMessageBox::sorry (parent,
                            i18n ("Could not open \"%1\" - unsupported image format.\n"
                                  "The file may be corrupt.",
                                  kpDocument::prettyFilenameForURL (url)));
        KIO::NetAccess::removeTempFile (tempFile);
        return QPixmap ();
    }

#if DEBUG_KP_DOCUMENT
    kDebug () << "\timage: depth=" << image.depth ()
                << " (X display=" << QPixmap::defaultDepth () << ")"
                << " hasAlphaChannel=" << image.hasAlphaChannel ()
                << endl;
#endif

    if (saveOptions)
    {
        saveOptions->setColorDepth (image.depth ());
        saveOptions->setDither (false);  // avoid double dithering when saving
    }

    if (metaInfo)
    {
        metaInfo->setDotsPerMeterX (image.dotsPerMeterX ());
        metaInfo->setDotsPerMeterY (image.dotsPerMeterY ());
        metaInfo->setOffset (image.offset ());

        QList <QImageTextKeyLang> keyList = image.textList ();
        for (QList <QImageTextKeyLang>::const_iterator it = keyList.begin ();
             it != keyList.end ();
             it++)
        {
            metaInfo->setText (*it, image.text (*it));
        }

    #if DEBUG_KP_DOCUMENT
        metaInfo->printDebug ("\tmetaInfo");
    #endif
    }

#if DEBUG_KP_DOCUMENT && 1
{
    if (image.width () <= 16 && image.height () <= 16)
    {
        kDebug () << "Image dump:" << endl;

        for (int y = 0; y < image.height (); y++)
        {
            for (int x = 0; x < image.width (); x++)
            {
                const QRgb rgb = image.pixel (x, y);
                fprintf (stderr, " %08X", rgb);
            }
            fprintf (stderr, "\n");
        }
    }
}
#endif


    QPixmap newPixmap = kpPixmapFX::convertToPixmapAsLosslessAsPossible (image,
        kpPixmapFX::WarnAboutLossInfo (
             ki18n ("The image \"%1\""
                   " may have more colors than the current screen mode."
                   " In order to display it, some colors may be changed."
                   " Try increasing your screen depth to at least %2bpp."

                   "\nIt also"

                   " contains translucency which is not fully"
                   " supported. The translucency data will be"
                   " approximated with a 1-bit transparency mask.")
                   .subs (prettyFilenameForURL (url)),
             ki18n ("The image \"%1\""
                   " may have more colors than the current screen mode."
                   " In order to display it, some colors may be changed."
                   " Try increasing your screen depth to at least %2bpp.")
                   .subs (prettyFilenameForURL (url)),
             i18n ("The image \"%1\""
                   " contains translucency which is not fully"
                   " supported. The translucency data will be"
                   " approximated with a 1-bit transparency mask.",
                   prettyFilenameForURL (url)),
            "docOpen",
            parent));


#if DEBUG_KP_DOCUMENT && 1
{
    const QImage image2 = kpPixmapFX::convertToImage (newPixmap);
    kDebug () << "(Converted to pixmap) Image dump:" << endl;

    bool differsFromOrgImage = false;
    unsigned long hash = 0;
    int numDiff = 0;
    for (int y = 0; y < image2.height (); y++)
    {
        for (int x = 0; x < image2.width (); x++)
        {
            const QRgb rgb = image2.pixel (x, y);
            hash += ((x % 2) + 1) * rgb;
            if (rgb != image.pixel (x, y))
            {
                differsFromOrgImage = true;
                numDiff++;
            }
            if (image2.width () <= 16 && image2.height () <= 16)
                fprintf (stderr, " %08X", rgb);
        }
        if (image2.width () <= 16 && image2.height () <= 16)
            fprintf (stderr, "\n");
    }

    kDebug () << "\tdiffersFromOrgImage="
               << differsFromOrgImage
               << " numDiff="
               << numDiff
               << " hash=" << hash << endl;
}
#endif

    if (newPixmap.isNull ())
    {
        KMessageBox::sorry (parent,
                            i18n ("Could not open \"%1\" - out of graphics memory.",
                                  kpDocument::prettyFilenameForURL (url)));
        KIO::NetAccess::removeTempFile (tempFile);
        return QPixmap ();
    }

#if DEBUG_KP_DOCUMENT
    kDebug () << "\tpixmap: depth=" << newPixmap.depth ()
                << " hasAlphaChannelOrMask=" << newPixmap.hasAlpha ()
                << " hasAlphaChannel=" << newPixmap.hasAlphaChannel ()
                << endl;
#endif


    KIO::NetAccess::removeTempFile (tempFile);
    return newPixmap;
}

void kpDocument::openNew (const KUrl &url)
{
#if DEBUG_KP_DOCUMENT
    kDebug () << "kpDocument::openNew (" << url << ")" << endl;
#endif

    m_pixmap->fill (Qt::white);

    setURL (url, false/*not from url*/);
    *m_saveOptions = kpDocumentSaveOptions ();
    *m_metaInfo = kpDocumentMetaInfo ();
    m_modified = false;

    emit documentOpened ();
}

bool kpDocument::open (const KUrl &url, bool newDocSameNameIfNotExist)
{
#if DEBUG_KP_DOCUMENT
    kDebug () << "kpDocument::open (" << url << ")" << endl;
#endif

    kpDocumentSaveOptions newSaveOptions;
    kpDocumentMetaInfo newMetaInfo;
    QPixmap newPixmap = kpDocument::getPixmapFromFile (url,
        newDocSameNameIfNotExist/*suppress "doesn't exist" dialog*/,
        m_mainWindow,
        &newSaveOptions,
        &newMetaInfo);

    if (!newPixmap.isNull ())
    {
        delete m_pixmap;
        m_pixmap = new QPixmap (newPixmap);

        setURL (url, true/*is from url*/);
        *m_saveOptions = newSaveOptions;
        *m_metaInfo = newMetaInfo;
        m_modified = false;

        emit documentOpened ();
        return true;
    }

    if (newDocSameNameIfNotExist)
    {
        if (!url.isEmpty () &&
            // not just a permission error?
            !KIO::NetAccess::exists (url, true/*open*/, m_mainWindow))
        {
            openNew (url);
        }
        else
        {
            openNew (KUrl ());
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool kpDocument::save (bool overwritePrompt, bool lossyPrompt)
{
#if DEBUG_KP_DOCUMENT
    kDebug () << "kpDocument::save("
               << "overwritePrompt=" << overwritePrompt
               << ",lossyPrompt=" << lossyPrompt
               << ") url=" << m_url
               << " savedAtLeastOnceBefore=" << savedAtLeastOnceBefore ()
               << endl;
#endif

    // TODO: check feels weak
    if (m_url.isEmpty () || m_saveOptions->mimeType ().isEmpty ())
    {
        KMessageBox::detailedError (m_mainWindow,
            i18n ("Could not save image - insufficient information."),
            i18n ("URL: %1\n"
                  "Mimetype: %2",
                  prettyUrl (),
                  m_saveOptions->mimeType ().isEmpty () ?
                          i18n ("<empty>") :
                          m_saveOptions->mimeType ()),
            i18n ("Internal Error"));
        return false;
    }

    return saveAs (m_url, *m_saveOptions,
                   overwritePrompt,
                   lossyPrompt);
}


// public static
bool kpDocument::lossyPromptContinue (const QPixmap &pixmap,
                                      const kpDocumentSaveOptions &saveOptions,
                                      QWidget *parent)
{
#if DEBUG_KP_DOCUMENT
    kDebug () << "kpDocument::lossyPromptContinue()" << endl;
#endif

#define QUIT_IF_CANCEL(messageBoxCommand)            \
{                                                    \
    if (messageBoxCommand != KMessageBox::Continue)  \
    {                                                \
        return false;                                \
    }                                                \
}

    const int lossyType = saveOptions.isLossyForSaving (pixmap);
    if (lossyType & (kpDocumentSaveOptions::MimeTypeMaximumColorDepthLow |
                     kpDocumentSaveOptions::Quality))
    {
        QUIT_IF_CANCEL (
            KMessageBox::warningContinueCancel (parent,
                i18n ("<qt><p>The <b>%1</b> format may not be able"
                      " to preserve all of the image's color information.</p>"

                      "<p>Are you sure you want to save in this format?</p></qt>",
                      KMimeType::mimeType (saveOptions.mimeType ())->comment ()),
                // TODO: caption misleading for lossless formats that have
                //       low maximum colour depth
                i18n ("Lossy File Format"),
                KStdGuiItem::save (),
                QLatin1String ("SaveInLossyMimeTypeDontAskAgain")));
    }
    else if (lossyType & kpDocumentSaveOptions::ColorDepthLow)
    {
        QUIT_IF_CANCEL (
            KMessageBox::warningContinueCancel (parent,
                i18n ("<qt><p>Saving the image at the low color depth of %1-bit"
                        " may result in the loss of color information."

                        " Any transparency will also be removed.</p>"

                        "<p>Are you sure you want to save at this color depth?</p></qt>",
                      saveOptions.colorDepth ()),
                i18n ("Low Color Depth"),
                KStdGuiItem::save (),
                QLatin1String ("SaveAtLowColorDepthDontAskAgain")));
    }
#undef QUIT_IF_CANCEL

    return true;
}

// public static
bool kpDocument::savePixmapToDevice (const QPixmap &pixmap,
                                     QIODevice *device,
                                     const kpDocumentSaveOptions &saveOptions,
                                     const kpDocumentMetaInfo &metaInfo,
                                     bool lossyPrompt,
                                     QWidget *parent,
                                     bool *userCancelled)
{
    if (userCancelled)
        *userCancelled = false;

    QString type = KImageIO::typeForMime (saveOptions.mimeType ()) [0];  // COMPAT: dangerous [0]
#if DEBUG_KP_DOCUMENT
    kDebug () << "\tmimeType=" << saveOptions.mimeType ()
               << " type=" << type << endl;
#endif

    if (lossyPrompt && !lossyPromptContinue (pixmap, saveOptions, parent))
    {
        if (userCancelled)
            *userCancelled = true;

    #if DEBUG_KP_DOCUMENT
        kDebug () << "\treturning false because of lossyPrompt" << endl;
    #endif
        return false;
    }


    QPixmap pixmapToSave =
        kpPixmapFX::pixmapWithDefinedTransparentPixels (pixmap,
            Qt::white);  // CONFIG
    QImage imageToSave = kpPixmapFX::convertToImage (pixmapToSave);


    // TODO: fix dup with kpDocumentSaveOptions::isLossyForSaving()
    const bool useSaveOptionsColorDepth =
        (saveOptions.mimeTypeHasConfigurableColorDepth () &&
         !saveOptions.colorDepthIsInvalid ());
    const bool useSaveOptionsQuality =
        (saveOptions.mimeTypeHasConfigurableQuality () &&
         !saveOptions.qualityIsInvalid ());


    //
    // Reduce colors if required
    //

    if (useSaveOptionsColorDepth &&
        imageToSave.depth () != saveOptions.colorDepth ())
    {
        // TODO: I think this erases the alpha channel!
        //       See comment in kpEffectReduceColors::applyEffect().
        imageToSave = kpEffectReduceColors::convertImageDepth (imageToSave,
                                           saveOptions.colorDepth (),
                                           saveOptions.dither ());
    }


    //
    // Write Meta Info
    //

    imageToSave.setDotsPerMeterX (metaInfo.dotsPerMeterX ());
    imageToSave.setDotsPerMeterY (metaInfo.dotsPerMeterY ());
    imageToSave.setOffset (metaInfo.offset ());

    QList <QImageTextKeyLang> keyList = metaInfo.textList ();
    for (QList <QImageTextKeyLang>::const_iterator it = keyList.begin ();
         it != keyList.end ();
         it++)
    {
        imageToSave.setText ((*it).key, (*it).lang, metaInfo.text (*it));
    }


    //
    // Save at required quality
    //

    int quality = -1;  // default

    if (useSaveOptionsQuality)
        quality = saveOptions.quality ();

    if (!imageToSave.save (device, type.toLatin1 (), quality))
    {
    #if DEBUG_KP_DOCUMENT
        kDebug () << "\tQImage::save() returned false" << endl;
    #endif
        return false;
    }


#if DEBUG_KP_DOCUMENT
    kDebug () << "\tsave OK" << endl;
#endif
    return true;
}

// public static
bool kpDocument::savePixmapToFile (const QPixmap &pixmap,
                                   const KUrl &url,
                                   const kpDocumentSaveOptions &saveOptions,
                                   const kpDocumentMetaInfo &metaInfo,
                                   bool overwritePrompt,
                                   bool lossyPrompt,
                                   QWidget *parent)
{
#if DEBUG_KP_DOCUMENT
    kDebug () << "kpDocument::savePixmapToFile ("
               << url
               << ",overwritePrompt=" << overwritePrompt
               << ",lossyPrompt=" << lossyPrompt
               << ")" << endl;
    saveOptions.printDebug (QLatin1String ("\tsaveOptions"));
    metaInfo.printDebug (QLatin1String ("\tmetaInfo"));
#endif

    if (overwritePrompt && KIO::NetAccess::exists (url, false/*write*/, parent))
    {
        int result = KMessageBox::warningContinueCancel (parent,
            i18n ("A document called \"%1\" already exists.\n"
                  "Do you want to overwrite it?",
                  prettyFilenameForURL (url)),
            QString::null,
            KGuiItem(i18n ("Overwrite")));

        if (result != KMessageBox::Continue)
        {
        #if DEBUG_KP_DOCUMENT
            kDebug () << "\tuser doesn't want to overwrite" << endl;
        #endif

            return false;
        }
    }


    if (lossyPrompt && !lossyPromptContinue (pixmap, saveOptions, parent))
    {
    #if DEBUG_KP_DOCUMENT
        kDebug () << "\treturning false because of lossyPrompt" << endl;
    #endif
        return false;
    }

    KTemporaryFile tempFile;
    QString filename;

    if (!url.isLocalFile ())
    {
        if (!tempFile.open())
        {
        #if DEBUG_KP_DOCUMENT
            kDebug () << "\treturning false because tempFile empty" << endl;
        #endif
            KMessageBox::error (parent,
                                i18n ("Could not save image - unable to create temporary file."));
            return false;
        }else{
            filename = tempFile.fileName ();
        }
    }
    else
        filename = url.path ();


    QFile file (filename);
    bool fileOpenOK = false;
    if (!(fileOpenOK = file.open (QIODevice::WriteOnly)) ||
        !savePixmapToDevice (pixmap, &file,
                             saveOptions, metaInfo,
                             false/*no lossy prompt*/,
                             parent) ||
        (file.close (),
         file.error () != QFile::NoError))
    {
    #if DEBUG_KP_DOCUMENT
        if (!fileOpenOK)
        {
            kDebug () << "\treturning false because fileOpenOK=false"
                       << " errorString=" << file.errorString () << endl;
        }
        else
        {
            kDebug () << "\treturning false because could not save pixmap to device"
                       << endl;
        }
    #endif

        // TODO: use file.errorString()
        KMessageBox::error (parent,
                            i18n ("Could not save as \"%1\".",
                                  kpDocument::prettyFilenameForURL (url)));

        return false;
    }


    if (!url.isLocalFile ())
    {
        if (!KIO::NetAccess::upload (filename, url, parent))
        {
        #if DEBUG_KP_DOCUMENT
            kDebug () << "\treturning false because could not upload" << endl;
        #endif
            KMessageBox::error (parent,
                                i18n ("Could not save image - failed to upload."));
            return false;
        }
    }


    return true;
}

bool kpDocument::saveAs (const KUrl &url,
                         const kpDocumentSaveOptions &saveOptions,
                         bool overwritePrompt,
                         bool lossyPrompt)
{
#if DEBUG_KP_DOCUMENT
    kDebug () << "kpDocument::saveAs (" << url << ","
               << saveOptions.mimeType () << ")" << endl;
#endif

    if (kpDocument::savePixmapToFile (pixmapWithSelection (),
                                      url,
                                      saveOptions, *metaInfo (),
                                      overwritePrompt,
                                      lossyPrompt,
                                      m_mainWindow))
    {
        setURL (url, true/*is from url*/);
        *m_saveOptions = saveOptions;
        m_modified = false;

        m_savedAtLeastOnceBefore = true;

        emit documentSaved ();
        return true;
    }
    else
    {
        return false;
    }
}

// public
bool kpDocument::savedAtLeastOnceBefore () const
{
    return m_savedAtLeastOnceBefore;
}

// public
KUrl kpDocument::url () const
{
    return m_url;
}

// public
void kpDocument::setURL (const KUrl &url, bool isFromURL)
{
    m_url = url;
    m_isFromURL = isFromURL;
}

// public
bool kpDocument::isFromURL (bool checkURLStillExists) const
{
    if (!m_isFromURL)
        return false;

    if (!checkURLStillExists)
        return true;

    return (!m_url.isEmpty () &&
            KIO::NetAccess::exists (m_url, true/*open*/, m_mainWindow));
}


// static
QString kpDocument::prettyURLForURL (const KUrl &url)
{
    if (url.isEmpty ())
        return i18n ("Untitled");
    else
        return url.pathOrUrl ();
}

QString kpDocument::prettyUrl () const
{
    return prettyURLForURL (m_url);
}


// static
QString kpDocument::prettyFilenameForURL (const KUrl &url)
{
    if (url.isEmpty ())
        return i18n ("Untitled");
    else if (url.fileName ().isEmpty ())
        return prettyURLForURL (url);  // better than the name ""
    else
        return url.fileName ();
}

QString kpDocument::prettyFilename () const
{
    return prettyFilenameForURL (m_url);
}


// public
const kpDocumentSaveOptions *kpDocument::saveOptions () const
{
    return m_saveOptions;
}

// public
void kpDocument::setSaveOptions (const kpDocumentSaveOptions &saveOptions)
{
    *m_saveOptions = saveOptions;
}


// public
const kpDocumentMetaInfo *kpDocument::metaInfo () const
{
    return m_metaInfo;
}

// public
void kpDocument::setMetaInfo (const kpDocumentMetaInfo &metaInfo)
{
    *m_metaInfo = metaInfo;
}


/*
 * Properties
 */

void kpDocument::setModified (bool yes)
{
    if (yes == m_modified)
        return;

    m_modified = yes;

    if (yes)
        emit documentModified ();
}

bool kpDocument::isModified () const
{
    return m_modified;
}

bool kpDocument::isEmpty () const
{
    return url ().isEmpty () && !isModified ();
}


int kpDocument::constructorWidth () const
{
    return m_constructorWidth;
}

int kpDocument::width (bool ofSelection) const
{
    if (ofSelection && m_selection)
        return m_selection->width ();
    else
        return m_pixmap->width ();
}

int kpDocument::oldWidth () const
{
    return m_oldWidth;
}

void kpDocument::setWidth (int w, const kpColor &backgroundColor)
{
    resize (w, height (), backgroundColor);
}


int kpDocument::constructorHeight () const
{
    return m_constructorHeight;
}

int kpDocument::height (bool ofSelection) const
{
    if (ofSelection && m_selection)
        return m_selection->height ();
    else
        return m_pixmap->height ();
}

int kpDocument::oldHeight () const
{
    return m_oldHeight;
}

void kpDocument::setHeight (int h, const kpColor &backgroundColor)
{
    resize (width (), h, backgroundColor);
}

QRect kpDocument::rect (bool ofSelection) const
{
    if (ofSelection && m_selection)
        return m_selection->boundingRect ();
    else
        return m_pixmap->rect ();
}


/*
 * Pixmap access
 */

// public
QPixmap kpDocument::getPixmapAt (const QRect &rect) const
{
    return kpPixmapFX::getPixmapAt (*m_pixmap, rect);
}

// public
void kpDocument::setPixmapAt (const QPixmap &pixmap, const QPoint &at)
{
#if DEBUG_KP_DOCUMENT && 0
    kDebug () << "kpDocument::setPixmapAt (pixmap (w="
               << pixmap.width ()
               << ",h=" << pixmap.height ()
               << "), x=" << at.x ()
               << ",y=" << at.y ()
               << endl;
#endif

    kpPixmapFX::setPixmapAt (m_pixmap, at, pixmap);
    slotContentsChanged (QRect (at.x (), at.y (), pixmap.width (), pixmap.height ()));
}

// public
void kpDocument::paintPixmapAt (const QPixmap &pixmap, const QPoint &at)
{
    kpPixmapFX::paintPixmapAt (m_pixmap, at, pixmap);
    slotContentsChanged (QRect (at.x (), at.y (), pixmap.width (), pixmap.height ()));
}


// public
QPixmap *kpDocument::pixmap (bool ofSelection) const
{
    if (ofSelection)
    {
        if (m_selection && m_selection->pixmap ())
            return m_selection->pixmap ();
        else
            return 0;
    }
    else
        return m_pixmap;
}

// public
void kpDocument::setPixmap (const QPixmap &pixmap)
{
    m_oldWidth = width (), m_oldHeight = height ();

    *m_pixmap = pixmap;

    if (m_oldWidth == width () && m_oldHeight == height ())
        slotContentsChanged (pixmap.rect ());
    else
        slotSizeChanged (width (), height ());
}

// public
void kpDocument::setPixmap (bool ofSelection, const QPixmap &pixmap)
{
    if (ofSelection)
    {
        // Have to have a selection in order to set its pixmap.
        Q_ASSERT (m_selection);
        
        m_selection->setPixmap (pixmap);
    }
    else
        setPixmap (pixmap);
}


// public
kpSelection *kpDocument::selection () const
{
    return m_selection;
}

// public
void kpDocument::setSelection (const kpSelection &selection)
{
#if DEBUG_KP_DOCUMENT && 0
    kDebug () << "kpDocument::setSelection() sel boundingRect="
               << selection.boundingRect ()
               << endl;
#endif

    Q_ASSERT (m_mainWindow);

    kpViewManager *vm = m_mainWindow->viewManager ();
    Q_ASSERT (vm);

    vm->setQueueUpdates ();


    bool hadSelection = (bool) m_selection;


    const bool isTextChanged = (m_mainWindow->toolIsTextTool () !=
                                (selection.type () == kpSelection::Text));

    // (we don't change the Selection Tool if the new selection's
    //  shape is different to the tool's because all the Selection
    //  Tools act the same, except for what would be really irritating
    //  if it kept changing whenever you paste an image - drawing the
    //  selection region)
    if (!m_mainWindow->toolIsASelectionTool () || isTextChanged)
    {
        // Switch to the appropriately shaped selection tool
        // _before_ we change the selection
        // (all selection tool's ::end() functions nuke the current selection)
        switch (selection.type ())
        {
        case kpSelection::Rectangle:
            m_mainWindow->slotToolRectSelection ();
            break;
        case kpSelection::Ellipse:
            m_mainWindow->slotToolEllipticalSelection ();
            break;
        case kpSelection::Points:
            m_mainWindow->slotToolFreeFormSelection ();
            break;
        case kpSelection::Text:
            m_mainWindow->slotToolText ();
            break;
        default:
            break;
        }
    }


    if (m_selection)
    {
        if (m_selection->pixmap ())
            slotContentsChanged (m_selection->boundingRect ());
        else
            vm->updateViews (m_selection->boundingRect ());
        delete m_selection;
    }

    m_selection = new kpSelection (selection);

    // TODO: this coupling to m_mainWindow is bad, careless and lazy
    if (!m_selection->isText ())
    {
        if (m_selection->transparency () != m_mainWindow->selectionTransparency ())
        {
            kDebug () << "kpDocument::setSelection() sel's transparency differs "
                          "from mainWindow's transparency - setting mainWindow's transparency "
                          "to sel"
                       << endl;
            kDebug () << "\tisOpaque: sel=" << m_selection->transparency ().isOpaque ()
                       << " mainWindow=" << m_mainWindow->selectionTransparency ().isOpaque ()
                       << endl;
            m_mainWindow->setSelectionTransparency (m_selection->transparency ());
        }
    }
    else
    {
        if (m_selection->textStyle () != m_mainWindow->textStyle ())
        {
            kDebug () << "kpDocument::setSelection() sel's textStyle differs "
                          "from mainWindow's textStyle - setting mainWindow's textStyle "
                          "to sel"
                       << endl;
            m_mainWindow->setTextStyle (m_selection->textStyle ());
        }
    }

#if DEBUG_KP_DOCUMENT && 0
    kDebug () << "\tcheck sel " << (int *) m_selection
               << " boundingRect=" << m_selection->boundingRect ()
               << endl;
#endif
    if (m_selection->pixmap ())
        slotContentsChanged (m_selection->boundingRect ());
    else
        vm->updateViews (m_selection->boundingRect ());

    connect (m_selection, SIGNAL (changed (const QRect &)),
             this, SLOT (slotContentsChanged (const QRect &)));


    if (!hadSelection)
        emit selectionEnabled (true);

    if (isTextChanged)
        emit selectionIsTextChanged (selection.type () == kpSelection::Text);

    if (vm)
        vm->restoreQueueUpdates ();
}

// public
QPixmap kpDocument::getSelectedPixmap (const QBitmap &maskBitmap_) const
{
    kpSelection *sel = selection ();
    Q_ASSERT (sel);

    // easy if we already have it :)
    if (sel->pixmap ())
        return *sel->pixmap ();


    const QRect boundingRect = sel->boundingRect ();
    Q_ASSERT (boundingRect.isValid ());


    QBitmap maskBitmap = maskBitmap_;
    if (maskBitmap.isNull () &&
        !sel->isRectangular ())
    {
        maskBitmap = sel->maskForOwnType ();
        Q_ASSERT (!maskBitmap.isNull ());
    }


    QPixmap selPixmap = getPixmapAt (boundingRect);

    if (!maskBitmap.isNull ())
    {
        if (selPixmap.mask ().isNull ())
            selPixmap.setMask (kpPixmapFX::getNonNullMask (selPixmap));

        // Dest Src = Result
        // -----------------
        //  0    0      0
        //  0    1      0
        //  1    0      0
        //  1    1      1
        //
        // Dest = selMaskBitmap (rectangular pixmap cutout -
        //        may have transparent pixels so can't blindly opaque
        //        bits according to selection shape - must use AND)
        // Src = maskBitmap (selection shape)

        QPixmap maskPixmap (maskBitmap.width (), maskBitmap.height ());
        maskPixmap.fill (Qt::yellow/*arbitrary since source pixels ignored*/);
        maskPixmap.setMask (maskBitmap);  // Mask is not ignored though.

//selPixmap.save ("a.png", "PNG");
        QPainter painter (&selPixmap);
        painter.setCompositionMode (QPainter::CompositionMode_DestinationIn);
        painter.drawPixmap (QPoint (0, 0), maskPixmap);
        painter.end ();
//selPixmap.save ("b.png", "PNG");
    }

    return selPixmap;
}

// public
bool kpDocument::selectionPullFromDocument (const kpColor &backgroundColor)
{
    kpViewManager *vm = m_mainWindow ? m_mainWindow->viewManager () : 0;

    kpSelection *sel = selection ();
    Q_ASSERT (sel);

    // Should not already have a pixmap or we would not be pulling.
    Q_ASSERT (!sel->pixmap ());

    const QRect boundingRect = sel->boundingRect ();
    Q_ASSERT (boundingRect.isValid ());


    //
    // Figure out mask for non-rectangular selections
    //

    QBitmap maskBitmap = sel->maskForOwnType (true/*return null bitmap for rectangular*/);


    //
    // Get selection pixmap from document
    //

    QPixmap selPixmap = getSelectedPixmap (maskBitmap);

    if (vm)
        vm->setQueueUpdates ();

    sel->setPixmap (selPixmap);


    //
    // Fill opaque bits of the hole in the document
    //

    // TODO: this assumes backgroundColor == sel->transparency ().transparentColor()
    const QPixmap selTransparentPixmap = sel->transparentPixmap ();

    if (backgroundColor.isOpaque ())
    {
        QPixmap erasePixmap (boundingRect.width (), boundingRect.height ());
        erasePixmap.fill (backgroundColor.toQColor ());

        if (!selTransparentPixmap.mask ().isNull ())
            erasePixmap.setMask (selTransparentPixmap.mask ());

        paintPixmapAt (erasePixmap, boundingRect.topLeft ());
    }
    else
    {
        kpPixmapFX::paintMaskTransparentWithBrush (m_pixmap,
                                                   boundingRect.topLeft (),
                                                   kpPixmapFX::getNonNullMask (selTransparentPixmap));
        slotContentsChanged (boundingRect);
    }

    if (vm)
        vm->restoreQueueUpdates ();

    return true;
}

// public
bool kpDocument::selectionDelete ()
{
    kpSelection *sel = selection ();

    if (!sel)
        return false;

    const QRect boundingRect = sel->boundingRect ();
    if (!boundingRect.isValid ())
        return false;

    bool selectionHadPixmap = m_selection ? (bool) m_selection->pixmap () : false;

    delete m_selection;
    m_selection = 0;


    // HACK to prevent document from being modified when
    //      user cancels dragging out a new selection
    if (selectionHadPixmap)
        slotContentsChanged (boundingRect);
    else
        emit contentsChanged (boundingRect);

    emit selectionEnabled (false);


    return true;
}

// public
bool kpDocument::selectionCopyOntoDocument (bool useTransparentPixmap)
{
    kpSelection *sel = selection ();

    // must have a pixmap already
    if (!sel)
        return false;

    // hasn't actually been lifted yet
    if (!sel->pixmap ())
        return true;

    const QRect boundingRect = sel->boundingRect ();
    if (!boundingRect.isValid ())
        return false;

    if (!sel->isText ())
    {
        paintPixmapAt (useTransparentPixmap ? sel->transparentPixmap () : sel->opaquePixmap (),
                       boundingRect.topLeft ());
    }
    else
    {
        // (for antialiasing with background)
        sel->paint (m_pixmap, rect ());
    }

    return true;
}

// public
bool kpDocument::selectionPushOntoDocument (bool useTransparentPixmap)
{
    return (selectionCopyOntoDocument (useTransparentPixmap) && selectionDelete ());
}

// public
QPixmap kpDocument::pixmapWithSelection () const
{
#if DEBUG_KP_DOCUMENT && 1
    kDebug () << "kpDocument::pixmapWithSelection()" << endl;
#endif

    // Have floating selection?
    if (m_selection && m_selection->pixmap ())
    {
    #if DEBUG_KP_DOCUMENT && 1
        kDebug () << "\tselection @ " << m_selection->boundingRect () << endl;
    #endif
        QPixmap output = *m_pixmap;

        kpPixmapFX::paintPixmapAt (&output,
                                   m_selection->topLeft (),
                                   m_selection->transparentPixmap ());
        return output;
    }
    else
    {
    #if DEBUG_KP_DOCUMENT && 1
        kDebug () << "\tno selection" << endl;
    #endif
        return *m_pixmap;
    }
}


/*
 * Transformations
 */

void kpDocument::fill (const kpColor &color)
{
#if DEBUG_KP_DOCUMENT
    kDebug () << "kpDocument::fill ()" << endl;
#endif

    kpPixmapFX::fill (m_pixmap, color);
    slotContentsChanged (m_pixmap->rect ());
}

void kpDocument::resize (int w, int h, const kpColor &backgroundColor)
{
#if DEBUG_KP_DOCUMENT
    kDebug () << "kpDocument::resize (" << w << "," << h << ")" << endl;
#endif

    m_oldWidth = width (), m_oldHeight = height ();

#if DEBUG_KP_DOCUMENT && 1
    kDebug () << "\toldWidth=" << m_oldWidth
               << " oldHeight=" << m_oldHeight
               << endl;
#endif

    if (w == m_oldWidth && h == m_oldHeight)
        return;

    kpPixmapFX::resize (m_pixmap, w, h, backgroundColor);

    slotSizeChanged (width (), height ());
}


/*
 * Slots
 */

void kpDocument::slotContentsChanged (const QRect &rect)
{
    setModified ();
    emit contentsChanged (rect);
}

void kpDocument::slotSizeChanged (int newWidth, int newHeight)
{
    setModified ();
    emit sizeChanged (newWidth, newHeight);
    emit sizeChanged (QSize (newWidth, newHeight));
}

void kpDocument::slotSizeChanged (const QSize &newSize)
{
    slotSizeChanged (newSize.width (), newSize.height ());
}

#include <kpdocument.moc>

