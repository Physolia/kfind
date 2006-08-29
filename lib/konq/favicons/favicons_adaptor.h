/*
 * This file was generated by dbusidl2cpp version 0.5
 * when processing input file org.kde.FavIcon.xml
 *
 * The command line was:
 * dbusidl2cpp -m -a favicons_adaptor -c FavIconsAdaptor org.kde.FavIcon.xml
 *
 * dbusidl2cpp is Copyright (C) 2006 Trolltech AS. All rights reserved.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef FAVICONS_ADAPTOR_H_117111150580530
#define FAVICONS_ADAPTOR_H_117111150580530

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
class FavIconsModule;

/*
 * Adaptor class for interface org.kde.FavIcon
 */
class FavIconsAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.FavIcon")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.kde.FavIcon\" >\n"
"    <signal name=\"iconChanged\" >\n"
"      <arg direction=\"out\" type=\"b\" name=\"isHost\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"hostOrURL\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"iconName\" />\n"
"    </signal>\n"
"    <signal name=\"infoMessage\" >\n"
"      <arg direction=\"out\" type=\"s\" name=\"iconURL\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"msg\" />\n"
"    </signal>\n"
"    <method name=\"iconForUrl\" >\n"
"      <arg direction=\"out\" type=\"s\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"    </method>\n"
"    <method name=\"setIconForURL\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"iconURL\" />\n"
"    </method>\n"
"    <method name=\"downloadHostIcon\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"    </method>\n"
"  </interface>\n"
        "")
public:
    FavIconsAdaptor(FavIconsModule *parent);
    virtual ~FavIconsAdaptor();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    void downloadHostIcon(const QString &url);
    QString iconForUrl(const QString &url);
    void setIconForURL(const QString &url, const QString &iconURL);

Q_SIGNALS: // SIGNALS
    void iconChanged(bool isHost, const QString &hostOrURL, const QString &iconName);
    void infoMessage(const QString &iconURL, const QString &msg);
};

#endif
