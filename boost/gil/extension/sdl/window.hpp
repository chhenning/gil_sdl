#ifndef BOOST_GIL_SDL_WINDOWS_HPP
#define BOOST_GIL_SDL_WINDOWS_HPP

#include <SDL.h>

#include <boost/cstdint.hpp>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/gil/gil_all.hpp>

#include <boost/thread.hpp>
#include <boost/thread/once.hpp>

#include "threadsafe_queue.hpp"

namespace boost { namespace gil { namespace sdl {

struct initializer
{
    initializer()
    {
        static boost::once_flag flag = BOOST_ONCE_INIT;
        boost::call_once( flag, [] () { SDL_Init( SDL_INIT_EVERYTHING ); } );
    }

    ~initializer()
    {
        static boost::once_flag flag = BOOST_ONCE_INIT;
        boost::call_once( flag, [] () { SDL_Quit(); } );
    }
};

struct sdl_error : std::exception
{
    const char* what() const throw()
    {
        return "SDL subsystem error.";
    }
};

class window
{
public:

    // Constructor
    window( const char*           title = NULL
          , const int             window_pos_x = SDL_WINDOWPOS_CENTERED
          , const int             window_pos_y = SDL_WINDOWPOS_CENTERED
          , const int             window_width = 640
          , const int             window_height = 480
          , const boost::uint32_t window_flags = SDL_WINDOW_SHOWN
          , const int             renderer_index = -1
          , const boost::uint32_t renderer_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
          )
    : _error( false )
    {
        // create window
        _window = window_ptr_t( SDL_CreateWindow( title
                                                , window_pos_x
                                                , window_pos_y
                                                , window_width
                                                , window_height
                                                , window_flags
                                                )

                              , SDL_DestroyWindow
                              );

        if( _window == NULL )
        {
            _error = true;
            return;
        }

        // create renderer
        _renderer = renderer_ptr_t( SDL_CreateRenderer( _window.get()
                                                      , renderer_index
                                                      , renderer_flags
                                                      )
                                  , SDL_DestroyRenderer
                                  );


        if( _renderer == NULL )
        {
            _error = true;
            return;
        }

        // create surface
        _surface = surface_ptr_t( SDL_CreateRGBSurface( 0 // obsolete
                                                      , window_width
                                                      , window_height
                                                      , 32
                                                      , 0, 0, 0, 0
                                                      )
                                , SDL_FreeSurface
                                );

        if( _surface == NULL )
        {
            _error = true;
            return;
        }

        // create event loop
        _thread = boost::make_shared< thread_t >( &window::_run, this );

    }

    // Destructor
    ~window()
    {
      _thread->join();
    }


    void draw()
    {
        if( _error )
        {
            throw sdl_error();
        }

        texture_ptr_t texture = texture_ptr_t( SDL_CreateTextureFromSurface( _renderer.get()
                                                                           , _surface.get()
                                                                           )
                                             , SDL_DestroyTexture
                                             );

        if( texture == NULL )
        {
            throw sdl_error();
        }
        
        SDL_RenderCopy( _renderer.get()
                      , texture.get()
                      , NULL
                      , NULL
                      );

        SDL_RenderPresent( _renderer.get() );

        boost::this_thread::sleep( boost::posix_time::milliseconds( 2000 ));
    }

    boost::gil::bgra8_view_t wrap_sdl_image()
    {
       return interleaved_view( _surface->w
                              , _surface->h
                              , (boost::gil::bgra8_pixel_t*) _surface->pixels
                              , _surface->pitch   );
    }

private:

    void _run()
    {
    }

private:

    typedef SDL_Window window_t;
    typedef boost::shared_ptr< window_t > window_ptr_t;

    typedef SDL_Renderer renderer_t;
    typedef boost::shared_ptr< renderer_t > renderer_ptr_t;

    typedef SDL_Surface surface_t;
    typedef boost::shared_ptr< surface_t > surface_ptr_t;

    typedef SDL_Texture texture_t;
    typedef boost::shared_ptr< texture_t > texture_ptr_t;

    typedef boost::thread thread_t;
    typedef boost::shared_ptr< thread_t > thread_ptr_t;

    typedef threadsafe_queue< int > queue_t;
    typedef boost::shared_ptr< queue_t > queue_ptr_t;

    window_ptr_t   _window;
    renderer_ptr_t _renderer;
    surface_ptr_t  _surface;

    thread_ptr_t _thread;

    //queue_ptr_t _queue;

    bool _error;
};

} } } // namespace boost::gil::sdl

#endif // BOOST_GIL_SDL_WINDOWS_HPP