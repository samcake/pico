#ifndef VISUALIZATION_DLL_MAIN_H_
# define VISUALIZATION_DLL_MAIN_H_

#ifdef PICO_DLL_LIB
# ifdef VISUALIZATION_EXPORTS
#  define VISUALIZATION_API __declspec(dllexport)
# else
#  define VISUALIZATION_API __declspec(dllimport)
# endif
#else 
# define VISUALIZATION_API 
#endif

#endif // !VISUALIZATION_DLL_MAIN_H_
