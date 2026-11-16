#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <SDL3/SDL.h>

int NU_MacOS_EnableWindowCrossfade(SDL_Window* window)
{
    if (!window) return 0;

    SDL_PropertiesID properties = SDL_GetWindowProperties(window);

    NSWindow* nsWindow = (__bridge NSWindow*)SDL_GetPointerProperty(
        properties,
        SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,
        NULL
    );

    if (!nsWindow) return 0;

    NSView* view = nsWindow.contentView;

    if (!view) return 0;

    dispatch_async(dispatch_get_main_queue(), ^{
        view.wantsLayer = YES;
        view.layerContentsRedrawPolicy = NSViewLayerContentsRedrawCrossfade;
    });

    return 1;
}
