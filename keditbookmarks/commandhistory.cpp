/* This file is part of the KDE project
   Copyright (C) 2000, 2009 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "commandhistory.h"
#include <kdebug.h>
#include "commands.h"
#include <kactioncollection.h>
#include <kbookmarkmanager.h>
#include <QAction>
#include <QUndoCommand>

CommandHistory::CommandHistory(QObject* parent)
    : QObject(parent), m_manager(0), m_commandHistory()
{
}

void CommandHistory::setBookmarkManager(KBookmarkManager* manager)
{
    clearHistory(); // we can't keep old commands pointing to the wrong model/manager...
    m_manager = manager;
}

void CommandHistory::createActions(KActionCollection *actionCollection)
{
    // TODO use QUndoView?
    QAction* undoAction = m_commandHistory.createUndoAction(actionCollection);
    connect(undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    QAction* redoAction = m_commandHistory.createRedoAction(actionCollection);
    connect(redoAction, SIGNAL(triggered()), this, SLOT(redo()));
}

void CommandHistory::undo()
{
    const int idx = m_commandHistory.index();
    const QUndoCommand* cmd = m_commandHistory.command(idx-1);
    if (cmd) {
        m_commandHistory.undo();
        commandExecuted(cmd);
    }
}

void CommandHistory::redo()
{
    const int idx = m_commandHistory.index();
    const QUndoCommand* cmd = m_commandHistory.command(idx);
    if (cmd) {
        m_commandHistory.redo();
        commandExecuted(cmd);
    }
}

void CommandHistory::commandExecuted(const QUndoCommand *k)
{
    const IKEBCommand * cmd = dynamic_cast<const IKEBCommand *>(k);
    Q_ASSERT(cmd);

    KBookmark bk = m_manager->findByAddress(cmd->affectedBookmarks());
    Q_ASSERT(bk.isGroup());

    emit notifyCommandExecuted(bk.toGroup());
}

void CommandHistory::notifyDocSaved()
{
    m_commandHistory.setClean();
}

void CommandHistory::addCommand(QUndoCommand *cmd)
{
    if (!cmd)
        return;
    m_commandHistory.push(cmd); // calls cmd->redo()
    CommandHistory::commandExecuted(cmd);
}

void CommandHistory::clearHistory()
{
    if (m_commandHistory.count() > 0) {
        m_commandHistory.clear();
        emit notifyCommandExecuted(m_manager->root()); // not really, but we still want to update the GUI
    }
}

KBookmarkManager* CommandHistory::bookmarkManager()
{
    return m_manager;
}

#include "commandhistory.moc"
