#ifndef DIRCLEANUPMODAL_H
#define DIRCLEANUPMODAL_H

#include <QtCore>
#include <QtWidgets>

class DirCleanUpModal
{
public:
   explicit DirCleanUpModal(const QList<QString> &paths, QWidget *p_parent);
   explicit DirCleanUpModal(const QString &path, QWidget *p_parent);

   bool run();

private:
   QList<QString> m_paths;
   QWidget *mp_parent;
   bool m_aborted;
};

#endif // DIRCLEANUPMODAL_H
