#ifndef UIX_DLL_MAIN_H_
# define UIX_DLL_MAIN_H_

#ifdef PICO_DLL_LIB

# ifdef UIX_EXPORTS
#  define UIX_API __declspec(dllexport)
# else
#  define UIX_API __declspec(dllimport)
# endif

#else 
# define UIX_API 
#endif

#endif // !UIX_DLL_MAIN_H_
