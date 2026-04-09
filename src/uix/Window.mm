// Window.mm
// macOS NSWindow + CAMetalLayer backend for uix::Window
//
// Sam Gateau / pico - 2024
// MIT License
#include "Window.h"
#include "Imgui.h"

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#include <map>
#include <string>

using namespace uix;

// Forward-declare so ObjC @interface blocks can reference it
class MacWindowBackend;

// ---------------------------------------------------------------------------
// PicoView: NSView subclass that hosts a CAMetalLayer and forwards events
// ---------------------------------------------------------------------------
@interface PicoView : NSView {
    MacWindowBackend* _backend;
    CAMetalLayer* _metalLayer;
}
- (instancetype)initWithFrame:(NSRect)frame backend:(MacWindowBackend*)backend;
- (CAMetalLayer*)metalLayer;
@end

// ---------------------------------------------------------------------------
// PicoWindowDelegate: NSWindowDelegate to handle close / resize
// ---------------------------------------------------------------------------
@interface PicoWindowDelegate : NSObject<NSWindowDelegate> {
    MacWindowBackend* _backend;
}
- (instancetype)initWithBackend:(MacWindowBackend*)backend;
@end

// ---------------------------------------------------------------------------
// MacWindowBackend implementation
// ---------------------------------------------------------------------------
class MacWindowBackend : public WindowBackend {
public:
    NSWindow*          _nsWindow   { nullptr };
    PicoView*          _picoView   { nullptr };
    PicoWindowDelegate* _delegate  { nullptr };
    CAMetalLayer*      _metalLayer { nullptr };

    uint32_t _width  { 1280 };
    uint32_t _height {  720 };

    MacWindowBackend(Window* owner) : WindowBackend(owner) {}
    virtual ~MacWindowBackend() {
        if (_nsWindow) {
            [_nsWindow close];
            _nsWindow = nil;
        }
    }

    static MacWindowBackend* create(Window* owner);

    void init() {
        if (!NSApp) {
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            [NSApp finishLaunching];
        }

        NSRect frame = NSMakeRect(100, 100, _width, _height);
        NSWindowStyleMask style = NSWindowStyleMaskTitled
                                | NSWindowStyleMaskClosable
                                | NSWindowStyleMaskResizable
                                | NSWindowStyleMaskMiniaturizable;

        _nsWindow = [[NSWindow alloc] initWithContentRect:frame
                                               styleMask:style
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];

        _picoView = [[PicoView alloc] initWithFrame:frame backend:this];
        _metalLayer = [_picoView metalLayer];

        [_nsWindow setContentView:_picoView];
        [_nsWindow makeFirstResponder:_picoView];

        _delegate = [[PicoWindowDelegate alloc] initWithBackend:this];
        [_nsWindow setDelegate:_delegate];

        [_nsWindow makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];

        // Update actual dimensions from the backing store
        NSRect backing = [_picoView convertRectToBacking:[_picoView bounds]];
        _width  = (uint32_t)backing.size.width;
        _height = (uint32_t)backing.size.height;

        // Fire initial resize so the app knows the window size
        ResizeEvent e { _width, _height, true };
        _ownerWindow->onResize(e);
    }

    bool messagePump() override {
        @autoreleasepool {
            while (true) {
                NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                    untilDate:[NSDate distantPast]
                                                       inMode:NSDefaultRunLoopMode
                                                      dequeue:YES];
                if (!event) break;
                Imgui::customEventCallback((__bridge void*)event);
                [NSApp sendEvent:event];
                [NSApp updateWindows];
            }
        }
        // Drive continuous rendering by triggering onPaint every pump
        PaintEvent pe{};
        _ownerWindow->onPaint(pe);
        return (_nsWindow != nil) && ![_nsWindow isZoomed] && _running;
    }

    void* nativeWindow() override {
        return (__bridge void*)_picoView;
    }

    uint32_t width()  const override { return _width; }
    uint32_t height() const override { return _height; }
    uint32_t chromedWidth()  const override { return _width; }
    uint32_t chromedHeight() const override { return _height; }

    void setTitle(const std::string& title) override {
        if (_nsWindow) {
            [_nsWindow setTitle:[NSString stringWithUTF8String:title.c_str()]];
        }
    }

    void onWindowResize() {
        NSRect backing = [_picoView convertRectToBacking:[_picoView bounds]];
        _width  = (uint32_t)backing.size.width;
        _height = (uint32_t)backing.size.height;
        [_metalLayer setDrawableSize:CGSizeMake(_width, _height)];
        ResizeEvent e { _width, _height, true };
        _ownerWindow->onResize(e);
    }

    void onWindowClose() {
        _running = false;
        _nsWindow = nil;
    }

    bool _running { true };

    // Key event translation
    static Key translateKeyCode(unsigned short keyCode) {
        switch (keyCode) {
        case 0x00: return KEY_A;
        case 0x01: return KEY_S;
        case 0x02: return KEY_D;
        case 0x03: return KEY_F;
        case 0x04: return KEY_H;
        case 0x05: return KEY_G;
        case 0x06: return KEY_Z;
        case 0x07: return KEY_X;
        case 0x08: return KEY_C;
        case 0x09: return KEY_V;
        case 0x0B: return KEY_B;
        case 0x0C: return KEY_Q;
        case 0x0D: return KEY_W;
        case 0x0E: return KEY_E;
        case 0x0F: return KEY_R;
        case 0x10: return KEY_Y;
        case 0x11: return KEY_T;
        case 0x12: return KEY_1;
        case 0x13: return KEY_2;
        case 0x14: return KEY_3;
        case 0x15: return KEY_4;
        case 0x16: return KEY_6;
        case 0x17: return KEY_5;
        case 0x18: return KEY_ADD;
        case 0x19: return KEY_9;
        case 0x1A: return KEY_7;
        case 0x1B: return KEY_SUBSTRACT;
        case 0x1C: return KEY_8;
        case 0x1D: return KEY_0;
        case 0x1F: return KEY_O;
        case 0x20: return KEY_U;
        case 0x21: return KEY_NOPE;
        case 0x22: return KEY_I;
        case 0x23: return KEY_P;
        case 0x24: return KEY_RETURN;
        case 0x25: return KEY_L;
        case 0x26: return KEY_J;
        case 0x28: return KEY_K;
        case 0x29: return KEY_NOPE;
        case 0x2A: return KEY_NOPE;
        case 0x2B: return KEY_SEPARATOR;
        case 0x2C: return KEY_NOPE;
        case 0x2D: return KEY_N;
        case 0x2E: return KEY_M;
        case 0x31: return KEY_SPACE;
        case 0x33: return KEY_BACK;
        case 0x35: return KEY_ESC;
        case 0x36: return KEY_RCONTROL;
        case 0x37: return KEY_NOPE;  // Command
        case 0x38: return KEY_LSHIFT;
        case 0x39: return KEY_CAPSLOCK;
        case 0x3A: return KEY_LALT;
        case 0x3B: return KEY_LCONTROL;
        case 0x3C: return KEY_RSHIFT;
        case 0x3D: return KEY_RALT;
        case 0x3E: return KEY_RCONTROL;
        case 0x60: return KEY_F5;
        case 0x61: return KEY_F6;
        case 0x62: return KEY_F7;
        case 0x63: return KEY_F3;
        case 0x64: return KEY_F8;
        case 0x65: return KEY_F9;
        case 0x67: return KEY_F11;
        case 0x69: return KEY_F13;
        case 0x6B: return KEY_F14;
        case 0x6D: return KEY_F10;
        case 0x6F: return KEY_F12;
        case 0x71: return KEY_F15;
        case 0x73: return KEY_HOME;
        case 0x74: return KEY_PAGEUP;
        case 0x75: return KEY_DELETE;
        case 0x77: return KEY_END;
        case 0x79: return KEY_PAGEDOWN;
        case 0x7B: return KEY_LEFT;
        case 0x7C: return KEY_RIGHT;
        case 0x7D: return KEY_DOWN;
        case 0x7E: return KEY_UP;
        default:   return KEY_NOPE;
        }
    }

    void dispatchKeyEvent(NSEvent* event, bool down) {
        Key k = translateKeyCode([event keyCode]);
        KeyboardEvent ke { k, down };
        _ownerWindow->onKeyboard(ke);
    }

    void dispatchMouseEvent(NSEvent* event, uint8_t stateFlags) {
        NSPoint pt = [event locationInWindow];
        NSRect bounds = [_picoView bounds];
        // Flip Y (AppKit is bottom-left origin, we want top-left)
        float x = (float)pt.x;
        float y = (float)(bounds.size.height - pt.y);
        core::vec2 pos { x, y };

        float wheel = 0.0f;
        uint8_t state = stateFlags;

        if ([event type] == NSEventTypeScrollWheel) {
            wheel = (float)[event scrollingDeltaY];
            if ([event hasPreciseScrollingDeltas]) wheel *= 0.05f;
            state |= MouseState::MOUSE_WHEEL;
        }

        static core::vec2 lastPos { 0.f, 0.f };
        core::vec2 delta { 0.f, 0.f };
        if (stateFlags & MouseState::MOUSE_MOVE) {
            delta = pos - lastPos;
        }
        lastPos = pos;

        MouseEvent me { pos, delta, wheel, state };
        _ownerWindow->onMouse(me);
    }
};

// ---------------------------------------------------------------------------
// PicoView implementation
// ---------------------------------------------------------------------------
@implementation PicoView

- (instancetype)initWithFrame:(NSRect)frame backend:(MacWindowBackend*)backend {
    self = [super initWithFrame:frame];
    if (self) {
        _backend = backend;

        self.wantsLayer = YES;
        _metalLayer = [CAMetalLayer layer];
        _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
        _metalLayer.framebufferOnly = YES;

        NSRect backing = [self convertRectToBacking:frame];
        _metalLayer.drawableSize = CGSizeMake(backing.size.width, backing.size.height);
        _metalLayer.contentsScale = self.window ? self.window.backingScaleFactor : [[NSScreen mainScreen] backingScaleFactor];

        self.layer = _metalLayer;
    }
    return self;
}

- (CAMetalLayer*)metalLayer { return _metalLayer; }

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)wantsUpdateLayer { return YES; }

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if (self.window) {
        _metalLayer.contentsScale = self.window.backingScaleFactor;
        NSRect backing = [self convertRectToBacking:self.bounds];
        _metalLayer.drawableSize = CGSizeMake(backing.size.width, backing.size.height);
    }
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    NSRect backing = [self convertRectToBacking:self.bounds];
    _metalLayer.drawableSize = CGSizeMake(backing.size.width, backing.size.height);
    if (_backend) _backend->onWindowResize();
}

// --- Mouse ---
- (void)mouseDown:(NSEvent*)event {
    _backend->dispatchMouseEvent(event, MouseState::MOUSE_LBUTTON);
}
- (void)mouseUp:(NSEvent*)event {
    _backend->dispatchMouseEvent(event, 0);
}
- (void)mouseDragged:(NSEvent*)event {
    _backend->dispatchMouseEvent(event, MouseState::MOUSE_LBUTTON | MouseState::MOUSE_MOVE);
}
- (void)rightMouseDown:(NSEvent*)event {
    _backend->dispatchMouseEvent(event, MouseState::MOUSE_RBUTTON);
}
- (void)rightMouseUp:(NSEvent*)event {
    _backend->dispatchMouseEvent(event, 0);
}
- (void)rightMouseDragged:(NSEvent*)event {
    _backend->dispatchMouseEvent(event, MouseState::MOUSE_RBUTTON | MouseState::MOUSE_MOVE);
}
- (void)mouseMoved:(NSEvent*)event {
    _backend->dispatchMouseEvent(event, MouseState::MOUSE_MOVE);
}
- (void)scrollWheel:(NSEvent*)event {
    _backend->dispatchMouseEvent(event, 0);
}

// --- Keyboard ---
- (void)keyDown:(NSEvent*)event {
    _backend->dispatchKeyEvent(event, true);
}
- (void)keyUp:(NSEvent*)event {
    _backend->dispatchKeyEvent(event, false);
}

@end

// ---------------------------------------------------------------------------
// PicoWindowDelegate implementation
// ---------------------------------------------------------------------------
MacWindowBackend* MacWindowBackend::create(Window* owner) {
    auto* backend = new MacWindowBackend(owner);
    backend->init();
    return backend;
}

@implementation PicoWindowDelegate

- (instancetype)initWithBackend:(MacWindowBackend*)backend {
    self = [super init];
    if (self) _backend = backend;
    return self;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    _backend->onWindowClose();
    return YES;
}

- (void)windowDidResize:(NSNotification*)notification {
    _backend->onWindowResize();
}

@end

#endif // __APPLE__
