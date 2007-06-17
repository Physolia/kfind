/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   and Patrice Tremblay                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/
#ifndef STATUSBARSPACEINFO_H
#define STATUSBARSPACEINFO_H

#include <kurl.h>

#include <QtGui/QColor>
#include <QtGui/QKeyEvent>
#include <QtGui/QWidget>

class KDiskFreeSp;

/**
 * @short Shows the available space for the volume represented
 *        by the given URL as part of the status bar.
 */
class StatusBarSpaceInfo : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBarSpaceInfo(QWidget* parent);
    virtual ~StatusBarSpaceInfo();

    void setUrl(const KUrl& url);
    const KUrl& url() const
    {
        return m_url;
    }

protected:
    /** @see QWidget::paintEvent() */
    virtual void paintEvent(QPaintEvent* event);

private slots:
    /**
     * The strange signature of this method is due to a compiler
     * bug (?). More details are given inside the class KDiskFreeSp (see
     * KDE Libs documentation).
     */
    void slotFoundMountPoint(const quint64& kBSize,
                             const quint64& kBUsed,
                             const quint64& kBAvailable,
                             const QString& mountPoint);
    void showResult();

    /** Refreshs the space information for the current set URL. */
    void refresh();

private:
    /**
     * Returns a color for the progress bar by respecting
     * the given background color \a bgColor. It is assured
     * that enough contrast is given to have a visual indication.
     */
    QColor progressColor(const QColor& bgColor) const;

private:
    KUrl m_url;
    bool m_gettingSize;
    quint64 m_kBSize;
    quint64 m_kBAvailable;
};

#endif
