#include "splashscreen.h"
#include "clientversion.h"
#include "util.h"

#include <QPainter>
#include <QApplication>

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f):
    QSplashScreen(pixmap, f)
{

    // set reference point, paddings
    int paddingRight            = 30;
    int paddingTop              = 50;
    int titleVersionVSpace      = 17;
    int titleCopyrightVSpace    = 40;

    float fontFactor            = 1.0;

    // define text to place
    QString titleText       = QString(QApplication::applicationName()).replace(QString("-testnet"), QString(""), Qt::CaseSensitive); // cut of testnet, place it as single object further down
    QString versionText     = QString("Version %1").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText   = QChar(0xA9)+QString(" 2009-%1 ").arg(COPYRIGHT_YEAR) + QString(tr("The Fastcoin developers"));
    QString testnetAddText  = QString(tr("[testnet]")); // define text to place as single text object

    QString font            = "Arial";

    // load the bitmap for writing some text over it
    QPixmap newPixmap;
    if(GetBoolArg("-testnet")) {
        newPixmap     = QPixmap(":/images/splash_testnet");
    }
    else {
        newPixmap     = QPixmap(":/images/splash");
    }

    QPainter pixPaint(&newPixmap);
    pixPaint.setPen(QColor(100,100,100));

    // check font size and drawing with
    pixPaint.setFont(QFont(font, 28*fontFactor));
    QFontMetrics fm = pixPaint.fontMetrics();
    int titleTextWidth  = fm.width(titleText);
    if(titleTextWidth > 160) {
        // strange font rendering, Arial probably not found
        fontFactor = 0.75;
    }

    QFont fz(font, 28*fontFactor);
    fz.setStyleStrategy(QFont::PreferAntialias);
    pixPaint.setFont(fz);
    fm = pixPaint.fontMetrics();
    titleTextWidth  = fm.width(titleText);
    pixPaint.drawText(newPixmap.width()-titleTextWidth-paddingRight,paddingTop,titleText); //newPixmap.width()-titleTextWidth-

    fz.setPixelSize(11);
    pixPaint.setFont(fz);

    // if the version string is to long, reduce size
    fm = pixPaint.fontMetrics();
    int versionTextWidth  = fm.width(versionText);
    if(versionTextWidth > titleTextWidth+paddingRight-10) {
        pixPaint.setFont(QFont(font, 10*fontFactor));
        titleVersionVSpace -= 5;
    }
    pixPaint.drawText(newPixmap.width()-titleTextWidth-paddingRight+2,paddingTop+titleVersionVSpace,versionText); //

    fz.setPixelSize(8.5);
    pixPaint.setFont(fz);

    pixPaint.drawText(newPixmap.width()-titleTextWidth-paddingRight,paddingTop+titleCopyrightVSpace,copyrightText); //newPixmap.width()-titleTextWidth-

    // draw testnet string if -testnet is on
    if(QApplication::applicationName().contains(QString("-testnet"))) {
        // draw copyright stuff
        QFont boldFont = QFont(font, 10);
        boldFont.setWeight(QFont::Bold);
        pixPaint.setFont(boldFont);
        fm = pixPaint.fontMetrics();
        int testnetAddTextWidth  = fm.width(testnetAddText);
        pixPaint.drawText(newPixmap.width()-testnetAddTextWidth-10,15,testnetAddText);
    }

    pixPaint.end();

    //showmstatusmessage
    fz.setPixelSize(11);
    QRect r(15, 310, 500, 15);
    this->setMessageRect(r, Qt::AlignCenter); // Setting the message position.
    this->setFont(fz);

    this->setPixmap(newPixmap);
}

void SplashScreen::drawContents(QPainter *painter)
{
 QPixmap textPix = QSplashScreen::pixmap();
 painter->setPen(this->color);
 painter->drawText(this->rect, this->alignement, this->message);
}

void SplashScreen::showStatusMessage(const QString &message, const QColor &color)
{
 this->message = message;
 this->color = color;
 this->showMessage(this->message, this->alignement, this->color);
}

void SplashScreen::setMessageRect(QRect rect, int alignement)
{
 this->rect = rect;
 this->alignement = alignement;
}
