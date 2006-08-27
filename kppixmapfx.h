
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


#ifndef KP_PIXMAP_FX_H
#define KP_PIXMAP_FX_H


#include <qstring.h>

#include <klocalizedstring.h>

#include <kpcolor.h>


class QBitmap;
class QColor;
class QImage;
class QMatrix;
class QPainter;
class QPen;
class QPixmap;
class QPoint;
class QPolygon;
class QRect;
class QString;
class QWidget;


class kpSelection;


//
// QPixmap (view) Manipulation.
//
// Anything that is supposed to be manipulating the document contents
// (i.e. kpImage), should be moved to kpPainter.
//
// kpPainter uses us for its Qt backend but we don't use kpPainter.
// TODO: We should not use kpColor nor QPixmap either for the same reason.
//
// WARNING: The given QPixmap's can have masks but not alpha channels.
//
#define KP_PFX_CHECK_NO_ALPHA_CHANNEL(pixmap) Q_ASSERT (!(pixmap).hasAlphaChannel ())
class kpPixmapFX
{
public:

    //
    // Overflow Resistant Arithmetic:
    //
    // TODO: How about use "long long" or Qt's 64-bit int or one of these
    //       wrapped in a class to avoid implicit typecasting/truncation?
    //       And, these don't belong in kpPixmapFX.
    //
    // Returns INT_MAX if <lhs> or <rhs> < 0 or if would overflow.
    static int addDimensions (int lhs, int rhs);
    static int multiplyDimensions (int lhs, int rhs);


    //
    // QPixmap Statistics
    //

    // Returns the width * height.
    static int pixmapArea (const QPixmap &pixmap);
    static int pixmapArea (const QPixmap *pixmap);
    static int pixmapArea (int width, int height);

    // Returns the estimated size of <pixmap> in pixmap memory.
    static int pixmapSize (const QPixmap &pixmap);
    static int pixmapSize (const QPixmap *pixmap);
    static int pixmapSize (int width, int height, int depth);

    static int imageSize (const QImage &image);
    static int imageSize (const QImage *image);
    static int imageSize (int width, int height, int depth);

    static int selectionSize (const kpSelection &sel);
    static int selectionSize (const kpSelection *sel);

    static int stringSize (const QString &string);

    static int pointArraySize (const QPolygon &points);


    //
    // QPixmap/QImage Conversion Functions
    //

    //
    // Converts <pixmap> to a QImage and returns it.
    //
    static QImage convertToImage (const QPixmap &pixmap);

    //
    // Dialog info for warning about data loss with convertToPixmap().
    //
    struct WarnAboutLossInfo
    {
        // <moreColorsThanDisplayAndHasAlphaChannelMessage>:
        //
        //     ki18n ("The (image \"example.jpg\"|image from the clipboard)"
        //           " may have more colors than the current screen mode."
        //           " In order to display it, some colors may be changed."
        //           " Try increasing your screen depth to at least %1bpp."
        //
        //           "\nIt also"
        //
        //           " contains translucency which is not fully"
        //           " supported. The translucency data will be"
        //           " approximated with a 1-bit transparency mask.")
        //
        // <moreColorsThanDisplayMessage>:
        //     ki18n ("The (image \"example.jpg\"|image from the clipboard)"
        //           " may have more colors than the current screen mode."
        //           " In order to display it, some colors may be changed."
        //           " Try increasing your screen depth to at least %1bpp.")
        //
        // <hasAlphaChannelMessage>:
        //     i18n ("The (image \"example.jpg\"|image from the clipboard)"
        //           " contains translucency which is not fully"
        //           " supported. The translucency data will be"
        //           " approximated with a 1-bit transparency mask.")
        //
        // <dontAskAgainPrefix>:
        //
        //     Don'tAskAgain ID for dialog.
        //
        // <parent>:
        //
        //     Dialog parent
        //
        WarnAboutLossInfo (const KLocalizedString &moreColorsThanDisplayAndHasAlphaChannelMessage,
                const KLocalizedString &moreColorsThanDisplayMessage,
                const QString &hasAlphaChannelMessage,
                const QString &dontAskAgainPrefix,
                QWidget *parent)
            :
                m_moreColorsThanDisplayAndHasAlphaChannelMessage (
                    moreColorsThanDisplayAndHasAlphaChannelMessage),
                m_moreColorsThanDisplayMessage (
                    moreColorsThanDisplayMessage),
                m_hasAlphaChannelMessage (
                    hasAlphaChannelMessage),
                m_dontAskAgainPrefix (
                    dontAskAgainPrefix),
                m_parent (parent),
                m_isValid (true)
        {
        }

        WarnAboutLossInfo ()
            : m_parent (0),
              m_isValid (false)
        {
        }

        ~WarnAboutLossInfo ()
        {
        }


        bool isValid () const { return m_isValid; }


        KLocalizedString m_moreColorsThanDisplayAndHasAlphaChannelMessage;
        KLocalizedString m_moreColorsThanDisplayMessage;
        QString m_hasAlphaChannelMessage;
        QString m_dontAskAgainPrefix;
        QWidget *m_parent;
        bool m_isValid;
    };

    //
    // Converts <image> to a QPixmap of the current display's depth and
    // returns it.
    //
    // If the flag <pretty> is set, it will dither the image making the
    // returned pixmap look better but if the image has few colours
    // (less than the screen can handle), this will be at the expense of
    // exactness of conversion.
    //
    // This will automatically call ensureNoAlphaChannel().
    //
    // Never use a foreign QPixmap that is offered to you - always get the
    // foreign QImage and use this function to convert it to a sane QPixmap.
    //
    // <wali>, if specified, describes parameters for the dialog that comes
    // up warning the user of data loss if the <image> contains translucency
    // and/or more colors than the current display.
    //
    static QPixmap convertToPixmap (const QImage &image, bool pretty = false,
                                    const WarnAboutLossInfo &wali = WarnAboutLossInfo ());

    // Same as convertToPixmap() but tries as hard as possible to make the
    // pixmap look like the original <image> - when in doubt, reads the
    // config to see whether or not to dither (default: on).
    //
    // If you know for sure that <image> can be displayed losslessly on
    // the screen, you should call convertToPixmap() with <pretty> = false
    // instead.  If you know for sure that <image> cannot be displayed
    // losslessly, then call convertToPixmap() with <pretty> = true.
    //
    static QPixmap convertToPixmapAsLosslessAsPossible (const QImage &image,
        const WarnAboutLossInfo &wali = WarnAboutLossInfo ());


    // Sets the RGB values of the pixels where <pixmap> is transparent to
    // <transparentColor>.  This has visually no effect on the <pixmap>
    // unless the mask is lost.
    static QPixmap pixmapWithDefinedTransparentPixels (const QPixmap &pixmap,
        const QColor &transparentColor);


    //
    // Get/Set Parts of Pixmap
    //


    //
    // Returns the pixel and mask data found at the <rect> in <pm>.
    //
    static QPixmap getPixmapAt (const QPixmap &pm, const QRect &rect);

    //
    // Sets the pixel and mask data at <destRect> in <*destPixmapPtr>
    // to <srcPixmap>.  Neither <destRect>'s width nor height are allowed
    // to be bigger than <srcPixmap>'s (you can't copy more than you have).
    // On the other hand, you can copy less than the size of <srcPixmap>
    // - no scaling is done.
    //
    static void setPixmapAt (QPixmap *destPixmapPtr, const QRect &destRect,
                             const QPixmap &srcPixmap);

    //
    // Sets the pixel and mask data at the rectangle in <*destPixmapPtr>,
    // with the top-left <destAt> and dimensions <srcPixmap.rect()>,
    // to <srcPixmap>.
    //
    static void setPixmapAt (QPixmap *destPixmapPtr, const QPoint &destAt,
                             const QPixmap &srcPixmap);
    static void setPixmapAt (QPixmap *destPixmapPtr, int destX, int destY,
                             const QPixmap &srcPixmap);

    //
    // Draws <srcPixmap> on top of <*destPixmapPtr> at <destAt>.
    // The mask of <*destPixmapPtr> is adjusted so that all opaque
    // pixels in <srcPixmap> will be opaque in <*destPixmapPtr>.
    //
    static void paintPixmapAt (QPixmap *destPixmapPtr, const QPoint &destAt,
                               const QPixmap &srcPixmap);
    static void paintPixmapAt (QPixmap *destPixmapPtr, int destX, int destY,
                               const QPixmap &srcPixmap);

    //
    // Returns the colour of the pixel at <at> in <pm>.
    // If the pixel is transparent, a value is returned such that
    // kpTool::isColorTransparent(<return_value>) will return true.
    //
    static kpColor getColorAtPixel (const QPixmap &pm, const QPoint &at);
    static kpColor getColorAtPixel (const QPixmap &pm, int x, int y);

    //
    // Returns the color of the pixel at <at> in <img>.
    // If the pixel is transparent, a value is returned such that
    // kpTool::isColorTransparent(<return_value>) will return true.
    //
    static kpColor getColorAtPixel (const QImage &img, const QPoint &at);
    static kpColor getColorAtPixel (const QImage &img, int x, int y);


    //
    // Mask Operations
    //


    //
    // Removes <*destPixmapPtr>'s Alpha Channel and attempts to convert it
    // to a mask.  KolourPaint - and QPixmap to a great extent - does not
    // support Alpha Channels - only masks.  Call this whenever you get
    // a pixmap from a foreign source; else all KolourPaint code will
    // exhibit "undefined behaviour".
    //
    static void ensureNoAlphaChannel (QPixmap *destPixmapPtr);

    //
    // Returns <pm>'s mask or a fully opaque mask (with <pm>'s dimensions)
    // if <pm> does not have a mask.
    //
    static QBitmap getNonNullMask (const QPixmap &pm);

    //
    // Ensures that <*destPixmapPtr> is transparent at <rect>.
    //
    static void ensureTransparentAt (QPixmap *destPixmapPtr, const QRect &destRect);

    //
    // Sets the mask of <*destPixmapPtr> at the rectangle, with the
    // top-left <destAt> and dimensions <srcMaskBitmap.rect()>,
    // to transparent where <brushBitmap> is opaque.
    //
    // <brushPixmap> must be a QPixmap of depth 1 (or a QBitmap).
    //
    // TODO: we must drop this since 1. can't be supported in Qt4 2. we later want to support alpha
    static void paintMaskTransparentWithBrush (QPixmap *destPixmapPtr, const QPoint &destAt,
                                               const QPixmap &brushBitmap) KDE_DEPRECATED;
    static void paintMaskTransparentWithBrush (QPixmap *destPixmapPtr, int destX, int destY,
                                               const QPixmap &brushBitmap) KDE_DEPRECATED;

    //
    // Ensures that <*destPixmapPtr> is opaque at <rect>.
    //
    static void ensureOpaqueAt (QPixmap *destPixmapPtr, const QRect &destRect);


    //
    // Effects
    //


    //
    // Converts the image to grayscale.
    //
    static void convertToGrayscale (QPixmap *destPixmapPtr);
    static QPixmap convertToGrayscale (const QPixmap &pm);
    static void convertToGrayscale (QImage *destImagePtr);
    static QImage convertToGrayscale (const QImage &img);

    //
    // Fills an image in the given color.
    //
    static void fill (QPixmap *destPixmapPtr, const kpColor &color);
    static QPixmap fill (const QPixmap &pm, const kpColor &color);

    //
    // Resizes an image to the given width and height,
    // filling any new areas with <backgroundColor>.
    //
    static void resize (QPixmap *destPixmapPtr, int w, int h,
                        const kpColor &backgroundColor);
    static QPixmap resize (const QPixmap &pm, int w, int h,
                           const kpColor &backgroundColor);

    //
    // Scales an image to the given width and height.
    // If <pretty> is true, a smooth scale will be used.
    //
    static void scale (QPixmap *destPixmapPtr, int w, int h, bool pretty = false);
    static QPixmap scale (const QPixmap &pm, int w, int h, bool pretty = false);


    // The minimum difference between 2 angles (in degrees) such that they are
    // considered different.  This gives you at least enough precision to
    // rotate an image whose width <= 10000 such that its height increases
    // by just 1 (and similarly with height <= 10000 and width).
    //
    // Currently used for skew & rotate operations.
    static double AngleInDegreesEpsilon;


    //
    // Skews an image.
    //
    // <hangle>             horizontal angle clockwise (-90 < x < 90)
    // <vangle>             vertical angle clockwise (-90 < x < 90)
    // <backgroundColor>    color to fill new areas with
    // <targetWidth>        if > 0, the desired width of the resultant pixmap
    // <targetHeight>       if > 0, the desired height of the resultant pixmap
    //
    // Using <targetWidth> & <targetHeight> to generate preview pixmaps is
    // significantly more efficient than skewing and then scaling yourself.
    //
    static QMatrix skewMatrix (int width, int height, double hangle, double vangle);
    static QMatrix skewMatrix (const QPixmap &pixmap, double hangle, double vangle);

    static void skew (QPixmap *destPixmapPtr, double hangle, double vangle,
                      const kpColor &backgroundColor,
                      int targetWidth = -1, int targetHeight = -1);
    static QPixmap skew (const QPixmap &pm, double hangle, double vangle,
                         const kpColor &backgroundColor,
                         int targetWidth = -1, int targetHeight = -1);

    //
    // Rotates an image.
    //
    // <angle>              clockwise angle to rotate by
    // <backgroundColor>    color to fill new areas with
    // <targetWidth>        if > 0, the desired width of the resultant pixmap
    // <targetHeight>       if > 0, the desired height of the resultant pixmap
    //
    // Using <targetWidth> & <targetHeight> to generate preview pixmaps is
    // significantly more efficient than rotating and then scaling yourself.
    //
    static QMatrix rotateMatrix (int width, int height, double angle);
    static QMatrix rotateMatrix (const QPixmap &pixmap, double angle);

    static bool isLosslessRotation (double angle);

    static void rotate (QPixmap *destPixmapPtr, double angle,
                        const kpColor &backgroundColor,
                        int targetWidth = -1, int targetHeight = -1);
    static QPixmap rotate (const QPixmap &pm, double angle,
                           const kpColor &backgroundColor,
                           int targetWidth = -1, int targetHeight = -1);


    //
    // Flips an image in the given directions.
    //
    static QMatrix flipMatrix (int width, int height, bool horz, bool vert);
    static QMatrix flipMatrix (const QPixmap &pixmap, bool horz, bool vert);

    // TODO: this kind of overloading is error prone
    //       e.g. QPixmap pixmap;
    //            kpPixmapFX::flip (pixmap, false, true);
    //       looks like it will flip vertically but does absolutely nothing!
    //       (should be &pixmap)
    static void flip (QPixmap *destPixmapPtr, bool horz, bool vert);
    static QPixmap flip (const QPixmap &pm, bool horz, bool vert);
    static void flip (QImage *destImagePtr, bool horz, bool vert);
    static QImage flip (const QImage &img, bool horz, bool vert);


    // Returns a pen suitable for drawing a rectangle with 90 degree
    // corners ("MiterJoin").  This is necessary since Qt4 defaults to
    // "BevelJoin".  <qtWidth> is passed straight to QPen without modification.
    static QPen QPainterDrawRectPen (const QColor &color, int qtWidth);

    // Returns a pen suitable for drawing lines / polylines / polygons /
    // curves with rounded corners.  This is necessary since Qt4 defaults
    // to square corners ("SquareCap") and "BevelJoin".
    // <qtWidth> is passed straight to QPen without modification.
    static QPen QPainterDrawLinePen (const QColor &color, int qtWidth);
    

    // Inside a <drawFunc> passed to kpPixmapFX::draw(), pass the <color>
    // you intend to draw in and <drawingOnRGBLayer> (passed to your <drawFunc>),
    // and this will spit back the appropriate QColor depending on whether you
    // are drawing on the RGB layer or a mask.
    static QColor draw_ToQColor (const kpColor &color, bool drawingOnRGBLayer);

    // Exercises the drawing pattern on QPixmap's - draws on, separately, the:
    //
    // 1. RGB layer (if there is an opaque colour involved in the drawing i.e.
    //               <anyColorOpaque>)
    // 2. Mask layer (if there is a transparency involved i.e.
    //                <anyColorTransparent> or the <image> has a mask to start
    //                with)
    //
    // Each time, it opens up a QPainter and calls <drawFunc> with:
    //
    // 1. A pointer to this QPainter
    // 2. A boolean that is true if we are currently drawing on the RGB layer
    //    and false if we are drawing on the Mask layer.  Often passed straight
    //    to draw_ToQColor() to convert from kpColor to QColor.
    // 3. A pointer to the provided <data>
    static void draw (QPixmap *image,
        void (*drawFunc) (QPainter * /*p*/,
            bool /*drawingOnRGBLayer*/,
            void * /*data*/),
        bool anyColorOpaque, bool anyColorTransparent,
        void *data);

    
    // Draws a line from (x1,y1) to (x2,y2) onto <image>, with <color>
    // and <width>.  The corners are rounded and centred at those
    // coordinates so if <width> > 1, the line is likely to extend past
    // a rectangle with those corners.
    static void drawPolyline (QPixmap *image,
        const QPolygon &points,
        const kpColor &color, int penWidth);
    static void drawLine (QPixmap *image,
        int x1, int y1, int x2, int y2,
        const kpColor &color, int penWidth);
    // <isFinal> = shape completed else drawing but haven't finalised.
    // If not <isFinal>, the edge that would form the closure, if the
    // shape were finalised now, is highlighted specially.  Unfortunately,
    // the argument is currently ignored.
    //
    // Odd-even fill.
    static void drawPolygon (QPixmap *image,
        const QPolygon &points,
        const kpColor &fcolor, int penWidth,
        const kpColor &bcolor = kpColor::invalid,
        bool isFinal = true);
    // Cubic Beizer.
    static void drawCurve (QPixmap *image,
        const QPoint &startPoint,
        const QPoint &controlPointP, const QPoint &controlPointQ,
        const QPoint &endPoint,
        const kpColor &color, int penWidth);

    static void fillRect (QPixmap *image,
        int x, int y, int width, int height,
        const kpColor &color);
                
    // Draws a rectangle / rounded rectangle / ellipse with top-left at
    // (x, y) with width <width> and height <height>.  Unlike QPainter,
    // this rectangle will really fit inside <width>x<height> and won't
    // be one pixel higher or wider etc.
    //
    // <width> and <height> must be >= 0.
    //
    // <fcolor> must not be invalid.  However, <bcolor> may be invalid
    // to signify an unfilled rectangle / rounded rectangle /ellipse.
    static void drawRect (QPixmap *image,
        int x, int y, int width, int height,
        const kpColor &fcolor, int penWidth = 1,
        const kpColor &bcolor = kpColor::invalid);
    static void drawRoundedRect (QPixmap *image,
        int x, int y, int width, int height,
        const kpColor &fcolor, int penWidth = 1,
        const kpColor &bcolor = kpColor::invalid);
    static void drawEllipse (QPixmap *image,
        int x, int y, int width, int height,
        const kpColor &fcolor, int penWidth = 1,
        const kpColor &bcolor = kpColor::invalid);
};


#endif  // KP_PIXMAP_FX_H

