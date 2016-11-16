#include <Common.h>
#include <cstdio>
#ifdef WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

bool hacerNoBloqueante(int sock) {
#if defined (MAC) || defined(LINUX)
    int nonBlocking = 1;
    if ( fcntl( sock, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 ) {
        printf( "failed to set non-blocking socket\n" );
        return false;
    }else{
        return true;
    }
#elif defined(WINDOWS)
    DWORD nonBlocking = 1;
    if ( ioctlsocket( sock, FIONBIO, &nonBlocking ) != 0 ) {
        printf( "failed to set non-blocking socket\n" );
        return false;
    }else{
        return true;
    }
#endif

}

void Wait( int milliseconds ) {
#ifdef WINDOWS
    Sleep( milliseconds  );
#else
    usleep( milliseconds * 1000.0f );
#endif
}
#ifdef WINDOWS
#include <SDL/SDL_timer.h>
#endif

unsigned int getTicks( ) {
#ifdef WINDOWS
    return SDL_GetTicks();
#else
    timeval seconds;
    gettimeofday( &seconds , NULL );
    unsigned int miliSeconds = 0;
    miliSeconds = (seconds.tv_sec*1000 + seconds.tv_usec/1000);
    return miliSeconds;
#endif
}
