#ifndef DOCUMENT_DLL_MAIN_H_
# define DOCUMENT_DLL_MAIN_H_

#ifdef PICO_DLL_LIB

# ifdef DOCUMENT_EXPORTS
#  define DOCUMENT_API __declspec(dllexport)
# else
#  define DOCUMENT_API __declspec(dllimport)
# endif

#else 
# define DOCUMENT_API 
#endif

#endif // !DOCUMENT_DLL_MAIN_H_
