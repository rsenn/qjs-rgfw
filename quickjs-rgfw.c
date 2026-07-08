#include <quickjs.h>
#include <cutils.h>

#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL /* if this line is not added, OpenGL functions will not be included */

#include "RGFW/RGFW.h"

static JSClassID js_window_class_id;
static JSValue window_proto, window_ctor;

enum {
  FUNC_POLL_EVENTS = 0,
  FUNC_WAIT_FOR_EVENT,
};

static JSValue
js_rgfw_function(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  switch(magic) {
    case FUNC_POLL_EVENTS: {
      RGFW_pollEvents();
      break;
    }

    case FUNC_WAIT_FOR_EVENT: {
      int32_t ms = 0;

      if(argc > 0)
        JS_ToInt32(ctx, &ms, argv[0]);

      RGFW_waitForEvent(ms);
      break;
    }
  }

  return ret;
}

static JSValue
js_window_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSValue proto, obj = JS_UNDEFINED;
  RGFW_window* win = NULL;

  if(!(win = js_mallocz(ctx, sizeof(RGFW_window))))
    return JS_EXCEPTION;

  if(argc > 0) {
    const char* name = JS_ToCString(ctx, argv[0]);
    int32_t x = 0, y = 0, w = 640, h = 480;
    uint32_t flags = 0;

    if(argc > 1)
      JS_ToInt32(ctx, &x, argv[1]);
    if(argc > 2)
      JS_ToInt32(ctx, &y, argv[2]);
    if(argc > 3)
      JS_ToInt32(ctx, &w, argv[3]);
    if(argc > 4)
      JS_ToInt32(ctx, &h, argv[4]);

    if(argc > 5)
      JS_ToUint32(ctx, &flags, argv[5]);

    if(flags & RGFW_windowOpenGL) {
      /* nanovg needs a stencil buffer and its GL3 backend uses #version 150 shaders */
      RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
      hints->stencil = 8;
      hints->major = 3;
      hints->minor = 2;
      RGFW_setGlobalHints_OpenGL(hints);
    }

    if(!RGFW_createWindowPtr(name, x, y, w, h, flags, win)) {
      JS_FreeCString(ctx, name);
      JS_ThrowInternalError(ctx, "RGFW_createWindow failed");
      goto fail;
    }

    JS_FreeCString(ctx, name);
  } else {
    JS_ThrowInternalError(ctx, "RGFW_window");
    goto fail;
  }

  /* using new_target to get the prototype is necessary when the class is
   * extended. */
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if(JS_IsException(proto))
    goto fail;

  if(!JS_IsObject(proto))
    proto = window_proto;

  /* using new_target to get the prototype is necessary when the class is
   * extended. */
  obj = JS_NewObjectProtoClass(ctx, proto, js_window_class_id);
  JS_FreeValue(ctx, proto);

  if(JS_IsException(obj))
    goto fail;

  JS_SetOpaque(obj, win);
  return obj;

fail:
  js_free(ctx, win);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

enum {
  PROP_WIDTH = 0,
  PROP_HEIGHT,
  PROP_SHOULD_CLOSE,
};

static JSValue
js_window_get(JSContext* ctx, JSValueConst this_val, int magic) {
  RGFW_window* w;
  JSValue ret = JS_UNDEFINED;

  if(!(w = JS_GetOpaque2(ctx, this_val, js_window_class_id)))
    return JS_EXCEPTION;

  switch(magic) {
    case PROP_WIDTH: {
      int32_t width, height;
      RGFW_window_getSize(w, &width, &height);
      ret = JS_NewInt32(ctx, width);
      break;
    }

    case PROP_HEIGHT: {
      int32_t width, height;
      RGFW_window_getSize(w, &width, &height);
      ret = JS_NewInt32(ctx, height);
      break;
    }

    case PROP_SHOULD_CLOSE: {
      ret = JS_NewBool(ctx, RGFW_window_shouldClose(w));
      break;
    }
  }

  return ret;
}

static JSValue
js_window_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  RGFW_window* w;
  JSValue ret = JS_UNDEFINED;

  if(!(w = JS_GetOpaque2(ctx, this_val, js_window_class_id)))
    return JS_EXCEPTION;

  switch(magic) {
    case PROP_SHOULD_CLOSE: {
      RGFW_window_setShouldClose(w, JS_ToBool(ctx, value));
      break;
    }
  }

  return ret;
}

static JSValue
js_window_event_object(JSContext* ctx, const RGFW_event* ev) {
  JSValue obj = JS_NewObject(ctx);

  JS_SetPropertyStr(ctx, obj, "type", JS_NewInt32(ctx, ev->type));

  switch(ev->type) {
    case RGFW_keyPressed:
    case RGFW_keyReleased: {
      JS_SetPropertyStr(ctx, obj, "key", JS_NewInt32(ctx, ev->key.value));
      JS_SetPropertyStr(ctx, obj, "repeat", JS_NewBool(ctx, ev->key.repeat));
      JS_SetPropertyStr(ctx, obj, "mod", JS_NewInt32(ctx, ev->key.mod));
      break;
    }

    case RGFW_keyChar: {
      JS_SetPropertyStr(ctx, obj, "codepoint", JS_NewUint32(ctx, ev->keyChar.value));
      break;
    }

    case RGFW_mouseButtonPressed:
    case RGFW_mouseButtonReleased: {
      JS_SetPropertyStr(ctx, obj, "button", JS_NewInt32(ctx, ev->button.value));
      break;
    }

    case RGFW_mouseScroll: {
      JS_SetPropertyStr(ctx, obj, "x", JS_NewFloat64(ctx, ev->scroll.x));
      JS_SetPropertyStr(ctx, obj, "y", JS_NewFloat64(ctx, ev->scroll.y));
      break;
    }

    case RGFW_mousePosChanged: {
      JS_SetPropertyStr(ctx, obj, "x", JS_NewInt32(ctx, ev->mouse.x));
      JS_SetPropertyStr(ctx, obj, "y", JS_NewInt32(ctx, ev->mouse.y));
      JS_SetPropertyStr(ctx, obj, "vecX", JS_NewFloat64(ctx, ev->mouse.vecX));
      JS_SetPropertyStr(ctx, obj, "vecY", JS_NewFloat64(ctx, ev->mouse.vecY));
      break;
    }
  }

  return obj;
}

enum {
  METHOD_CLOSE = 0,
  METHOD_CHECK_EVENT,
  METHOD_SWAP_BUFFERS,
  METHOD_MAKE_CURRENT,
  METHOD_SWAP_INTERVAL,
};

static JSValue
js_window_method(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  RGFW_window* w;
  JSValue ret = JS_UNDEFINED;

  if(!(w = JS_GetOpaque2(ctx, this_val, js_window_class_id)))
    return JS_EXCEPTION;

  switch(magic) {
    case METHOD_CLOSE: {
      RGFW_window_closePtr(w);
      js_free(ctx, w);
      JS_SetOpaque(this_val, NULL);
      break;
    }

    case METHOD_CHECK_EVENT: {
      RGFW_event ev;

      if(RGFW_window_checkEvent(w, &ev))
        ret = js_window_event_object(ctx, &ev);
      else
        ret = JS_NULL;

      break;
    }

    case METHOD_SWAP_BUFFERS: {
      RGFW_window_swapBuffers_OpenGL(w);
      break;
    }

    case METHOD_MAKE_CURRENT: {
      RGFW_window_makeCurrentWindow_OpenGL(w);
      break;
    }

    case METHOD_SWAP_INTERVAL: {
      int32_t interval = 1;

      if(argc > 0)
        JS_ToInt32(ctx, &interval, argv[0]);

      RGFW_window_swapInterval_OpenGL(w, interval);
      break;
    }
  }

  return ret;
}

static void
js_window_finalizer(JSRuntime* rt, JSValue val) {
  RGFW_window* st;

  if((st = JS_GetOpaque(val, js_window_class_id))) {
    RGFW_window_closePtr(st);
    js_free_rt(rt, st);
  }
}

static JSClassDef js_window_class = {
    .class_name = "RGFW_window",
    .finalizer = js_window_finalizer,
};

static const JSCFunctionListEntry js_window_funcs[] = {
    JS_CGETSET_MAGIC_DEF("width", js_window_get, 0, PROP_WIDTH),
    JS_CGETSET_MAGIC_DEF("height", js_window_get, 0, PROP_HEIGHT),
    JS_CGETSET_MAGIC_DEF("shouldClose", js_window_get, js_window_set, PROP_SHOULD_CLOSE),
    JS_CFUNC_MAGIC_DEF("close", 0, js_window_method, METHOD_CLOSE),
    JS_CFUNC_MAGIC_DEF("checkEvent", 0, js_window_method, METHOD_CHECK_EVENT),
    JS_CFUNC_MAGIC_DEF("swapBuffers", 0, js_window_method, METHOD_SWAP_BUFFERS),
    JS_CFUNC_MAGIC_DEF("makeCurrent", 0, js_window_method, METHOD_MAKE_CURRENT),
    JS_CFUNC_MAGIC_DEF("swapInterval", 1, js_window_method, METHOD_SWAP_INTERVAL),
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "RGFW_window", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry js_rgfw_funcs[] = {
    JS_CFUNC_MAGIC_DEF("pollEvents", 0, js_rgfw_function, FUNC_POLL_EVENTS),
    JS_CFUNC_MAGIC_DEF("waitForEvent", 1, js_rgfw_function, FUNC_WAIT_FOR_EVENT),

    JS_PROP_INT32_DEF("TRUE", RGFW_TRUE, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("FALSE", RGFW_FALSE, JS_PROP_CONFIGURABLE),

    /* window flags */
    JS_PROP_INT32_DEF("windowNoBorder", RGFW_windowNoBorder, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowNoResize", RGFW_windowNoResize, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowFullscreen", RGFW_windowFullscreen, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowTransparent", RGFW_windowTransparent, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowCenter", RGFW_windowCenter, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowHide", RGFW_windowHide, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowMaximize", RGFW_windowMaximize, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowFloating", RGFW_windowFloating, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowOpenGL", RGFW_windowOpenGL, JS_PROP_CONFIGURABLE),

    /* event types */
    JS_PROP_INT32_DEF("eventNone", RGFW_eventNone, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("keyPressed", RGFW_keyPressed, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("keyReleased", RGFW_keyReleased, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("keyChar", RGFW_keyChar, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("mouseButtonPressed", RGFW_mouseButtonPressed, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("mouseButtonReleased", RGFW_mouseButtonReleased, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("mouseScroll", RGFW_mouseScroll, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("mousePosChanged", RGFW_mousePosChanged, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowMoved", RGFW_windowMoved, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowResized", RGFW_windowResized, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("focusIn", RGFW_focusIn, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("focusOut", RGFW_focusOut, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("mouseEnter", RGFW_mouseEnter, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("mouseLeave", RGFW_mouseLeave, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("windowRefresh", RGFW_windowRefresh, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("quit", RGFW_quit, JS_PROP_CONFIGURABLE),

    /* mouse buttons */
    JS_PROP_INT32_DEF("mouseLeft", RGFW_mouseLeft, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("mouseMiddle", RGFW_mouseMiddle, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("mouseRight", RGFW_mouseRight, JS_PROP_CONFIGURABLE),

    JS_PROP_INT32_DEF("keyEscape", RGFW_escape, JS_PROP_CONFIGURABLE),
};

int
js_rgfw_init(JSContext* ctx, JSModuleDef* m) {
  JS_NewClassID(&js_window_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_window_class_id, &js_window_class);

  window_ctor =
      JS_NewCFunction2(ctx, js_window_constructor, "RGFW_window", 1, JS_CFUNC_constructor, 0);
  window_proto = JS_NewObject(ctx);

  JS_SetPropertyFunctionList(ctx, window_proto, js_window_funcs, countof(js_window_funcs));

  JS_SetClassProto(ctx, js_window_class_id, window_proto);

  if(m) {
    JS_SetModuleExport(ctx, m, "Window", window_ctor);
    JS_SetModuleExportList(ctx, m, js_rgfw_funcs, countof(js_rgfw_funcs));
  }

  return 0;
}

void
js_init_module_rgfw(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Window");
  JS_AddModuleExportList(ctx, m, js_rgfw_funcs, countof(js_rgfw_funcs));
}

__attribute__((visibility("default"))) JSModuleDef*
js_init_module(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if((m = JS_NewCModule(ctx, module_name, js_rgfw_init))) {
    js_init_module_rgfw(ctx, m);
  }

  return m;
}
