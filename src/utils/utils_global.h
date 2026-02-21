#ifndef LAKOT_UTILS_GLOBAL_H
#define LAKOT_UTILS_GLOBAL_H

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define LAKOT_UTILS_DECL_EXPORT __declspec(dllexport)
#  define LAKOT_UTILS_DECL_IMPORT __declspec(dllimport)
#else
#  define LAKOT_DECL_EXPORT     __attribute__((visibility("default")))
#  define LAKOT_DECL_IMPORT     __attribute__((visibility("default")))
#endif

#if defined(LAKOT_UTILS_LIBRARY)
#  define LAKOT_UTILS_EXPORT LAKOT_UTILS_DECL_EXPORT
#else
#  define LAKOT_UTILS_EXPORT LAKOT_UTILS_DECL_IMPORT
#endif

#endif
