
#ifndef VERSION_H

#define VERSION_H

// Note: last digit can be used for RC #
//Note: To change version, the following macros need to be modified (do not change anything else):


#define VER_MAJOR                   1
#define VER_MINOR                   7
#define VER_REVISION                0
#define VER_BUILD                   0

#define VER_XSTR(s)                 #s
#define VER_STR(s)                  VER_XSTR(s)


#define VER_PASTER(x,y,z,a)         0x ## x ## y ## z ## a
#define VER_EVALUATOR(x,y,z,a)      VER_PASTER(x,y,z,a)
#define VER_BUILDVERSION            VER_EVALUATOR(VER_MAJOR, VER_MINOR, VER_REVISION, VER_BUILD)

#define VER_FILEVERSION             VER_MAJOR,VER_MINOR,VER_REVISION,VER_BUILD
#define VER_FILEVERSION_STR         VER_STR(VER_MAJOR) "." VER_STR(VER_MINOR) "." VER_STR(VER_REVISION) "." VER_STR(VER_BUILD) "\0"

#define VER_PRODUCTVERSION          VER_MAJOR,VER_MINOR,VER_REVISION,VER_BUILD
#define VER_PRODUCTVERSION_STR      VER_STR(VER_MAJOR) "." VER_STR(VER_MINOR) VER_STR(VER_REVISION) "\0"

#define VER_COMPANYNAME_STR         "Singular Sound"
#define VER_FILEDESCRIPTION_STR     "BeatBuddy Manager Lean"
#define VER_INTERNALNAME_STR        "BBManagerLean"
#define VER_LEGALCOPYRIGHT_STR      "Copyright © 2014-2019 Singular Sound"
#define VER_LEGALTRADEMARKS1_STR    "All Rights Reserved"
#define VER_LEGALTRADEMARKS2_STR    VER_LEGALTRADEMARKS1_STR
#define VER_ORIGINALFILENAME_STR    "BBManager.exe"
#define VER_PRODUCTNAME_SHORT_STR   "BeatBuddy Manager Lean"
#define VER_PRODUCTNAME_STR         "BeatBuddy Manager Software Lean"

#define VER_COMPANYDOMAIN_STR       "singularsound.com"

#endif
