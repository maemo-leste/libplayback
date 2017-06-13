/*
** Playback manager - some macros
** (c) Nokia MM - Makoto Sugano, 2007
** 
*/

#ifndef PLAYBACK_MACROS_H_
# define PLAYBACK_MACROS_H_

#define PB_STRINGIFY(macro_or_string)	PB_STRINGIFY_ARG (macro_or_string)
#define	PB_STRINGIFY_ARG(contents)	#contents

#ifndef TRUE
# define TRUE 1
#endif
#ifndef FALSE
# define FALSE 0
#endif

#ifndef NULL
# ifdef __cplusplus
#  define NULL        (0L)
# else /* !__cplusplus */
#  define NULL        ((void*) 0)
# endif /* !__cplusplus */
#endif

#ifdef  __cplusplus
# define PB_BEGIN_DECLS  extern "C" {
# define PB_END_DECLS    }
#else
# define PB_BEGIN_DECLS
# define PB_END_DECLS
#endif

/* Provide a string identifying the current code position */
#if defined(__GNUC__) && (__GNUC__ < 3) && !defined(__cplusplus)
#  define PB_STRLOC	__FILE__ ":" PB_STRINGIFY (__LINE__) ":" __PRETTY_FUNCTION__ "()"
#else
#  define PB_STRLOC	__FILE__ ":" PB_STRINGIFY (__LINE__)
#endif

#ifdef PLAYBACK_DEBUG
# define PB_LOG(...) do{ fprintf (stderr, PB_STRLOC ", " __VA_ARGS__ ); fprintf (stderr, "\n"); }while(0)
#else
# define PB_LOG(...)
#endif	/* !PLAYBACK_DEBUG */

#define pb_return_if_fail(Exp)	 do{			\
	if ((Exp) == FALSE) {				\
		PB_LOG ("assertion %s failed", #Exp);	\
		return ;				\
	}						\
}while(0)

#define pb_return_val_if_fail(Exp, Val)	 do{		\
	if ((Exp) == FALSE) {				\
		PB_LOG ("assertion %s failed", #Exp);	\
		return (Val);				\
	}						\
}while(0)

#define pb_n_elements(x) (sizeof(x)/sizeof(x[0]))

#endif	/* PLAYBACK_MACROS_H_ */
