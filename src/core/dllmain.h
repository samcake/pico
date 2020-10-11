#ifndef CORE_DLL_MAIN_H_
# define CORE_DLL_MAIN_H_

#ifdef PICO_DLL_LIB

# ifdef CORE_EXPORTS
#  define CORE_API __declspec(dllexport)
# else
#  define CORE_API __declspec(dllimport)
# endif

#else 
# define CORE_API 
#endif

#endif // !CORE_DLL_MAIN_H_
