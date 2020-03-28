#ifndef DEFAULTLIBRARY_H
#define DEFAULTLIBRARY_H

#include <QObject>
#include <QtCore>

class Workspace;
class LibContent;

class ContentLibrary : public QObject
{
   Q_OBJECT
public:
   explicit ContentLibrary(Workspace *p_parent, const QString &name);
   bool create(QWidget *p_parentWidget);
   void invalidate();
   void resetCurrentPath();

   inline const QString &name() const {return m_name;}
   QString key() const;
   inline bool isValid() const{return m_valid;}
   QDir dir() const;
   QString defaultPath() const;

   inline LibContent *libProjects()   {return mp_libProjects;}
   inline LibContent *libSongs()      {return mp_libSongs;}
   inline LibContent *libDrumSets()   {return mp_libDrumSets;}
   inline LibContent *libMidiSources(){return mp_libMidiSources;}
   inline LibContent *libWaveSources(){return mp_libWaveSources;}

signals:

public slots:

private:
   bool initialize();

private:
   Workspace *mp_parent;
   QString m_name;
   bool m_valid;
   QDir m_dir;

   LibContent *mp_libProjects;
   LibContent *mp_libSongs;
   LibContent *mp_libDrumSets;
   LibContent *mp_libMidiSources;
   LibContent *mp_libWaveSources;

};

#endif // DEFAULTLIBRARY_H
