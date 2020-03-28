#ifndef PRAGMAPACK_H
#define PRAGMAPACK_H



#ifdef _MSC_VER
#   define PACK   __pragma(pack(push,1))
#   define PACKED __pragma(pack(pop))
#elif __clang__
#   define PACK
#   define PACKED __attribute__ ((packed))
#elif __MINGW32__
#   define PACK
#   define PACKED __attribute__ ((packed))
#else
#   define PACK
#   define PACKED
#endif


#endif // PRAGMAPACK_H
