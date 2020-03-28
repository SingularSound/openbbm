#ifndef PORTABLESONGFILE_H
#define PORTABLESONGFILE_H

#include "../../../version.h"
#include "../../beatsmodelfiles.h"

/**
 *   PORTABLESONGFILE Version #0 :
 *       Revision 0 : Original Format and features. No version file
 *   PORTABLESONGFILE Version #1 :
 *       Revision 0 : - Added version file VERSION.BCF.
 *                    - Song file names are now CRC of the song names.
 *                    - Added CONFIG.CSV file
 */

#define PORTABLESONGFILE_VERSION  1u
#define PORTABLESONGFILE_REVISION 0u
#define PORTABLESONGFILE_BUILD    VER_BUILDVERSION

/**
 *   PORTABLEFOLDERFILE Version #0 :
 *       Revision 0 : Original Format and features. No version file
 *   PORTABLEFOLDERFILE Version #1 :
 *       Revision 0 : - Added version file VERSION.BCF.
 *                    - Used quazip to zip. Format does not seem to be supported by QT zip reader
 */

#define PORTABLEFOLDERFILE_VERSION  1u
#define PORTABLEFOLDERFILE_REVISION 0u
#define PORTABLEFOLDERFILE_BUILD    VER_BUILDVERSION

#endif // PORTABLESONGFILE_H
