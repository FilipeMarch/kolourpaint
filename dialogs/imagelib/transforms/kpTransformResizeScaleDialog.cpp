
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


#define DEBUG_KP_TOOL_RESIZE_SCALE_COMMAND 0
#define DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG 0


#include <kpTransformResizeScaleDialog.h>

#include <math.h>

#include <q3accel.h>
#include <qapplication.h>
#include <qboxlayout.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qgridlayout.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qpolygon.h>
#include <qpushbutton.h>
#include <qrect.h>
#include <qsize.h>
#include <qtoolbutton.h>
#include <qmatrix.h>

#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <khbox.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>

#include <kpAbstractSelection.h>
#include <kpBug.h>
#include <kpDefs.h>
#include <kpDocument.h>
#include <kpPixmapFX.h>
#include <kpTextSelection.h>
#include <kpTool.h>
#include <kpTransformDialogEnvironment.h>


#define SET_VALUE_WITHOUT_SIGNAL_EMISSION(knuminput_instance,value)    \
{                                                                      \
    knuminput_instance->blockSignals (true);                           \
    knuminput_instance->setValue (value);                              \
    knuminput_instance->blockSignals (false);                          \
}

#define IGNORE_KEEP_ASPECT_RATIO(cmd) \
{                                     \
    m_ignoreKeepAspectRatio++;        \
    cmd;                              \
    m_ignoreKeepAspectRatio--;        \
}


// private static
kpTransformResizeScaleCommand::Type kpTransformResizeScaleDialog::s_lastType =
    kpTransformResizeScaleCommand::Resize;

// private static
double kpTransformResizeScaleDialog::s_lastPercentWidth = 100,
       kpTransformResizeScaleDialog::s_lastPercentHeight = 100;


kpTransformResizeScaleDialog::kpTransformResizeScaleDialog (
        kpTransformDialogEnvironment *environ, QWidget *parent)
    : KDialog (parent),
      m_environ (environ),
      m_ignoreKeepAspectRatio (0)
{
    setCaption( i18n ("Resize / Scale") );
    setButtons( KDialog::Ok | KDialog::Cancel);
    // Using the percentage from last time become too confusing so disable for now
    s_lastPercentWidth = 100, s_lastPercentHeight = 100;


    QWidget *baseWidget = new QWidget (this);
    setMainWidget (baseWidget);


    createActOnBox (baseWidget);
    createOperationGroupBox (baseWidget);
    createDimensionsGroupBox (baseWidget);


    QVBoxLayout *baseLayout = new QVBoxLayout (baseWidget);
    baseLayout->setSpacing (spacingHint ());
    baseLayout->setMargin (0/*margin*/);
    baseLayout->addWidget (m_actOnBox);
    baseLayout->addWidget (m_operationGroupBox);
    baseLayout->addWidget (m_dimensionsGroupBox);


    slotActOnChanged ();

    m_operationGroupBox->setFocus ();

    //enableButtonOk (!isNoOp ());
}

kpTransformResizeScaleDialog::~kpTransformResizeScaleDialog ()
{
}


// private
kpDocument *kpTransformResizeScaleDialog::document () const
{
    return m_environ->document ();
}

// private
kpAbstractSelection *kpTransformResizeScaleDialog::selection () const
{
    Q_ASSERT (document ());
    return document ()->selection ();
}

// private
kpTextSelection *kpTransformResizeScaleDialog::textSelection () const
{
    Q_ASSERT (document ());
    return document ()->textSelection ();
}


// private
void kpTransformResizeScaleDialog::createActOnBox (QWidget *baseWidget)
{
    m_actOnBox = new KHBox (baseWidget);
    m_actOnBox->setSpacing (spacingHint () * 2);


    m_actOnLabel = new QLabel (i18n ("Ac&t on:"), m_actOnBox);
    m_actOnCombo = new KComboBox (m_actOnBox);


    m_actOnLabel->setBuddy (m_actOnCombo);

    m_actOnCombo->insertItem (Image, i18n ("Entire Image"));
    if (selection ())
    {
        QString selName = i18n ("Selection");

        if (textSelection ())
            selName = i18n ("Text Box");

        m_actOnCombo->insertItem (Selection, selName);
        m_actOnCombo->setCurrentIndex (Selection);
    }
    else
    {
        m_actOnLabel->setEnabled (false);
        m_actOnCombo->setEnabled (false);
    }


    m_actOnBox->setStretchFactor (m_actOnCombo, 1);


    connect (m_actOnCombo, SIGNAL (activated (int)),
             this, SLOT (slotActOnChanged ()));
}


static QIcon toolButtonIconSet (const QString &iconName)
{
    QIcon iconSet = KIcon (iconName);


    // No "disabled" pixmap is generated by UserIconSet() so generate it
    // ourselves:

    QPixmap disabledIcon = KIconLoader::global ()->iconEffect ()->apply (
        UserIcon (iconName),
        K3Icon::Toolbar, K3Icon::DisabledState);

    const QPixmap iconSetNormalIcon = iconSet.pixmap (QSize(22,22),
                                                      QIcon::Normal);

    // I bet past or future versions of KIconEffect::apply() resize the
    // disabled icon if we claim it's in group K3Icon::Toolbar.  So resize
    // it to match the QIcon::Normal icon, just in case.
    disabledIcon = kpPixmapFX::scale (disabledIcon,
                                      iconSetNormalIcon.width (),
                                      iconSetNormalIcon.height (),
                                      true/*smooth scale*/);


    iconSet.addPixmap (disabledIcon,
                       QIcon::Disabled);

    return iconSet;
}

static void toolButtonSetLook (QToolButton *button,
                               const QString &iconName,
                               const QString &name)
{
    button->setIcon (toolButtonIconSet (iconName));
    button->setToolButtonStyle (Qt::ToolButtonTextUnderIcon);
    button->setText (name);
    button->setShortcut ( name );
    button->setFocusPolicy (Qt::StrongFocus);
    button->setCheckable (true);
}


// private
void kpTransformResizeScaleDialog::createOperationGroupBox (QWidget *baseWidget)
{
    m_operationGroupBox = new QGroupBox (i18n ("Operation"), baseWidget);

    m_resizeButton = new QToolButton (m_operationGroupBox);
    toolButtonSetLook (m_resizeButton,
                       QLatin1String ("resize"),
                       i18n ("&Resize"));

    m_scaleButton = new QToolButton (m_operationGroupBox);
    toolButtonSetLook (m_scaleButton,
                       QLatin1String ("scale"),
                       i18n ("&Scale"));

    m_smoothScaleButton = new QToolButton (m_operationGroupBox);
    toolButtonSetLook (m_smoothScaleButton,
                       QLatin1String ("smooth_scale"),
                       i18n ("S&mooth Scale"));


    //m_resizeLabel = new QLabel (i18n ("&Resize"), m_operationGroupBox);
    //m_scaleLabel = new QLabel (i18n ("&Scale"), m_operationGroupBox);
    //m_smoothScaleLabel = new QLabel (i18n ("S&mooth scale"), m_operationGroupBox);


    //m_resizeLabel->setAlignment (m_resizeLabel->alignment () | Qt::TextShowMnemonic);
    //m_scaleLabel->setAlignment (m_scaleLabel->alignment () | Qt::TextShowMnemonic);
    //m_smoothScaleLabel->setAlignment (m_smoothScaleLabel->alignment () | Qt::TextShowMnemonic);


    QButtonGroup *resizeScaleButtonGroup = new QButtonGroup (baseWidget);
    resizeScaleButtonGroup->addButton (m_resizeButton);
    resizeScaleButtonGroup->addButton (m_scaleButton);
    resizeScaleButtonGroup->addButton (m_smoothScaleButton);


    QGridLayout *operationLayout = new QGridLayout (m_operationGroupBox );
    operationLayout->setMargin (marginHint () * 2/*don't overlap groupbox title*/);
    operationLayout->setSpacing (spacingHint ());

    operationLayout->addWidget (m_resizeButton, 0, 0, Qt::AlignCenter);
    //operationLayout->addWidget (m_resizeLabel, 1, 0, Qt::AlignCenter);

    operationLayout->addWidget (m_scaleButton, 0, 1, Qt::AlignCenter);
    //operationLayout->addWidget (m_scaleLabel, 1, 1, Qt::AlignCenter);

    operationLayout->addWidget (m_smoothScaleButton, 0, 2, Qt::AlignCenter);
    //operationLayout->addWidget (m_smoothScaleLabel, 1, 2, Qt::AlignCenter);


    // Call this _after_ we've constructed all the child widgets.
    // Of course, this will not work if any of our child widgets are clever
    // and create more widgets at runtime.
    kpBug::QWidget_SetWhatsThis (m_operationGroupBox,
        i18n ("<qt>"
              "<ul>"
                  "<li><b>Resize</b>: The size of the picture will be"
                  " increased"
                  " by creating new areas to the right and/or bottom"
                  " (filled in with the background color) or"
                  " decreased by cutting"
                  " it at the right and/or bottom.</li>"

                  "<li><b>Scale</b>: The picture will be expanded"
                  " by duplicating pixels or squashed by dropping pixels.</li>"

                  "<li><b>Smooth Scale</b>: This is the same as"
                  " <i>Scale</i> except that it blends neighboring"
                  " pixels to produce a smoother looking picture.</li>"
              "</ul>"
              "</qt>"));


    connect (m_resizeButton, SIGNAL (toggled (bool)),
             this, SLOT (slotTypeChanged ()));
    connect (m_scaleButton, SIGNAL (toggled (bool)),
             this, SLOT (slotTypeChanged ()));
    connect (m_smoothScaleButton, SIGNAL (toggled (bool)),
             this, SLOT (slotTypeChanged ()));
}

// private
void kpTransformResizeScaleDialog::createDimensionsGroupBox (QWidget *baseWidget)
{
    m_dimensionsGroupBox = new QGroupBox (i18n ("Dimensions"), baseWidget);

    QLabel *widthLabel = new QLabel (i18n ("Width:"), m_dimensionsGroupBox);
    widthLabel->setAlignment (widthLabel->alignment () | Qt::AlignHCenter);
    QLabel *heightLabel = new QLabel (i18n ("Height:"), m_dimensionsGroupBox);
    heightLabel->setAlignment (heightLabel->alignment () | Qt::AlignHCenter);

    QLabel *originalLabel = new QLabel (i18n ("Original:"), m_dimensionsGroupBox);
    m_originalWidthInput = new KIntNumInput (
        document ()->width ((bool) selection ()),
        m_dimensionsGroupBox);
    QLabel *xLabel0 = new QLabel (i18n ("x"), m_dimensionsGroupBox);
    m_originalHeightInput = new KIntNumInput (
        document ()->height ((bool) selection ()),
        m_dimensionsGroupBox);

    QLabel *newLabel = new QLabel (i18n ("&New:"), m_dimensionsGroupBox);
    m_newWidthInput = new KIntNumInput (m_dimensionsGroupBox);
    QLabel *xLabel1 = new QLabel (i18n ("x"), m_dimensionsGroupBox);
    m_newHeightInput = new KIntNumInput (m_dimensionsGroupBox);

    QLabel *percentLabel = new QLabel (i18n ("&Percent:"), m_dimensionsGroupBox);
    m_percentWidthInput = new KDoubleNumInput (0.01/*lower*/, 1000000/*upper*/,
                                               100/*value*/,
                                               m_dimensionsGroupBox,
                                               1/*step*/,
                                               2/*precision*/);
    m_percentWidthInput->setSuffix (i18n ("%"));
    QLabel *xLabel2 = new QLabel (i18n ("x"), m_dimensionsGroupBox);
    m_percentHeightInput = new KDoubleNumInput (0.01/*lower*/, 1000000/*upper*/,
                                                100/*value*/,
                                                m_dimensionsGroupBox,
                                                1/*step*/,
                                                2/*precision*/);
    m_percentHeightInput->setSuffix (i18n ("%"));

    m_keepAspectRatioCheckBox = new QCheckBox (i18n ("Keep &aspect ratio"),
                                               m_dimensionsGroupBox);


    m_originalWidthInput->setEnabled (false);
    m_originalHeightInput->setEnabled (false);
    originalLabel->setBuddy (m_originalWidthInput);
    newLabel->setBuddy (m_newWidthInput);
    m_percentWidthInput->setValue (s_lastPercentWidth);
    m_percentHeightInput->setValue (s_lastPercentHeight);
    percentLabel->setBuddy (m_percentWidthInput);


    QGridLayout *dimensionsLayout = new QGridLayout (m_dimensionsGroupBox);
    dimensionsLayout->setMargin (marginHint () * 2);
    dimensionsLayout->setSpacing (spacingHint ());
    dimensionsLayout->setColumnStretch (1/*column*/, 1);
    dimensionsLayout->setColumnStretch (3/*column*/, 1);


    dimensionsLayout->addWidget (widthLabel, 0, 1);
    dimensionsLayout->addWidget (heightLabel, 0, 3);

    dimensionsLayout->addWidget (originalLabel, 1, 0);
    dimensionsLayout->addWidget (m_originalWidthInput, 1, 1);
    dimensionsLayout->addWidget (xLabel0, 1, 2);
    dimensionsLayout->addWidget (m_originalHeightInput, 1, 3);

    dimensionsLayout->addWidget (newLabel, 2, 0);
    dimensionsLayout->addWidget (m_newWidthInput, 2, 1);
    dimensionsLayout->addWidget (xLabel1, 2, 2);
    dimensionsLayout->addWidget (m_newHeightInput, 2, 3);

    dimensionsLayout->addWidget (percentLabel, 3, 0);
    dimensionsLayout->addWidget (m_percentWidthInput, 3, 1);
    dimensionsLayout->addWidget (xLabel2, 3, 2);
    dimensionsLayout->addWidget (m_percentHeightInput, 3, 3);

    dimensionsLayout->addWidget (m_keepAspectRatioCheckBox, 4, 0, 1, 4);
    dimensionsLayout->setRowStretch (4/*row*/, 1);
    dimensionsLayout->setRowMinimumHeight (4/*row*/, dimensionsLayout->rowMinimumHeight (4) * 2);


    connect (m_newWidthInput, SIGNAL (valueChanged (int)),
             this, SLOT (slotWidthChanged (int)));
    connect (m_newHeightInput, SIGNAL (valueChanged (int)),
             this, SLOT (slotHeightChanged (int)));

    connect (m_percentWidthInput, SIGNAL (valueChanged (double)),
             this, SLOT (slotPercentWidthChanged (double)));
    connect (m_percentHeightInput, SIGNAL (valueChanged (double)),
             this, SLOT (slotPercentHeightChanged (double)));

    connect (m_keepAspectRatioCheckBox, SIGNAL (toggled (bool)),
             this, SLOT (setKeepAspectRatio (bool)));
}


// private
void kpTransformResizeScaleDialog::widthFitHeightToAspectRatio ()
{
    if (m_keepAspectRatioCheckBox->isChecked () && !m_ignoreKeepAspectRatio)
    {
        // width / height = oldWidth / oldHeight
        // height = width * oldHeight / oldWidth
        const int newHeight = qRound (double (imageWidth ()) * double (originalHeight ())
                                      / double (originalWidth ()));
        IGNORE_KEEP_ASPECT_RATIO (m_newHeightInput->setValue (newHeight));
    }
}

// private
void kpTransformResizeScaleDialog::heightFitWidthToAspectRatio ()
{
    if (m_keepAspectRatioCheckBox->isChecked () && !m_ignoreKeepAspectRatio)
    {
        // width / height = oldWidth / oldHeight
        // width = height * oldWidth / oldHeight
        const int newWidth = qRound (double (imageHeight ()) * double (originalWidth ())
                                     / double (originalHeight ()));
        IGNORE_KEEP_ASPECT_RATIO (m_newWidthInput->setValue (newWidth));
    }
}


// private
bool kpTransformResizeScaleDialog::resizeEnabled () const
{
    return (!actOnSelection () ||
            (actOnSelection () && textSelection ()));
}

// private
bool kpTransformResizeScaleDialog::scaleEnabled () const
{
    return (!(actOnSelection () && textSelection ()));
}

// private
bool kpTransformResizeScaleDialog::smoothScaleEnabled () const
{
    return scaleEnabled ();
}


// public slot
void kpTransformResizeScaleDialog::slotActOnChanged ()
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpTransformResizeScaleDialog::slotActOnChanged()";
#endif

    m_resizeButton->setEnabled (resizeEnabled ());
    //m_resizeLabel->setEnabled (resizeEnabled ());

    m_scaleButton->setEnabled (scaleEnabled ());
    //m_scaleLabel->setEnabled (scaleEnabled ());

    m_smoothScaleButton->setEnabled (smoothScaleEnabled ());
    //m_smoothScaleLabel->setEnabled (smoothScaleEnabled ());


    // TODO: somehow share logic with (resize|*scale)Enabled()
    if (actOnSelection ())
    {
        if (textSelection ())
        {
            m_resizeButton->setChecked (true);
        }
        else
        {
            if (s_lastType == kpTransformResizeScaleCommand::Scale)
                m_scaleButton->setChecked (true);
            else
                m_smoothScaleButton->setChecked (true);
        }
    }
    else
    {
        if (s_lastType == kpTransformResizeScaleCommand::Resize)
            m_resizeButton->setChecked (true);
        else if (s_lastType == kpTransformResizeScaleCommand::Scale)
            m_scaleButton->setChecked (true);
        else
            m_smoothScaleButton->setChecked (true);
    }


    m_originalWidthInput->setValue (originalWidth ());
    m_originalHeightInput->setValue (originalHeight ());


    m_newWidthInput->blockSignals (true);
    m_newHeightInput->blockSignals (true);

    m_newWidthInput->setMinimum (actOnSelection () ?
                                      selection ()->minimumWidth () :
                                      1);
    m_newHeightInput->setMinimum (actOnSelection () ?
                                       selection ()->minimumHeight () :
                                       1);

    m_newWidthInput->blockSignals (false);
    m_newHeightInput->blockSignals (false);


    IGNORE_KEEP_ASPECT_RATIO (slotPercentWidthChanged (m_percentWidthInput->value ()));
    IGNORE_KEEP_ASPECT_RATIO (slotPercentHeightChanged (m_percentHeightInput->value ()));

    setKeepAspectRatio (m_keepAspectRatioCheckBox->isChecked ());
}


// public slot
void kpTransformResizeScaleDialog::slotTypeChanged ()
{
    s_lastType = type ();
}

// public slot
void kpTransformResizeScaleDialog::slotWidthChanged (int width)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpTransformResizeScaleDialog::slotWidthChanged("
               << width << ")" << endl;
#endif
    const double newPercentWidth = double (width) * 100 / double (originalWidth ());

    SET_VALUE_WITHOUT_SIGNAL_EMISSION (m_percentWidthInput, newPercentWidth);

    widthFitHeightToAspectRatio ();

    //enableButtonOk (!isNoOp ());
    s_lastPercentWidth = newPercentWidth;
}

// public slot
void kpTransformResizeScaleDialog::slotHeightChanged (int height)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpTransformResizeScaleDialog::slotHeightChanged("
               << height << ")" << endl;
#endif
    const double newPercentHeight = double (height) * 100 / double (originalHeight ());

    SET_VALUE_WITHOUT_SIGNAL_EMISSION (m_percentHeightInput, newPercentHeight);

    heightFitWidthToAspectRatio ();

    //enableButtonOk (!isNoOp ());
    s_lastPercentHeight = newPercentHeight;
}

// public slot
void kpTransformResizeScaleDialog::slotPercentWidthChanged (double percentWidth)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpTransformResizeScaleDialog::slotPercentWidthChanged("
               << percentWidth << ")" << endl;
#endif

    SET_VALUE_WITHOUT_SIGNAL_EMISSION (m_newWidthInput,
                                       qRound (percentWidth * originalWidth () / 100.0));

    widthFitHeightToAspectRatio ();

    //enableButtonOk (!isNoOp ());
    s_lastPercentWidth = percentWidth;
}

// public slot
void kpTransformResizeScaleDialog::slotPercentHeightChanged (double percentHeight)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpTransformResizeScaleDialog::slotPercentHeightChanged("
               << percentHeight << ")" << endl;
#endif

    SET_VALUE_WITHOUT_SIGNAL_EMISSION (m_newHeightInput,
                                       qRound (percentHeight * originalHeight () / 100.0));

    heightFitWidthToAspectRatio ();

    //enableButtonOk (!isNoOp ());
    s_lastPercentHeight = percentHeight;
}

// public
bool kpTransformResizeScaleDialog::keepAspectRatio () const
{
    return m_keepAspectRatioCheckBox->isChecked ();
}

// public slot
void kpTransformResizeScaleDialog::setKeepAspectRatio (bool on)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpTransformResizeScaleDialog::setKeepAspectRatio("
               << on << ")" << endl;
#endif
    if (on != m_keepAspectRatioCheckBox->isChecked ())
        m_keepAspectRatioCheckBox->setChecked (on);

    if (on)
        widthFitHeightToAspectRatio ();
}

#undef IGNORE_KEEP_ASPECT_RATIO
#undef SET_VALUE_WITHOUT_SIGNAL_EMISSION


// private
int kpTransformResizeScaleDialog::originalWidth () const
{
    return document ()->width (actOnSelection ());
}

// private
int kpTransformResizeScaleDialog::originalHeight () const
{
    return document ()->height (actOnSelection ());
}


// public
int kpTransformResizeScaleDialog::imageWidth () const
{
    return m_newWidthInput->value ();
}

// public
int kpTransformResizeScaleDialog::imageHeight () const
{
    return m_newHeightInput->value ();
}

// public
bool kpTransformResizeScaleDialog::actOnSelection () const
{
    return (m_actOnCombo->currentIndex () == Selection);
}

// public
kpTransformResizeScaleCommand::Type kpTransformResizeScaleDialog::type () const
{
    if (m_resizeButton->isChecked ())
        return kpTransformResizeScaleCommand::Resize;
    else if (m_scaleButton->isChecked ())
        return kpTransformResizeScaleCommand::Scale;
    else
        return kpTransformResizeScaleCommand::SmoothScale;
}

// public
bool kpTransformResizeScaleDialog::isNoOp () const
{
    return (imageWidth () == originalWidth () &&
            imageHeight () == originalHeight ());
}


// private slot virtual [base QDialog]
void kpTransformResizeScaleDialog::accept ()
{
    enum { eText, eSelection, eImage } actionTarget = eText;

    if (actOnSelection ())
    {
        if (textSelection ())
        {
            actionTarget = eText;
        }
        else
        {
            actionTarget = eSelection;
        }
    }
    else
    {
        actionTarget = eImage;
    }


    KLocalizedString message;
    QString caption, continueButtonText;

    // Note: If eText, can't Scale nor SmoothScale.
    //       If eSelection, can't Resize.

    switch (type ())
    {
    default:
    case kpTransformResizeScaleCommand::Resize:
        if (actionTarget == eText)
        {
            message =
                ki18n ("<qt><p>Resizing the text box to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to resize the text box?</p></qt>");

            caption = i18n ("Resize Text Box?");
            continueButtonText = i18n ("R&esize Text Box");
        }
        else if (actionTarget == eImage)
        {
            message =
                ki18n ("<qt><p>Resizing the image to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to resize the image?</p></qt>");

            caption = i18n ("Resize Image?");
            continueButtonText = i18n ("R&esize Image");
        }

        break;

    case kpTransformResizeScaleCommand::Scale:
        if (actionTarget == eImage)
        {
            message =
                ki18n ("<qt><p>Scaling the image to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to scale the image?</p></qt>");

            caption = i18n ("Scale Image?");
            continueButtonText = i18n ("Scal&e Image");
        }
        else if (actionTarget == eSelection)
        {
            message =
                ki18n ("<qt><p>Scaling the selection to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to scale the selection?</p></qt>");

            caption = i18n ("Scale Selection?");
            continueButtonText = i18n ("Scal&e Selection");
        }

        break;

    case kpTransformResizeScaleCommand::SmoothScale:
        if (actionTarget == eImage)
        {
            message =
                ki18n ("<qt><p>Smooth Scaling the image to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to smooth scale the image?</p></qt>");

            caption = i18n ("Smooth Scale Image?");
            continueButtonText = i18n ("Smooth Scal&e Image");
        }
        else if (actionTarget == eSelection)
        {
            message =
                ki18n ("<qt><p>Smooth Scaling the selection to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to smooth scale the selection?</p></qt>");

            caption = i18n ("Smooth Scale Selection?");
            continueButtonText = i18n ("Smooth Scal&e Selection");
        }

        break;
    }


    if (kpTool::warnIfBigImageSize (originalWidth (),
            originalHeight (),
            imageWidth (), imageHeight (),
            message.subs (imageWidth ()).subs (imageHeight ()).toString (),
            caption,
            continueButtonText,
            this))
    {
        KDialog::accept ();
    }
}


#include <kpTransformResizeScaleDialog.moc>
