/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Implementation of spawnve() for DOS, OS/2, and Win32.
*
****************************************************************************/


#undef __INLINE_FUNCTIONS__
#include "widechar.h"
#include "variety.h"
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <process.h>
#ifdef __DOS__
    #include <dos.h>
#endif
#include "rtdata.h"
#include "rterrno.h"
#include "liballoc.h"
#include "filestr.h"
#include "msdos.h"
#include "_process.h"
#include "thread.h"
#include "pathmac.h"

#ifdef __USE_POSIX_HANDLE_STRINGS
    #define _POSIX_HANDLE_CLEANUP   {                                   \
                                        if( doFreeFlag ) {              \
                                            lib_free( (void *)envp );   \
                                            lib_free( fileinfo );       \
                                            doFreeFlag = 0;             \
                                        }                               \
                                    }

#else
    #define _POSIX_HANDLE_CLEANUP
#endif

#if defined( __OS2__ ) && defined( __BIG_DATA__ )
    #define LIB_ALLOC   lib_fmalloc
    #define LIB_FREE    lib_ffree
#else
    #define LIB_ALLOC   lib_nmalloc
    #define LIB_FREE    lib_nfree
#endif

#if defined( __DOS__ ) && defined( _M_I86 )
    #define ENV_ARG     unsigned
    #define ENVPARM     envseg
    #define SPVE_NEAR   _WCI86NEAR      //__based( __segname( "_STACK" ) )
#else
    #define ENV_ARG     CHAR_TYPE *
    #define ENVPARM     envmem
    #define SPVE_NEAR
#endif

#if defined( __DOS__ )
extern int  __dospawn( int mode, char SPVE_NEAR *pgmname, char SPVE_NEAR *cmdline, ENV_ARG env );
#pragma aux __dospawn "_*" parm caller [];
#endif

/* P_OVERLAY macro expands to a variable, not a constant! */
#define OLD_P_OVERLAY   2

#define FALSE   0

static int file_exists( const CHAR_TYPE *filename )                     /* 05-apr-91 */
{
#if defined(__OS2__) || defined( __NT__ )
    if( __F_NAME(access,_waccess)( filename, 0 ) == 0 )
        return( 1 );
#else
    /* should use _dos_findfirst to avoid DOS APPEND bug */
    struct find_t  find_buf;

    if( _dos_findfirst( filename,
            _A_NORMAL | _A_RDONLY | _A_HIDDEN | _A_SYSTEM, &find_buf ) == 0 )
        return( 1 );
#endif
    return( 0 );
}

#if defined( __DOS__ )
static int _dospawn( int mode, char SPVE_NEAR *pgmname, char SPVE_NEAR *cmdline, ENV_ARG env, const char * const *argv )
{
    /* do this here instead of in the .asm files */
    __ccmdline( pgmname, argv, cmdline, 0 );
    return( __dospawn( mode, pgmname, cmdline, env ) );
}
#endif


#pragma on(check_stack);

_WCRTLINK int __F_NAME(spawnve,_wspawnve)( int mode, const CHAR_TYPE * path,
                                          const CHAR_TYPE * const argv[], const CHAR_TYPE * const in_envp[] )
{
    const CHAR_TYPE * const *envp = (const CHAR_TYPE * const *)in_envp;
    CHAR_TYPE               *envmem;
    CHAR_TYPE               *envstrings;
    unsigned                envseg;
    int                     len;
    CHAR_TYPE SPVE_NEAR     *np;
    CHAR_TYPE SPVE_NEAR     *p;
    CHAR_TYPE SPVE_NEAR     *end_of_p;
    int                     retval;
    int                     num_of_paras;       /* for environment */
    size_t                  cmdline_len;
    CHAR_TYPE SPVE_NEAR     *cmdline_mem;
    CHAR_TYPE SPVE_NEAR     *cmdline;
    CHAR_TYPE               switch_c[4];
    unsigned char           prot_mode286;
    unsigned char           use_cmd;
#if defined( __DOS__ )
    auto _87state           _87save;
#endif
    CHAR_TYPE               *drive;
    CHAR_TYPE               *dir;
    CHAR_TYPE               *fname;
    CHAR_TYPE               *ext;
    int                     rc;

#ifdef __USE_POSIX_HANDLE_STRINGS
    CHAR_TYPE               **newEnvp;
    int                     count;
    CHAR_TYPE               *fileinfo;
    int                     doFreeFlag = 0;

    if( _fileinfo != 0 ) {
        fileinfo = __F_NAME(__FormPosixHandleStr,__wFormPosixHandleStr)();
        if( fileinfo != NULL ) {
            count = 1;
            if( envp != NULL ) {
                newEnvp = (CHAR_TYPE **)envp;
                while( *newEnvp != NULL ) { /* count 'em, including NULL */
                    newEnvp++;
                    count++;
                }
            }
            count++;                        /* one more for the new entry */
            newEnvp = lib_malloc( count * sizeof(CHAR_TYPE*) );
            if( newEnvp != NULL ) {
                if( envp != NULL ) {
                    memcpy( newEnvp, envp, (count-1)*sizeof(CHAR_TYPE*) );
                }
                newEnvp[count-2] = fileinfo;/* add C_FILE_INFO entry */
                newEnvp[count-1] = NULL;    /* terminate pointer list */
                envp = (const CHAR_TYPE **)newEnvp;/* use new environment */
                doFreeFlag = 1;             /* do cleanup on exit */
            } else {
                lib_free( fileinfo );
            }
        }
    }
#endif

#if defined( _M_I86 )
 #if defined( __OS2__ )
    prot_mode286 = ( _RWD_osmode != DOS_MODE );
    if( mode == OLD_P_OVERLAY ) {
        rc = execve(path, argv, envp);
        _POSIX_HANDLE_CLEANUP;
        return( rc );
    }
 #else
    prot_mode286 = FALSE;
    if( mode == OLD_P_OVERLAY ) {
        execveaddr_type    execve;
        execve = __execaddr();
        if( execve != NULL ) {
            rc = (*execve)( path, argv, envp );
            _POSIX_HANDLE_CLEANUP;
            return( rc );
        }
        _RWD_errno = EINVAL;
        _POSIX_HANDLE_CLEANUP;
        return( -1 );
    }
 #endif
    use_cmd = prot_mode286;
#else   // 32-bit
    prot_mode286 = FALSE;

 #if defined(__OS2__) || defined(__NT__)
    use_cmd = 1;
    if( mode == OLD_P_OVERLAY ) {
        rc = __F_NAME(execve,_wexecve)(path, argv, envp);
        _POSIX_HANDLE_CLEANUP;
        return( rc );
    }
 #else      // __DOS__
    use_cmd = 0;
    if( mode >= OLD_P_OVERLAY ) {
        _RWD_errno = EINVAL;
        rc = -1;
        _POSIX_HANDLE_CLEANUP;
        return( rc );
    }
 #endif
#endif
    retval = __F_NAME(__cenvarg,__wcenvarg)( argv, envp, &envmem,
        &envstrings, &envseg, &cmdline_len, FALSE );
    if( retval == -1 ) {
        _POSIX_HANDLE_CLEANUP;
        return( -1 );
    }
    num_of_paras = retval;
    len = __F_NAME(strlen,wcslen)( path ) + 7 + _MAX_PATH2;
    np = LIB_ALLOC( len * sizeof( CHAR_TYPE ) );
    if( np == NULL ) {
        p = (CHAR_TYPE SPVE_NEAR *)alloca( len * sizeof( CHAR_TYPE ) );
        if( p == NULL ) {
            lib_free( envmem );
            _POSIX_HANDLE_CLEANUP;
            return( -1 );
        }
    } else {
        p = np;
    }
    __F_NAME(_splitpath2,_wsplitpath2)( path, p + ( len - _MAX_PATH2 ),
                                        &drive, &dir, &fname, &ext );
#if defined( __DOS__ )
    _RWD_Save8087( &_87save );
#endif
#if defined( __DOS__ ) && defined( _M_I86 )
    if( _RWD_osmode != DOS_MODE ) {     /* if protect-mode e.g. DOS/16M */
        unsigned    segment;

        if( _dos_allocmem( num_of_paras, &segment ) != 0 ) {
            lib_nfree( np );
            lib_free( envmem );
            _POSIX_HANDLE_CLEANUP;
            return( -1 );
        }
        envseg = segment;
        _fmemcpy( MK_FP( segment, 0 ), envstrings, num_of_paras * 16 );
    }
#endif
    /* allocate the cmdline buffer */
    cmdline_mem = LIB_ALLOC( cmdline_len * sizeof( CHAR_TYPE ) );
    if( cmdline_mem == NULL ) {
        cmdline = (CHAR_TYPE SPVE_NEAR *)alloca( cmdline_len * sizeof( CHAR_TYPE ) );
        if( cmdline == NULL ) {
            retval = -1;
            _RWD_errno = E2BIG;
            _RWD_doserrno = E_badenv;
        }
    } else {
        cmdline = cmdline_mem;
    }
    if( cmdline != NULL ) {
#if defined( __DOS__ )
        /* 17-oct-88, check version #
         * 'append' program in DOS has a bug, so we avoid it by putting '.\'
         * on the front of a filename that doesn't have a path or drive
         */
        if( _RWD_osmajor >= 3 ) {
            if( drive[0] == NULLCHAR && dir[0] == NULLCHAR ) {
                dir = STRING( ".\\" );
            }
        }
#endif
        __F_NAME(_makepath,_wmakepath)( p, drive, dir, fname, ext );
        _RWD_errno = ENOENT;
        if( ext[0] != NULLCHAR ) {
#if defined( __OS2__ )
            if( stricmp( ext, STRING( ".cmd" ) ) == 0 || stricmp( ext, STRING( ".bat" ) ) == 0 ) {
#else
            if( __F_NAME(stricmp,wcscmp)( ext, STRING( ".bat" ) ) == 0 ) {
#endif
                retval = -1; /* assume file doesn't exist */
                if( file_exists( p ) ) {
#if defined( __DOS__ )
                    /* the environment will have to be reconstructed */
                    lib_free( envmem );
                    envmem = NULL;
#endif
                    __F_NAME(__ccmdline,__wccmdline)( p, argv, cmdline, 1 );
                    retval = __F_NAME(spawnl,_wspawnl)( mode,
                        __F_NAME(getenv,_wgetenv)( STRING( "COMSPEC" ) ),
                        use_cmd ? STRING( "CMD" ) : STRING( "COMMAND" ),
                        __F_NAME(__Slash_C,__wSlash_C)( switch_c, use_cmd ),
                        p, cmdline, NULL );
                }
            } else {
                _RWD_errno = 0;
                /* user specified an extension, so try it */
                retval = __F_NAME(_dospawn,_wdospawn)( mode, p, cmdline, ENVPARM, argv );
            }
#if defined( __OS2__ ) || defined( __NT__ )
        }
        /*
         * consider the following valid executable filenames:
         *      a.b.exe  a.cmd.exe  a.exe.cmd  a.cmd
         * we must always try to add .exe, etc.
         */
        if( _RWD_errno == ENOENT || _RWD_errno == EINVAL ) {
#else
        } else {
#endif
            end_of_p = p + __F_NAME(strlen,wcslen)( p );
            if( prot_mode286 ) {
                _RWD_errno = ENOENT;
            } else {
                __F_NAME(strcpy,wcscpy)( end_of_p, STRING( ".com" ) );
                _RWD_errno = 0;
                retval = __F_NAME(_dospawn,_wdospawn)( mode, p, cmdline, ENVPARM, argv );
            }
            if( _RWD_errno == ENOENT || _RWD_errno == EINVAL ) {
                _RWD_errno = 0;
                __F_NAME(strcpy,wcscpy)( end_of_p, STRING( ".exe" ) );
                retval = __F_NAME(_dospawn,_wdospawn)( mode, p, cmdline, ENVPARM, argv );
                if( _RWD_errno == ENOENT || _RWD_errno == EINVAL ) {
                    /* try for a .BAT file */
                    _RWD_errno = 0;
#if defined( __OS2__ )
                    strcpy( end_of_p, STRING( ".cmd" ) );
                    if( !file_exists( p ) )
                        strcpy( end_of_p, STRING( ".bat" ) );
#else
                    __F_NAME(strcpy,wcscpy)( end_of_p, STRING( ".bat" ) );
#endif
                    if( file_exists( p ) ) {
                        /* the environment will have to be reconstructed */
                        lib_free( envmem );
                        envmem = NULL;
                        __F_NAME(__ccmdline,__wccmdline)( p, argv, cmdline, 1 );
                        retval = __F_NAME(spawnl,_wspawnl)( mode,
                            __F_NAME(getenv,_wgetenv)( STRING( "COMSPEC" ) ),
                            use_cmd ? STRING( "CMD" ) : STRING( "COMMAND" ),
                            __F_NAME(__Slash_C,__wSlash_C)( switch_c, use_cmd ),
                            p, cmdline, NULL );
                    }
                }
            }
        }
    }
    _POSIX_HANDLE_CLEANUP;
    LIB_FREE( cmdline_mem );
    LIB_FREE( np );
    lib_free( envmem );
#if !defined(__OS2__) && defined( _M_I86 )
    if( _RWD_osmode != DOS_MODE ) {     /* if protect-mode e.g. DOS/16M */
        _dos_freemem( envseg );
    }
#endif
#if defined( __DOS__ )
    _RWD_Rest8087( &_87save );
#endif
    return( retval );
}
