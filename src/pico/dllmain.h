#ifndef PICO_DLL_MAIN_H_
# define PICO_DLL_MAIN_H_

#ifdef PICO_DLL_LIB

# ifdef CORE_EXPORTS
#  define CORE_API __declspec(dllexport)
# else
#  define CORE_API __declspec(dllimport)
# endif

# ifdef VISUALIZATION_EXPORTS
#  define VISUALIZATION_API __declspec(dllexport)
# else
#  define VISUALIZATION_API __declspec(dllimport)
# endif

#else 
# define CORE_API 
# define VISUALIZATION_API 
#endif

#endif // !PICO_DLL_MAIN_H_
