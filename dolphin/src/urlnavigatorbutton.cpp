/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
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

#include "urlnavigatorbutton.h"

#include <assert.h>

#include "urlnavigator.h"

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kglobalsettings.h>
#include <kmenu.h>

#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

UrlNavigatorButton::UrlNavigatorButton(int index, UrlNavigator* parent) :
    UrlButton(parent),
    m_index(-1),
    m_popupDelay(0),
    m_listJob(0)
{
    setAcceptDrops(true);
    setMinimumWidth(arrowWidth());
    setIndex(index);
    connect(this, SIGNAL(clicked()), this, SLOT(updateNavigatorUrl()));

    m_popupDelay = new QTimer(this);
    m_popupDelay->setSingleShot(true);
    connect(m_popupDelay, SIGNAL(timeout()), this, SLOT(startListJob()));
    connect(this, SIGNAL(pressed()), this, SLOT(startPopupDelay()));
}

UrlNavigatorButton::~UrlNavigatorButton()
{
}

void UrlNavigatorButton::setIndex(int index)
{
    m_index = index;

    if (m_index < 0) {
        return;
    }

    QString path(urlNavigator()->url().pathOrUrl());
    setText(path.section('/', index, index));

    // Check whether the button indicates the full path of the Url. If
    // this is the case, the button is marked as 'active'.
    ++index;
    QFont adjustedFont(font());
    if (path.section('/', index, index).isEmpty()) {
        setDisplayHintEnabled(ActivatedHint, true);
        adjustedFont.setBold(true);
    }
    else {
        setDisplayHintEnabled(ActivatedHint, false);
        adjustedFont.setBold(false);
    }

    setFont(adjustedFont);
    update();
}

QSize UrlNavigatorButton::sizeHint() const
{
    const int width = fontMetrics().width(text()) + (arrowWidth() * 4);
    return QSize(width, UrlButton::sizeHint().height());
}

void UrlNavigatorButton::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setClipRect(event->rect());
    const int buttonWidth  = width();
    const int buttonHeight = height();

    QColor backgroundColor;
    QColor foregroundColor;
    const bool isHighlighted = isDisplayHintEnabled(EnteredHint) ||
                               isDisplayHintEnabled(DraggedHint) ||
                               isDisplayHintEnabled(PopupActiveHint);
    if (isHighlighted) {
        backgroundColor = KGlobalSettings::highlightColor();
        foregroundColor = KGlobalSettings::highlightedTextColor();
    }
    else {
        backgroundColor = palette().brush(QPalette::Background).color();
        foregroundColor = KGlobalSettings::buttonTextColor();
    }

    // dimm the colors if the parent view does not have the focus
    const bool isActive = urlNavigator()->isActive();
    if (!isActive) {
        QColor dimmColor(palette().brush(QPalette::Background).color());
        foregroundColor = mixColors(foregroundColor, dimmColor);
        if (isHighlighted) {
            backgroundColor = mixColors(backgroundColor, dimmColor);
        }
    }

    // draw button background
    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor);
    painter.drawRect(0, 0, buttonWidth, buttonHeight);

    int textWidth = buttonWidth;
    if (isDisplayHintEnabled(ActivatedHint) && isActive || isHighlighted) {
        painter.setPen(foregroundColor);
    }
    else {
        // dimm the foreground color by mixing it with the background
        foregroundColor = mixColors(foregroundColor, backgroundColor);
        painter.setPen(foregroundColor);
    }

    if (!isDisplayHintEnabled(ActivatedHint)) {
        // draw arrow
        const int border = 2;  // horizontal border
        const int middleY = height() / 2;
        const int width = arrowWidth();
        const int startX = (buttonWidth - width) - (2 * border);
        const int startTopY = middleY - (width - 1);
        const int startBottomY = middleY + (width - 1);
        for (int i = 0; i < width; ++i) {
            painter.drawLine(startX, startTopY + i, startX + i, startTopY + i);
            painter.drawLine(startX, startBottomY - i, startX + i, startBottomY - i);
        }

        textWidth = startX - border;
    }

    const bool clipped = isTextClipped();
    const int align = clipped ? Qt::AlignVCenter : Qt::AlignCenter;
    const QRect textRect(0, 0, textWidth, buttonHeight);
    if (clipped) {
        QLinearGradient gradient(textRect.topLeft(), textRect.topRight());
        gradient.setColorAt(0.8, foregroundColor);
        gradient.setColorAt(1.0, backgroundColor);

        QPen pen;
        pen.setBrush(QBrush(gradient));
        painter.setPen(pen);
        painter.drawText(textRect, align, text());
    }
    else {
        painter.drawText(textRect, align, text());
    }
}

void UrlNavigatorButton::enterEvent(QEvent* event)
{
    UrlButton::enterEvent(event);

    // if the text is clipped due to a small window width, the text should
    // be shown as tooltip
    if (isTextClipped()) {
        setToolTip(text());
    }
}

void UrlNavigatorButton::leaveEvent(QEvent* event)
{
    UrlButton::leaveEvent(event);
    setToolTip(QString());
}

void UrlNavigatorButton::dropEvent(QDropEvent* event)
{
    if (m_index < 0) {
        return;
    }

    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        event->acceptProposedAction();

        setDisplayHintEnabled(DraggedHint, true);

        QString path(urlNavigator()->url().prettyUrl());
        path = path.section('/', 0, m_index + 2);

        urlNavigator()->dropUrls(urls, KUrl(path));

        setDisplayHintEnabled(DraggedHint, false);
        update();
    }
}

void UrlNavigatorButton::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        setDisplayHintEnabled(DraggedHint, true);
        event->acceptProposedAction();

        update();
    }
}

void UrlNavigatorButton::dragLeaveEvent(QDragLeaveEvent* event)
{
    UrlButton::dragLeaveEvent(event);

    setDisplayHintEnabled(DraggedHint, false);
    update();
}


void UrlNavigatorButton::updateNavigatorUrl()
{
    stopPopupDelay();

    if (m_index < 0) {
        return;
    }

    urlNavigator()->setUrl(urlNavigator()->url(m_index));
}

void UrlNavigatorButton::startPopupDelay()
{
    if (m_popupDelay->isActive() || (m_listJob != 0) || (m_index < 0)) {
        return;
    }

    m_popupDelay->start(300);
}

void UrlNavigatorButton::stopPopupDelay()
{
    m_popupDelay->stop();
    if (m_listJob != 0) {
        m_listJob->kill();
        m_listJob = 0;
    }
}

void UrlNavigatorButton::startListJob()
{
    if (m_listJob != 0) {
        return;
    }

    const KUrl& url = urlNavigator()->url(m_index);
    m_listJob = KIO::listDir(url, false, urlNavigator()->showHiddenFiles());
    m_subdirs.clear(); // just to be ++safe

    connect(m_listJob, SIGNAL(entries(KIO::Job*, const KIO::UDSEntryList &)),
            this, SLOT(entriesList(KIO::Job*, const KIO::UDSEntryList&)));
    connect(m_listJob, SIGNAL(result(KJob*)), this, SLOT(listJobFinished(KJob*)));
}

void UrlNavigatorButton::entriesList(KIO::Job* job, const KIO::UDSEntryList& entries)
{
    if (job != m_listJob) {
        return;
    }

    KIO::UDSEntryList::const_iterator it = entries.constBegin();
    KIO::UDSEntryList::const_iterator itEnd = entries.constEnd();

    bool showHidden = urlNavigator()->showHiddenFiles();
    while (it != itEnd) {
        QString name;
        //bool isDir = false;
        KIO::UDSEntry entry = *it;

        /* KDE3 reference:
            KIO::UDSEntry::const_iterator atomIt = entry.constBegin();
            KIO::UDSEntry::const_iterator atomEndIt = entry.constEnd();

            while (atomIt != atomEndIt) {
            switch ((*atomIt).m_uds) {
                case KIO::UDS_NAME:
                    name = (*atomIt).m_str;
                    break;
                case KIO::UDS_FILE_TYPE:
                    isDir = S_ISDIR((*atomIt).m_long);
                    break;
                default:
                    break;
            }
            ++atomIt;
         }
         if (isDir) {
             m_subdirs.append(name);
         }
        */

        if (entry.isDir()) {
            QString dir = entry.stringValue(KIO::UDS_NAME);

            if (!showHidden || (dir != "." && dir != "..")) {
                m_subdirs.append(entry.stringValue(KIO::UDS_NAME));
            }
        }

        ++it;
    }

    m_subdirs.sort();
}

void UrlNavigatorButton::listJobFinished(KJob* job)
{
    if (job != m_listJob) {
        return;
    }

    if (job->error() || m_subdirs.isEmpty()) {
        // clear listing
        return;
    }

    setDisplayHintEnabled(PopupActiveHint, true);
    update(); // ensure the button is drawn highlighted

    KMenu* dirsMenu = new KMenu(this);
    QStringList::const_iterator it = m_subdirs.constBegin();
    QStringList::const_iterator itEnd = m_subdirs.constEnd();
    int i = 0;
    while (it != itEnd) {
        QAction* action = new QAction(*it, this);
        action->setData(i);
        dirsMenu->addAction(action);
        ++it;
        ++i;
    }

    const QAction* action = dirsMenu->exec(urlNavigator()->mapToGlobal(geometry().bottomLeft()));
    if (action != 0) {
        const int result = action->data().toInt();
        KUrl url = urlNavigator()->url(m_index);
        url.addPath(m_subdirs[result]);
        urlNavigator()->setUrl(url);
    }

    m_listJob = 0;
    m_subdirs.clear();
    delete dirsMenu;
    dirsMenu = 0;

    setDisplayHintEnabled(PopupActiveHint, false);
}

int UrlNavigatorButton::arrowWidth() const
{
    int width = (height() / 2) - 7;
    if (width < 4) {
        width = 4;
    }
    return width;
}

bool UrlNavigatorButton::isTextClipped() const
{
    int availableWidth = width();
    if (!isDisplayHintEnabled(ActivatedHint)) {
        availableWidth -= arrowWidth() + 1;
    }

    QFontMetrics fontMetrics(font());
    return fontMetrics.width(text()) >= availableWidth;
}

#include "urlnavigatorbutton.moc"
