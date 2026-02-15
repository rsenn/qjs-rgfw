#include <quickjs.h>
#include <cutils.h>

#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL /* if this line is not added, OpenGL functions will not be included */

#include "RGFW/RGFW.h"

static JSClassID js_window_class_id;
static JSValue window_proto, window_ctor;

static JSValue
js_rgfw_function(
    JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[], int magic) {
  JSValue ret = JS_UNDEFINED;

  switch(magic) {}

  return ret;
}

static JSValue
js_window_constructor(JSContext* ctx, JSValueConst new_target, int argc, JSValueConst argv[]) {
  JSValue proto, obj = JS_UNDEFINED;
  RGFW_window* w = NULL;

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

  JS_SetOpaque(obj, w);
  return obj;

fail:
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}

enum {
  PROP_SIZE = 0,
};

static JSValue
js_window_get(JSContext* ctx, JSValueConst this_val, int magic) {
  RGFW_window* w;
  JSValue ret = JS_UNDEFINED;

  if(!(w = JS_GetOpaque2(ctx, this_val, js_window_class_id)))
    return JS_EXCEPTION;

  switch(magic) {}

  return ret;
}

static JSValue
js_window_set(JSContext* ctx, JSValueConst this_val, JSValueConst value, int magic) {
  RGFW_window* w;
  JSValue ret = JS_UNDEFINED;

  if(!(w = JS_GetOpaque2(ctx, this_val, js_window_class_id)))
    return JS_EXCEPTION;

  switch(magic) {}

  return ret;
}

enum {
  METHOD_CLOSE = 0,
};

static JSValue
js_window_method(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst argv[], int magic) {
  RGFW_window* w;
  JSValue ret = JS_UNDEFINED;

  if(!(w = JS_GetOpaque2(ctx, this_val, js_window_class_id)))
    return JS_EXCEPTION;

  switch(magic) {}

  return ret;
}

static void
js_window_finalizer(JSRuntime* rt, JSValue val) {
  RGFW_window* st;

  if((st = JS_GetOpaque(val, js_window_class_id))) {
    js_free_rt(rt, st);
  }
}

static JSClassDef js_window_class = {
    .class_name = "RGFW_window",
    .finalizer = js_window_finalizer,
};

static const JSCFunctionListEntry js_window_funcs[] = {
    JS_PROP_STRING_DEF("[Symbol.toStringTag]", "RGFW_window", JS_PROP_CONFIGURABLE),
};

static const JSCFunctionListEntry js_rgfw_funcs[] = {
    JS_PROP_INT32_DEF("TRUE", RGFW_TRUE, JS_PROP_CONFIGURABLE),
    JS_PROP_INT32_DEF("FALSE", RGFW_FALSE, JS_PROP_CONFIGURABLE),

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
    JS_SetModuleExport(ctx, m, "Stream", window_ctor);
    JS_SetModuleExportList(ctx, m, js_rgfw_funcs, countof(js_rgfw_funcs));
  }

  return 0;
}

void
js_init_module_rgfw(JSContext* ctx, JSModuleDef* m) {
  JS_AddModuleExport(ctx, m, "Stream");
}

__attribute__((visibility("default"))) JSModuleDef*
js_init_module(JSContext* ctx, const char* module_name) {
  JSModuleDef* m;

  if((m = JS_NewCModule(ctx, module_name, js_rgfw_init))) {
    js_init_module_rgfw(ctx, m);
  }

  return m;
}
