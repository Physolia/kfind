/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_childview.h"
#include "konq_factory.h"
#include "konq_frame.h"
#include "konq_mainview.h"
#include "konq_propsview.h"
#include "konq_run.h"
#include "konq_events.h"
#include "konq_viewmgr.h"
#include <kio/job.h>

#include <assert.h>
#include <kdebug.h>

#include <qapplication.h>

#include <kparts/factory.h>

template class QList<HistoryEntry>;

KonqChildView::KonqChildView( KonqViewFactory &viewFactory,
			      KonqFrame* viewFrame,
			      KonqMainView *mainView,
			      const KService::Ptr &service,
			      const KTrader::OfferList &partServiceOffers,
			      const KTrader::OfferList &appServiceOffers,
			      const QString &serviceType
                              )
{
  m_pKonqFrame = viewFrame;
  m_pKonqFrame->setChildView( this );

  m_sLocationBarURL = "";
  m_bLockHistory = false;
  m_pMainView = mainView;
  m_pRun = 0L;
  m_pView = 0L;

  m_service = service;
  m_partServiceOffers = partServiceOffers;
  m_appServiceOffers = appServiceOffers;
  m_serviceType = serviceType;

  //  m_bAllowHTML = KonqPropsView::defaultProps( KonqFactory::instance() )->isHTMLAllowed();
  m_bAllowHTML = KonqFactory::defaultViewProps()->isHTMLAllowed();
  m_lstHistory.setAutoDelete( true );
  m_bLoading = false;
  m_bPassiveMode = false;
  m_bLinkedView = false;

  switchView( viewFactory );

  show();
}

KonqChildView::~KonqChildView()
{
  //kdDebug(1202) << "KonqChildView::~KonqChildView : part = " << m_pView << endl;

  delete m_pView;
  delete (KonqRun *)m_pRun;
  //kdDebug(1202) << "KonqChildView::~KonqChildView " << this << " done" << endl;
}

void KonqChildView::repaint()
{
//  kdDebug(1202) << "KonqChildView::repaint()" << endl;
  if (m_pKonqFrame != 0L)
    m_pKonqFrame->repaint();
//  kdDebug(1202) << "KonqChildView::repaint() : done" << endl;
}

void KonqChildView::show()
{
  kdDebug(1202) << "KonqChildView::show()" << endl;
  if ( m_pKonqFrame )
    m_pKonqFrame->show();
}

void KonqChildView::openURL( const KURL &url )
{
  setServiceTypeInExtension();

  if ( !m_bLockHistory )
  {
    // Store this new URL in the history, removing any existing forward history.
    // We do this first so that everything is ready if a part calls completed().
    createHistoryEntry();
  } else
      m_bLockHistory = false;

  m_pView->openURL( url );

  sendOpenURLEvent( url );

  //update metaviews!
  if ( m_metaView )
    m_metaView->openURL( url );

  updateHistoryEntry();

  //kdDebug(1202) << "Current position : " << m_lstHistory.at() << endl;
}

void KonqChildView::switchView( KonqViewFactory &viewFactory )
{
  kdDebug(1202) << "KonqChildView::switchView" << endl;
  if ( m_pView )
    m_pView->widget()->hide();

  KParts::ReadOnlyPart *oldView = m_pView;
  m_pView = m_pKonqFrame->attach( viewFactory );

  // uncomment if you want to use metaviews (Simon)
  // initMetaView();

  if ( oldView )
  {
    emit sigViewChanged( this, oldView, m_pView );

    delete oldView;
    //    closeMetaView(); I think this is not needed (Simon)
  }

  connectView();

  // Honour "non-removeable passive mode" (like the dirtree)
  QVariant prop = m_service->property( "X-KDE-BrowserView-PassiveMode");
  if ( prop.isValid() && prop.toBool() )
  {
    setPassiveMode( true ); // set as passive
    //TODO, perhaps: don't allow "Unlock all views" to unlock this view ?
    //frame()->statusbar()->hideStuff(); // prevent user from removing passive mode
  }

  // Honour "linked view"
  prop = m_service->property( "X-KDE-BrowserView-LinkedView");
  if ( prop.isValid() && prop.toBool() )
  {
    setLinkedView( true ); // set as linked
    if (m_pMainView->viewCount() == 1 || // can happen if this view is not yet in the map
        m_pMainView->viewCount() == 2)
    {
        // Two views : link both
        KonqMainView::MapViews mapViews = m_pMainView->viewMap();
        KonqMainView::MapViews::Iterator it = mapViews.begin();
        if ( (*it) == this )
            ++it;
        if ( it != mapViews.end() )
            (*it)->setLinkedView( true );
    }
  }
}

bool KonqChildView::changeViewMode( const QString &serviceType,
                                    const QString &serviceName,
                                    const KURL &url,
                                    const QString &locationBarURL)
{
  if ( m_bLoading )
  {
    stop();
  }

  // Since we only set URL to empty URL when calling this from go(), it
  // means we are going back or forward in the history and we already shifted
  // our position in the history -> don't update the history entry.
  if ( !url.isEmpty() )
  {
    if ( m_lstHistory.count() > 0 )
      updateHistoryEntry();
  }

  // Ok, now we can show (and store) the new location bar URL
  // (If we do that later, slotPartActivated will display the wrong one,
  //  and if we do that earlier, it messes up the history entry ;-)
  if ( !locationBarURL.isEmpty() )
      setLocationBarURL( locationBarURL );

  if ( !m_service->serviceTypes().contains( serviceType ) ||
       ( !serviceName.isEmpty() && serviceName != m_service->name() ) )
  {

    KTrader::OfferList partServiceOffers, appServiceOffers;
    KService::Ptr service = 0L;
    KonqViewFactory viewFactory = KonqFactory::createView( serviceType, serviceName, &service, &partServiceOffers, &appServiceOffers );

    if ( viewFactory.isNull() )
    {
      // Revert location bar's URL to the working one
      setLocationBarURL( history().current()->locationBarURL );
      return false;
    }

    m_service = service;
    m_partServiceOffers = partServiceOffers;
    m_appServiceOffers = appServiceOffers;
    m_serviceType = serviceType;

    switchView( viewFactory );
  }

  if ( !url.isEmpty() )
  {
    openURL( url );

    show();
    // Give focus to the view
    m_pView->widget()->setFocus();

    /*
      completed() does it now
    // openURL is the one that changes the history
    if ( m_pMainView->currentChildView() == this )
    {
      //kdDebug() << "updating toolbar actions" << endl;
      m_pMainView->updateToolBarActions();
    }
    */
  }
  return true;
}

void KonqChildView::connectView(  )
{
  //kdDebug(1202) << "KonqChildView::connectView" << endl;
  connect( m_pView, SIGNAL( started( KIO::Job * ) ),
           this, SLOT( slotStarted( KIO::Job * ) ) );
  connect( m_pView, SIGNAL( completed() ),
           this, SLOT( slotCompleted() ) );
  connect( m_pView, SIGNAL( canceled( const QString & ) ),
           this, SLOT( slotCanceled( const QString & ) ) );

  KParts::BrowserExtension *ext = browserExtension();

  if ( !ext )
    return;

  connect( ext, SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs &) ),
           m_pMainView, SLOT( openURL( const KURL &, const KParts::URLArgs & ) ) );

  connect( ext, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ),
           m_pMainView, SLOT( slotPopupMenu( const QPoint &, const KFileItemList & ) ) );

  connect( ext, SIGNAL( popupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ),
	   m_pMainView, SLOT( slotPopupMenu( const QPoint &, const KURL &, const QString &, mode_t ) ) );

  connect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ),
           m_pMainView, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KFileItemList & ) ) );

  connect( ext, SIGNAL( popupMenu( KXMLGUIClient *, const QPoint &, const KURL &, const QString &, mode_t ) ),
	   m_pMainView, SLOT( slotPopupMenu( KXMLGUIClient *, const QPoint &, const KURL &, const QString &, mode_t ) ) );

  connect( ext, SIGNAL( setLocationBarURL( const QString & ) ),
           this, SLOT( setLocationBarURL( const QString & ) ) );

  connect( ext, SIGNAL( createNewWindow( const KURL &, const KParts::URLArgs & ) ),
           m_pMainView, SLOT( slotCreateNewWindow( const KURL &, const KParts::URLArgs & ) ) );

  connect( ext, SIGNAL( loadingProgress( int ) ),
           m_pKonqFrame->statusbar(), SLOT( slotLoadingProgress( int ) ) );

  connect( ext, SIGNAL( speedProgress( int ) ),
           m_pKonqFrame->statusbar(), SLOT( slotSpeedProgress( int ) ) );

  connect( ext, SIGNAL( selectionInfo( const KFileItemList & ) ),
	   this, SLOT( slotSelectionInfo( const KFileItemList & ) ) );

  connect( ext, SIGNAL( openURLNotify() ),
	   this, SLOT( slotOpenURLNotify() ) );
}

void KonqChildView::slotStarted( KIO::Job * job )
{
  //kdDebug(1202) << "KonqChildView::slotStarted"  << job << endl;
  m_bLoading = true;

  if ( m_pMainView->currentChildView() == this )
  {
    m_pMainView->updateStatusBar();
    m_pMainView->updateToolBarActions();
  }

  if (job)
  {
      connect( job, SIGNAL( percent( KIO::Job *, unsigned long ) ), this, SLOT( slotPercent( KIO::Job *, unsigned long ) ) );
      connect( job, SIGNAL( speed( KIO::Job *, unsigned long ) ), this, SLOT( slotSpeed( KIO::Job *, unsigned long ) ) );
  }
}

void KonqChildView::slotPercent( KIO::Job *, unsigned long percent )
{
  m_pKonqFrame->statusbar()->slotLoadingProgress( percent );
}

void KonqChildView::slotSpeed( KIO::Job *, unsigned long bytesPerSecond )
{
  m_pKonqFrame->statusbar()->slotSpeedProgress( bytesPerSecond );
}

void KonqChildView::slotCompleted()
{
  //kdDebug() << "KonqChildView::slotCompleted" << endl;
  m_bLoading = false;
  m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );

  // Success... update history entry (mostly for location bar URL)
  updateHistoryEntry();

  if ( m_pMainView->currentChildView() == this )
  {
    //kdDebug() << "updating toolbar actions" << endl;
    m_pMainView->updateToolBarActions();
  }
}

void KonqChildView::slotCanceled( const QString & )
{
#ifdef __GNUC__
#warning TODO obey errMsg
#endif
  slotCompleted();
}

void KonqChildView::slotSelectionInfo( const KFileItemList &items )
{
  KonqFileSelectionEvent ev( items, m_pView );
  QApplication::sendEvent( m_pMainView, &ev );
}

void KonqChildView::setLocationBarURL( const QString & locationBarURL )
{
  //kdDebug(1202) << "KonqChildView::setLocationBarURL " << locationBarURL << endl;
  m_sLocationBarURL = locationBarURL;
  if ( m_pMainView->currentChildView() == this )
  {
    //kdDebug(1202) << "is current view" << endl;
    m_pMainView->setLocationBarURL( m_sLocationBarURL );
  }
}

void KonqChildView::slotOpenURLNotify()
{
  createHistoryEntry();
  updateHistoryEntry();
}

void KonqChildView::createHistoryEntry()
{
    // First, remove any forward history
    HistoryEntry * current = m_lstHistory.current();
    if (current)
    {
      //kdDebug(1202) << "Truncating history" << endl;
        m_lstHistory.at( m_lstHistory.count() - 1 ); // go to last one
        for ( ; m_lstHistory.current() != current ; )
        {
            if ( !m_lstHistory.removeLast() ) // and remove from the end (faster and easier)
                assert(0);
        }
        // Now current is the current again.
    }
    // Append a new entry
    //kdDebug(1202) << "Append a new entry" << endl;
    m_lstHistory.append( new HistoryEntry ); // made current
}

void KonqChildView::updateHistoryEntry()
{
  ASSERT( !m_bLockHistory ); // should never happen

  HistoryEntry * current = m_lstHistory.current();
  assert( current ); // let's see if this happens
  if ( current == 0L) // empty history
  {
    kdDebug(1202) << "Creating item because history is empty !" << endl;
    current = new HistoryEntry;
    m_lstHistory.append( current );
  }

  if ( browserExtension() )
  {
    QDataStream stream( current->buffer, IO_WriteOnly );

    browserExtension()->saveState( stream );
  }

  current->url = m_pView->url();
  //kdDebug(1202) << "Saving location bar URL : " << m_sLocationBarURL << endl;
  current->locationBarURL = m_sLocationBarURL;
  current->strServiceType = m_serviceType;
  current->strServiceName = m_service->name();
}

void KonqChildView::go( int steps )
{

  if ( m_lstHistory.count() > 0 )
    updateHistoryEntry();

  //kdDebug(1202) << "go : " << steps << endl;
  int newPos = m_lstHistory.at() + steps;
  assert( newPos >= 0 && (uint)newPos < m_lstHistory.count() );
  // Yay, we can move there without a loop !
  HistoryEntry *currentHistoryEntry = m_lstHistory.at( newPos ); // sets current item

  assert( currentHistoryEntry );
  assert( newPos == m_lstHistory.at() ); // check we moved (i.e. if I understood the docu)
  assert( currentHistoryEntry == m_lstHistory.current() );
  //kdDebug(1202) << "New position " << m_lstHistory.at() << endl;

  HistoryEntry h( *currentHistoryEntry ); // make a copy of the current history entry, as the data
                                          // the pointer points to will change with the following calls

  //kdDebug(1202) << "Restoring servicetype/name, and location bar URL from history : " << h.locationBarURL << endl;
  if ( ! changeViewMode( h.strServiceType, h.strServiceName, QString::null, h.locationBarURL ) )
  {
    kdWarning(1202) << "Couldn't change view mode to " << h.strServiceType
                    << " " << h.strServiceName << endl;
    return /*false*/;
  }

  setServiceTypeInExtension();

  if ( browserExtension() )
  {
    //kdDebug(1202) << "Restoring view from stream" << endl;
    QDataStream stream( h.buffer, IO_ReadOnly );

    browserExtension()->restoreState( stream );
  }
  else
    m_pView->openURL( h.url );

  sendOpenURLEvent( h.url );

  if ( m_pMainView->currentChildView() == this )
    m_pMainView->updateToolBarActions();

  if ( m_metaView )
    m_metaView->openURL( h.url );

  //kdDebug(1202) << "New position (2) " << m_lstHistory.at() << endl;
}

KURL KonqChildView::url()
{
  assert( m_pView );
  return m_pView->url();
}

void KonqChildView::setRun( KonqRun * run )
{
  m_pRun = run;
}

void KonqChildView::stop()
{
  if ( m_bLoading )
  {
    m_pView->closeURL();
  }
  else if ( m_pRun )
    delete static_cast<KonqRun *>(m_pRun); // should set m_pRun to 0L

  m_bLoading = false;
  m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );

    //  if ( m_pRun ) debug(" m_pRun is not NULL "); else debug(" m_pRun is NULL ");
  //if ( m_pRun ) delete (KonqRun *)m_pRun; // should set m_pRun to 0L
}

void KonqChildView::reload()
{
  //lockHistory();
  if ( browserExtension() )
  {
    KParts::URLArgs args(true, browserExtension()->xOffset(), browserExtension()->yOffset());
    args.serviceType = m_serviceType;
    browserExtension()->setURLArgs( args );
  }

  m_pView->openURL( m_pView->url() );

  // update metaview? (Simon)
}

void KonqChildView::setPassiveMode( bool mode )
{
  m_bPassiveMode = mode;

  if ( mode && m_pMainView->viewCount() > 1 && m_pMainView->currentChildView() == this )
    m_pMainView->viewManager()->setActivePart( m_pMainView->viewManager()->chooseNextView( this )->view() );

  // Hide the mode button for the last active view
  //
  /*
  KonqChildView *current = m_pMainView->currentChildView();

  if (current != 0L) {
    if ( m_pMainView->viewManager()->chooseNextView( current ) == 0L ) {
      current->frame()->statusbar()->passiveModeCheckBox()->hide();
    } else {
      current->frame()->statusbar()->passiveModeCheckBox()->show();
    }
  }
  */
}

void KonqChildView::setLinkedView( bool mode )
{
  m_bLinkedView = mode;
  frame()->statusbar()->setLinkedView( mode );
}

void KonqChildView::sendOpenURLEvent( const KURL &url )
{
  KParts::OpenURLEvent ev( m_pView, url );
  QApplication::sendEvent( m_pMainView, &ev );
}

void KonqChildView::initMetaView()
{
  kdDebug() << "initMetaView" << endl;

  static QString constr = QString::fromLatin1( "'Konqueror/MetaView' in ServiceTypes" );

  KTrader::OfferList metaViewOffers = KTrader::self()->query( m_serviceType, constr );

  if ( metaViewOffers.count() == 0 )
    return;

  kdDebug() << "got offers!" << endl;

  KService::Ptr service = *metaViewOffers.begin();

  KLibFactory *libFactory = KLibLoader::self()->factory( service->library() );

  if ( !libFactory )
    return;

  assert( libFactory->inherits( "KParts::Factory" ) ); //requirement! (Simon)

  QMap<QString,QVariant> framePropMap;

  bool embedInFrame = false;
  QVariant embedInFrameProp = service->property( "EmbedInFrame" );
  if ( embedInFrameProp.isValid() )
    embedInFrame = embedInFrameProp.toBool();

  if ( embedInFrame && m_pView->widget()->inherits( "QFrame" ) )
  {
    QFrame *frame = static_cast<QFrame *>( m_pView->widget() );
    framePropMap.insert( "frameShape", frame->property( "frameShape" ) );
    framePropMap.insert( "frameShadow", frame->property( "frameShadow" ) );
    framePropMap.insert( "lineWidth", frame->property( "lineWidth" ) );
    framePropMap.insert( "margin", frame->property( "margin" ) );
    framePropMap.insert( "midLineWidth", frame->property( "midLineWidth" ) );
    framePropMap.insert( "frameRect", frame->property( "frameRect" ) );
    frame->setFrameStyle( QFrame::NoFrame );
  }

  KParts::Factory *factory = static_cast<KParts::Factory *>( libFactory );

  KParts::Part *part = factory->createPart( m_pKonqFrame->metaViewFrame(), "metaviewwidget", m_pView, "metaviewpart", "KParts::ReadOnlyPart" );

  if ( !part )
   return;

  m_metaView = static_cast<KParts::ReadOnlyPart *>( part );

  m_pKonqFrame->attachMetaView( m_metaView, embedInFrame, framePropMap );

  m_metaView->widget()->show();

  m_pView->insertChildClient( m_metaView );
}

void KonqChildView::closeMetaView()
{
  if ( m_metaView )
    delete static_cast<KParts::ReadOnlyPart *>( m_metaView );

  m_pKonqFrame->detachMetaView();
}

void KonqChildView::setServiceTypeInExtension()
{
  KParts::BrowserExtension *ext = browserExtension();
  if ( !ext )
    return;

  KParts::URLArgs args( ext->urlArgs() );
  args.serviceType = m_serviceType;
  ext->setURLArgs( args );
}

QStringList KonqChildView::frameNames() const
{
  return childFrameNames( m_pView );
}

QStringList KonqChildView::childFrameNames( KParts::ReadOnlyPart *part )
{
  QStringList res;

  KParts::BrowserHostExtension *hostExtension = static_cast<KParts::BrowserHostExtension *>( part->child( 0L, "KParts::BrowserHostExtension" ) );

  if ( !hostExtension )
    return res;

  res += hostExtension->frameNames();

  const QList<KParts::ReadOnlyPart> children = hostExtension->frames();
  QListIterator<KParts::ReadOnlyPart> it( children );
  for (; it.current(); ++it )
    res += childFrameNames( it.current() );

  return res;
}

KParts::BrowserHostExtension *KonqChildView::hostExtension( KParts::ReadOnlyPart *part, const QString &name )
{
  KParts::BrowserHostExtension *ext = static_cast<KParts::BrowserHostExtension *>( part->child( 0L, "KParts::BrowserHostExtension" ) );

  if ( !ext )
    return 0;

  if ( ext->frameNames().contains( name ) )
    return ext;

  const QList<KParts::ReadOnlyPart> children = ext->frames();
  QListIterator<KParts::ReadOnlyPart> it( children );
  for (; it.current(); ++it )
  {
    KParts::BrowserHostExtension *childHost = hostExtension( it.current(), name );
    if ( childHost )
      return childHost;
  }

  return 0;
}

#include "konq_childview.moc"
