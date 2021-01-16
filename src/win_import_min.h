
#ifndef WIN_IMPORT_MIN_H
#define WIN_IMPORT_MIN_H

#define __fastcall                   __attribute__((fastcall))

/* support from recent _mingw.h */

#ifdef __cplusplus
#define __forceinline inline __attribute__((__always_inline__))
#else
#define __forceinline extern __inline__ __attribute__((__always_inline__,__gnu_inline__))
#endif /* __cplusplus */

#ifdef __GNUC__
#define _byteswap_ulong(x)           __builtin_bswap32(x)
#endif

#define _read                        read
#define _lseeki64                    lseek
#define _close                       close
#define OutputDebugString(x)         fprintf(stderr,x)

/* gnu libc offers the equivalent 'aligned_alloc' BUT requested 'size'
   has to be a multiple of 'alignment' - in case it isn't, I'll set
   a different size, rounding up the value */
#define _aligned_malloc(s,a)         (                               \
                                     aligned_alloc(a,((s-1)/a+1)*a)  \
                                     )

#define _aligned_free(x)             free(x)

#define _atoi64(x)                   strtoll(x,NULL,10)
#define sprintf_s(buf,...)           snprintf((buf),sizeof(buf),__VA_ARGS__)
#define strncpy_s(d,n,s,c)           strncpy(d,s,c)
#define vsprintf_s(d,n,t,v)          vsprintf(d,t,v)
#define sscanf_s(buf,...)            sscanf((buf),__VA_ARGS__)
#define fscanf_s(f,t,...)            fscanf(f,t,__VA_ARGS__)

#endif // WIN_IMPORT_MIN_H

