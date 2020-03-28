#ifndef MININI_GLOBAL_H
#define MININI_GLOBAL_H

//#include <QtCore/qglobal.h>

#ifdef _MSC_VER
#  if defined(MININI_LIBRARY)
#    define MININISHARED_EXPORT __declspec(dllexport)
#  else
#    define MININISHARED_EXPORT __declspec(dllimport)
#  endif
#else
#  define MININISHARED_EXPORT
#endif

#endif // MININI_GLOBAL_H
