/***********************************************************************
 *
 *  KfArchiver.h
 *
 **********************************************************************/

#include <qstring.h>
#include <qstrlist.h>

#ifndef _KFARCH_H_
#define _KFARCH_H_

class KfArchiver
{
public:
  /**
    *Create a KfArchiver
    */
  KfArchiver() {};
      
  /**
    * Create a KfArchiver with archiver
    */
  KfArchiver(const QString&); 
      
  virtual ~KfArchiver() {}

  /** Initialize archivers
    * Call this function before you call any other function.
    * This function initializes global variable
    */
  static void init();

  /**
    * This function scans file ~/.kfindrc for archivers
    */
  static void initArchivers();

  /**
    * Returns archiver name
    */
  QString getArName() { return arName; };
      
  /** 
    * Returns comment for archiver
    */
  QString getArComment() { return arComment; };
  
  /**
    * Returns command for creating new archive
    */
  QString getOnCreate() { return arOnCreate; };
      
  /**
    * Returns command for updating archive
    */
  QString getOnUpdate() { return arOnUpdate; };
      
  /** Returns valid pattern for archive
    * Returns the extensions assoziated with this archiver, 
    * for example '.tar.gz' or '.zip'.
    */      
  QStrList& getArPattern() { return arPattern; };

  /**
    * Sets the comment
    */
  void setComment( const QString& _c) { arComment = _c; }

  /**
    * Sets the command for archive creating 
    */
  void setOnCreate( const QString& _command) { arOnCreate = _command; }

  /**
    * Sets the command for archive updating
    */
  void setOnUpdate( const QString& _command) { arOnUpdate = _command; }

  /**
    * Add a pattern which matches this type.
    */
  virtual void addPattern( const char *_p ) 
    { if ( arPattern.find( _p ) == -1 ) arPattern.append( _p ); }

  /** 
    *Find archiver by pattern.
    *
    * If for the given pattern no KfArchiver has been created, the function
    * will retun 0L;
    */
  static KfArchiver *findByPattern( const char *_pattern );
 
protected:
  QString arName;
  QString arComment;
  QString arOnCreate;
  QString arOnUpdate;
  QStrList arPattern;  
};

#endif

