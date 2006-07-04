/*
 * This file was generated by dbusxml2cpp version 0.6
 * Command line was: dbusxml2cpp -m -p callback_proxy -- ../org.kde.nsplugins.CallBack.xml
 *
 * dbusxml2cpp is Copyright (C) 2006 Trolltech AS. All rights reserved.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef CALLBACK_PROXY_H_285231151890622
#define CALLBACK_PROXY_H_285231151890622

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface org.kde.nsplugins.CallBack
 */
class Q_DECL_EXPORT OrgKdeNspluginsCallBackInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.kde.nsplugins.CallBack"; }

public:
    OrgKdeNspluginsCallBackInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~OrgKdeNspluginsCallBackInterface();

public Q_SLOTS: // METHODS
    inline Q_NOREPLY void evalJavaScript(int id, const QString &script)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(id) << qVariantFromValue(script);
        callWithArgumentList(QDBus::NoBlock, QLatin1String("evalJavaScript"), argumentList);
    }

    inline Q_NOREPLY void postURL(const QString &url, const QString &target, const QByteArray &data, const QString &mime)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(url) << qVariantFromValue(target) << qVariantFromValue(data) << qVariantFromValue(mime);
        callWithArgumentList(QDBus::NoBlock, QLatin1String("postURL"), argumentList);
    }

    inline Q_NOREPLY void requestURL(const QString &url, const QString &target)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(url) << qVariantFromValue(target);
        callWithArgumentList(QDBus::NoBlock, QLatin1String("requestURL"), argumentList);
    }

    inline Q_NOREPLY void statusMessage(const QString &msg)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(msg);
        callWithArgumentList(QDBus::NoBlock, QLatin1String("statusMessage"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

namespace org {
  namespace kde {
    namespace nsplugins {
      typedef ::OrgKdeNspluginsCallBackInterface CallBack;
    }
  }
}
#endif
