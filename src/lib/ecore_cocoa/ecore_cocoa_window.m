#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore_Input.h>

#include <Ecore.h>
#include <Ecore_Cocoa.h>
#import "ecore_cocoa_window.h"
#import "ecore_cocoa_app.h"
#include "ecore_cocoa_private.h"

static NSCursor *_cursors[__ECORE_COCOA_CURSOR_LAST];


@implementation EcoreCocoaWindow

@synthesize ecore_window_data;

- (id) initWithContentRect: (NSRect) contentRect
                 styleMask: (unsigned int) aStyle
                   backing: (NSBackingStoreType) bufferingType
                     defer: (BOOL) flag
{
   if (![super initWithContentRect: contentRect
                         styleMask: aStyle
                           backing: bufferingType
                             defer: flag]) return nil;

   [self setBackgroundColor: [NSColor whiteColor]];
   [self makeKeyWindow];
   [self setDelegate:self];
   [self setAcceptsMouseMovedEvents:YES];

   [self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];

   _live_resize = 0;

   return self;
}

- (BOOL)isFullScreen
{
   return (([self styleMask] & NSWindowStyleMaskFullScreen) == NSWindowStyleMaskFullScreen);
}

- (BOOL)acceptsFirstResponder
{
   return YES;
}

- (BOOL)canBecomeKeyWindow
{
   return YES;
}

- (BOOL) requestResize: (NSSize) size
{
   Ecore_Cocoa_Event_Window_Resize_Request *event;

   event = malloc(sizeof(*event));
   if (EINA_UNLIKELY(event == NULL))
     {
        CRI("Failed to allocate Ecore_Cocoa_Event_Window_Resize_Request");
        return NO;
     }

   event->w = size.width;
   event->h = size.height -
      (([self isFullScreen] == YES) ? 0 : ecore_cocoa_titlebar_height_get());
   event->cocoa_window = self.ecore_window_data;
   ecore_event_add(ECORE_COCOA_EVENT_WINDOW_RESIZE_REQUEST, event, NULL, NULL);

   return YES;
}

- (void)windowWillClose:(NSNotification *) EINA_UNUSED notif
{
   Ecore_Cocoa_Event_Window_Destroy *event;

   event = malloc(sizeof(*event));
   if (EINA_UNLIKELY(event == NULL))
     {
        CRI("Failed to allocate Ecore_Cocoa_Event_Window");
        return;
     }
   event->cocoa_window = self.ecore_window_data;
   ecore_event_add(ECORE_COCOA_EVENT_WINDOW_DESTROY, event, NULL, NULL);
}

- (void)windowDidEnterFullScreen:(NSNotification *) EINA_UNUSED notif
{
   [self requestResize: self.frame.size];
}

- (void)windowDidExitFullScreen:(NSNotification *) EINA_UNUSED notif
{
   [self requestResize: self.frame.size];
}

- (void)windowDidResize:(NSNotification *) EINA_UNUSED notif
{
   /*
    * Only throw a resize event and manipulate the main loop when
    * we are 100% sure we are in a live resize, and the main loop
    * has already been paused!!
    */
   if (_live_resize > 0)
     {
        [self requestResize: self.frame.size];

        /*
         * During live resize, NSRunLoop blocks, and prevent the ecore_main_loop
         * to be run.
         * This, combined with the -pauseNSRunLoopMonitoring and
         * -resumeNSRunLoopMonitoring methods invoked in
         * -windowWillStartLiveResize and -windowDidEndLiveResize
         * allow the ecore_main_loop to run withing NSRunLoop during the
         * live resizing of a window.
         */
        ecore_main_loop_iterate();
     }
}

- (void)windowDidBecomeKey:(NSNotification *) EINA_UNUSED notif
{
   Ecore_Cocoa_Event_Window_Focused *e;

   e = malloc(sizeof(*e));
   if (EINA_UNLIKELY(e == NULL))
     {
        CRI("Failed to allocate Ecore_Cocoa_Event_Window");
        return;
     }
   e->cocoa_window = self.ecore_window_data;
   ecore_event_add(ECORE_COCOA_EVENT_WINDOW_FOCUSED, e, NULL, NULL);
}

- (void) windowWillStartLiveResize:(NSNotification *) EINA_UNUSED notification
{
   /* Live resize must be set AFTER pausing the main loop */
   [NSApp pauseNSRunLoopMonitoring];
   _live_resize++;
}

- (void) windowDidEndLiveResize:(NSNotification *) EINA_UNUSED notification
{
   /* Live resize must be clear BEFORE resuming the main loop */
   _live_resize--;
   [NSApp resumeNSRunLoopMonitoring];
}

- (void)windowDidResignKey:(NSNotification *) EINA_UNUSED notif
{
   Ecore_Cocoa_Event_Window_Unfocused *e;

   e = malloc(sizeof(*e));
   if (EINA_UNLIKELY(e == NULL))
     {
        CRI("Failed to allocate Ecore_Cocoa_Event_Window");
        return;
     }
   e->cocoa_window = self.ecore_window_data;
   ecore_event_add(ECORE_COCOA_EVENT_WINDOW_UNFOCUSED, e, NULL, NULL);
}

- (void) mouseDown:(NSEvent*) event
{
   unsigned int time = (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff);

   NSView *view = [self contentView];
   NSPoint event_location = [event locationInWindow];
   NSPoint pt = [view convertPoint:event_location fromView:nil];

   int h = [view frame].size.height;
   int x = pt.x;
   int y = h - pt.y;

   //we ignore left click in titlebar as it is handled by the OS (to move window)
   //and the corresponding mouseUp event isn't sent
   if (y <= 0 && [event buttonNumber] == 0) {
	 return;
   }

   Ecore_Event_Mouse_Button * ev = calloc(1, sizeof(Ecore_Event_Mouse_Button));

   if (EINA_UNLIKELY(!ev))
     {
        CRI("Failed to allocate Ecore_Event_Mouse_Button");
        return;
     }

   ev->x = x;
   ev->y = y;
   ev->root.x = ev->x;
   ev->root.y = ev->y;
   ev->timestamp = time;
   switch ([event buttonNumber])
     {
      case 0: ev->buttons = 1; break;
      case 1: ev->buttons = 3; break;
      case 2: ev->buttons = 2; break;
      default: ev->buttons = 0; break;
     }
   ev->window = (Ecore_Window)self.ecore_window_data;
   ev->event_window = ev->window;

   if ([event clickCount] == 2)
     ev->double_click = 1;
   else
     ev->double_click = 0;

   if ([event clickCount] >= 3)
     ev->triple_click = 1;
   else
     ev->triple_click = 0;

   ev->modifiers = ecore_cocoa_event_modifiers([event modifierFlags]);
   ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, ev, NULL, NULL);
}

- (void) rightMouseDown:(NSEvent*) event
{
   [self mouseDown: event];
}

- (void) otherMouseDown:(NSEvent*) event
{
   [self mouseDown: event];
}

- (void) mouseUp:(NSEvent*) event
{
   unsigned int time = (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff);

   NSView *view = [self contentView];
   NSPoint event_location = [event locationInWindow];
   NSPoint pt = [view convertPoint:event_location fromView:nil];

   int h = [view frame].size.height;
   int x = pt.x;
   int y = h - pt.y;

   Ecore_Event_Mouse_Button *ev = calloc(1, sizeof(*ev));
   if (EINA_UNLIKELY(!ev))
     {
        CRI("Failed to allocate Ecore_Event_Mouse_Button");
        return;
     }

   ev->x = x;
   ev->y = y;
   ev->root.x = ev->x;
   ev->root.y = ev->y;
   ev->timestamp = time;
   switch ([event buttonNumber])
     {
      case 0: ev->buttons = 1; break;
      case 1: ev->buttons = 3; break;
      case 2: ev->buttons = 2; break;
      default: ev->buttons = 0; break;
     }
   ev->window = (Ecore_Window)self.ecore_window_data;
   ev->event_window = ev->window;

   if ([event clickCount] == 2)
     ev->double_click = 1;
   else
     ev->double_click = 0;

   if ([event clickCount] >= 3)
     ev->triple_click = 1;
   else
     ev->triple_click = 0;

   ev->modifiers = ecore_cocoa_event_modifiers([event modifierFlags]);
   ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_UP, ev, NULL, NULL);
}

- (void) rightMouseUp:(NSEvent*) event
{
   [self mouseUp: event];
}

- (void) otherMouseUp:(NSEvent*) event
{
   [self mouseUp: event];
}

- (void) mouseMoved:(NSEvent*) event
{
   unsigned int time = (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff);
   Ecore_Event_Mouse_Move *ev = calloc(1, sizeof(*ev));
   if (EINA_UNLIKELY(!ev))
     {
        CRI("Failed to allocate Ecore_Event_Mouse_Move");
        return;
     }

   NSView *view = [self contentView];
   NSPoint event_location = [event locationInWindow];
   NSPoint pt = [view convertPoint:event_location fromView:nil];

   ev->x = pt.x;
   ev->y = [view frame].size.height - pt.y;
   ev->root.x = ev->x;
   ev->root.y = ev->y;
   ev->timestamp = time;
   ev->window = (Ecore_Window)self.ecore_window_data;
   ev->event_window = ev->window;
   ev->modifiers = 0; /* FIXME: keep modifier around. */

   ev->multi.device = 0;
   ev->multi.x = ev->x;
   ev->multi.y = ev->y;

   ev->modifiers = ecore_cocoa_event_modifiers([event modifierFlags]);
   ecore_event_add(ECORE_EVENT_MOUSE_MOVE, ev, NULL, NULL);
}

- (void) mouseDragged: (NSEvent*) event
{
   [self mouseMoved:event];
}

@end

EAPI Ecore_Cocoa_Window *
ecore_cocoa_window_new(int x,
                       int y,
                       int w,
                       int h)
{
   Ecore_Cocoa_Window *win;
   EcoreCocoaWindow *window;
   NSUInteger style =
      NSWindowStyleMaskTitled    |
      NSWindowStyleMaskClosable  |
      NSWindowStyleMaskResizable |
      NSWindowStyleMaskMiniaturizable;

   window = [[EcoreCocoaWindow alloc] initWithContentRect:NSMakeRect(x, y, w, h)
                                                styleMask:style
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
   if (EINA_UNLIKELY(!window))
     {
        CRI("Failed to create EcoreCocoaWindow");
        return NULL;
     }

   win = calloc(1, sizeof(*win));
   if (EINA_UNLIKELY(win == NULL))
     {
        CRI("Failed to allocate Ecore_Cocoa_Window");
        [window release];
        return NULL;
     }
   win->window = window;
   win->borderless = 0;

   window.ecore_window_data = win;

   return win;
}

EAPI void
ecore_cocoa_window_free(Ecore_Cocoa_Window *window)
{
   if (!window)
     return;

   [window->window release];
   free(window);
}

EAPI void
ecore_cocoa_window_size_min_set(Ecore_Cocoa_Window *window,
                                int                 w,
                                int                 h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   window->window.contentMinSize = NSMakeSize(w, h);
}

EAPI void
ecore_cocoa_window_size_min_get(const Ecore_Cocoa_Window *window,
                                int                      *w,
                                int                      *h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   const NSSize size = window->window.contentMinSize;
   if (w) *w = size.width;
   if (h) *h = size.height;
}

EAPI void
ecore_cocoa_window_size_max_set(Ecore_Cocoa_Window *window,
                                int                 w,
                                int                 h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   window->window.contentMaxSize = NSMakeSize(w, h);
}

EAPI void
ecore_cocoa_window_size_max_get(const Ecore_Cocoa_Window *window,
                                int                      *w,
                                int                      *h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   const NSSize size = window->window.contentMaxSize;
   if (w) *w = size.width;
   if (h) *h = size.height;
}

EAPI void
ecore_cocoa_window_size_step_set(Ecore_Cocoa_Window *window,
                                 int                 w,
                                 int                 h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   window->window.contentResizeIncrements = NSMakeSize(w, h);
}

EAPI void
ecore_cocoa_window_size_step_get(const Ecore_Cocoa_Window *window,
                                 int                      *w,
                                 int                      *h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   const NSSize size = window->window.contentResizeIncrements;
   if (w) *w = size.width;
   if (h) *h = size.height;
}

EAPI void
ecore_cocoa_window_move(Ecore_Cocoa_Window *window,
                        int                 x,
                        int                 y)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   NSRect win_frame;

   win_frame = [window->window frame];
   win_frame.origin.x = x;
   win_frame.origin.y = y;

   [window->window setFrame:win_frame display:YES];
}

EAPI void
ecore_cocoa_window_resize(Ecore_Cocoa_Window *window,
                          int                 w,
                          int                 h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   NSRect win_frame;
   EcoreCocoaWindow *const win = window->window;

   win_frame = [win frame];
   win_frame.size.height = h +
      (([win isFullScreen] == YES) ? 0 : ecore_cocoa_titlebar_height_get());
   win_frame.size.width = w;

   [win setFrame:win_frame display:YES];
}

EAPI void
ecore_cocoa_window_geometry_get(const Ecore_Cocoa_Window *window,
                                int                      *x,
                                int                      *y,
                                int                      *w,
                                int                      *h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   const NSRect frame = window->window.frame;
   if (x) *x = frame.origin.x;
   if (y) *y = frame.origin.y;
   if (w) *w = frame.size.width;
   if (h) *h = frame.size.height;
}

EAPI void
ecore_cocoa_window_size_get(const Ecore_Cocoa_Window *window,
                            int                      *w,
                            int                      *h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   const NSSize size = window->window.frame.size;
   if (w) *w = size.width;
   if (h) *h = size.height;
}

EAPI void
ecore_cocoa_window_move_resize(Ecore_Cocoa_Window *window,
                               int                 x,
                               int                 y,
                               int                 w,
                               int                 h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   NSRect win_frame;

   win_frame = [window->window frame];
   win_frame.size.height = h +
      (([window->window isFullScreen] == YES) ? 0 : ecore_cocoa_titlebar_height_get());
   win_frame.size.width = w;
   win_frame.origin.x = x;
   win_frame.origin.y = y;

   [window->window setFrame:win_frame display:YES];
}

EAPI void
ecore_cocoa_window_title_set(Ecore_Cocoa_Window *window, const char *title)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   EINA_SAFETY_ON_NULL_RETURN(title);

   [window->window setTitle:[NSString stringWithUTF8String:title]];
}

EAPI void
ecore_cocoa_window_show(Ecore_Cocoa_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if (![window->window isVisible])
     [window->window makeKeyAndOrderFront:NSApp];
   [window->window display];
}

EAPI void
ecore_cocoa_window_hide(Ecore_Cocoa_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if ([window->window isVisible])
     [window->window orderOut:NSApp];
}

EAPI void
ecore_cocoa_window_raise(Ecore_Cocoa_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   [window->window orderFront:nil];
}

EAPI void
ecore_cocoa_window_lower(Ecore_Cocoa_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   [window->window orderBack:nil];
}

EAPI void
ecore_cocoa_window_activate(Ecore_Cocoa_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   [window->window makeKeyAndOrderFront:nil];
}

EAPI void
ecore_cocoa_window_iconified_set(Ecore_Cocoa_Window *window,
                                 Eina_Bool           on)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if (on)
     {
        [window->window miniaturize:nil];
     }
   else
     {
        [window->window deminiaturize:nil];
     }
}

EAPI void
ecore_cocoa_window_borderless_set(Ecore_Cocoa_Window *window,
                                  Eina_Bool           on)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if (on)
     [window->window setContentBorderThickness:0.0
                                       forEdge:NSMinXEdge | NSMinYEdge | NSMaxXEdge | NSMaxYEdge];
   else
     {
        // TODO
     }
}

EAPI void
ecore_cocoa_window_view_set(Ecore_Cocoa_Window *window,
                            Ecore_Cocoa_Object *view)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   EINA_SAFETY_ON_NULL_RETURN(view);

   //[[window->window contentView] addSubview:view];
   NSView *v = view;
   [window->window setContentView:view];

   NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:[v frame]
                                                       options:NSTrackingMouseMoved |
                                                       NSTrackingActiveInActiveApp |
                                                       NSTrackingInVisibleRect
                                                       owner:v
                                                    userInfo:nil];
   [v addTrackingArea:area];

   [area release];
}

EAPI Ecore_Cocoa_Object *
ecore_cocoa_window_get(const Ecore_Cocoa_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);
   return window->window;
}

EAPI void
ecore_cocoa_window_cursor_set(Ecore_Cocoa_Window *win,
                              Ecore_Cocoa_Cursor  c)
{
   EINA_SAFETY_ON_NULL_RETURN(win);
   EINA_SAFETY_ON_FALSE_RETURN((c >= 0) && (c <= __ECORE_COCOA_CURSOR_LAST));

   NSCursor *cursor = _cursors[c];

   DBG("Setting cursor %i (%s)", c, [[cursor description] UTF8String]);
   [cursor set];
}

EAPI void
ecore_cocoa_window_cursor_show(Ecore_Cocoa_Window *win,
                               Eina_Bool           show)
{
   EINA_SAFETY_ON_NULL_RETURN(win);

   if (show) [NSCursor unhide];
   else [NSCursor hide];
}

Eina_Bool
_ecore_cocoa_window_init(void)
{
   _cursors[ECORE_COCOA_CURSOR_ARROW]                 = [NSCursor arrowCursor];
   _cursors[ECORE_COCOA_CURSOR_CONTEXTUAL_MENU]       = [NSCursor contextualMenuCursor];
   _cursors[ECORE_COCOA_CURSOR_CLOSED_HAND]           = [NSCursor closedHandCursor];
   _cursors[ECORE_COCOA_CURSOR_CROSSHAIR]             = [NSCursor crosshairCursor];
   _cursors[ECORE_COCOA_CURSOR_DISAPPEARING_ITEM]     = [NSCursor disappearingItemCursor];
   _cursors[ECORE_COCOA_CURSOR_DRAG_COPY]             = [NSCursor dragCopyCursor];
   _cursors[ECORE_COCOA_CURSOR_DRAG_LINK]             = [NSCursor dragLinkCursor];
   _cursors[ECORE_COCOA_CURSOR_IBEAM]                 = [NSCursor IBeamCursor];
   _cursors[ECORE_COCOA_CURSOR_OPEN_HAND]             = [NSCursor openHandCursor];
   _cursors[ECORE_COCOA_CURSOR_OPERATION_NOT_ALLOWED] = [NSCursor operationNotAllowedCursor];
   _cursors[ECORE_COCOA_CURSOR_POINTING_HAND]         = [NSCursor pointingHandCursor];
   _cursors[ECORE_COCOA_CURSOR_RESIZE_DOWN]           = [NSCursor resizeDownCursor];
   _cursors[ECORE_COCOA_CURSOR_RESIZE_LEFT]           = [NSCursor resizeLeftCursor];
   _cursors[ECORE_COCOA_CURSOR_RESIZE_LEFT_RIGHT]     = [NSCursor resizeLeftRightCursor];
   _cursors[ECORE_COCOA_CURSOR_RESIZE_RIGHT]          = [NSCursor resizeRightCursor];
   _cursors[ECORE_COCOA_CURSOR_RESIZE_UP]             = [NSCursor resizeUpCursor];
   _cursors[ECORE_COCOA_CURSOR_RESIZE_UP_DOWN]        = [NSCursor resizeUpDownCursor];
   _cursors[ECORE_COCOA_CURSOR_IBEAM_VERTICAL]        = [NSCursor IBeamCursorForVerticalLayout];

   return EINA_TRUE;
}

