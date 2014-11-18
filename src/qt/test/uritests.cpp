#include "uritests.h"
#include "../guiutil.h"
#include "../walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?label=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN"));
    QVERIFY(rv.label == QString("Wikipedia Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?amount=1.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?amount=100&label=Wikipedia Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("Wikipedia Example"));

    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitcoinURI("fastcoin://fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?message=Wikipedia Example Address", &rv));
    QVERIFY(rv.address == QString("fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN"));
    QVERIFY(rv.label == QString());

    // We currently don't implement the message parameter (ok, yea, we break spec...)
    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?req-message=Wikipedia Example Address"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?amount=1,000&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("fastcoin:fpQXGrv3ChSdG9iurnP58wUUmZpqJXRgnN?amount=1,000.0&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));
}
