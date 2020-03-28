#ifndef STYLESHEETHELPER_H
#define STYLESHEETHELPER_H

#include <QObject>

class StyleSheetHelper : public QObject
{
   Q_OBJECT
public:
   ~StyleSheetHelper();

   static const QString & getStyleSheet();
   static const QString & getStateStyleSheet(bool selected);

signals:

public slots:

private:
   explicit StyleSheetHelper(QObject *parent = nullptr);

   static StyleSheetHelper *sp_Instance;
   QString *mp_MainStyleSheet;
   QString *mp_SelectedStyleSheet;
   QString *mp_UnselectedStyleSheet;

};

#endif // STYLESHEETHELPER_H
