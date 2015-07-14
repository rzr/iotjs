/* Copyright 2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "iotjs_def.h"
#include "iotjs_module_process.h"
#include "iotjs_js.h"

#include <cstdlib>
#include <string.h>


#if defined(__LINUX__)
 #define PLATFORM "linux"
#elif defined(__NUTTX__)
 #define PLATFORM "nuttx"
#elif defined(__DARWIN__)
 #define PLATFORM "darwin"
#else
 #error Cannot identify PLATFORM
#endif

#if defined(__ARM__)
 #define ARCHITECTURE "arm"
#elif defined(__i686__)
 #define ARCHITECTURE "ia32"
#elif defined(__x86_64__)
 #define ARCHITECTURE "x64"
#else
 #error Cannot identify ARCHITECTURE
#endif


namespace iotjs {


static JObject* GetProcess() {
  Module* module = GetBuiltinModule(MODULE_PROCESS);
  IOTJS_ASSERT(module != NULL);

  JObject* process = module->module;
  IOTJS_ASSERT(process != NULL);
  IOTJS_ASSERT(process->IsObject());

  return process;
}


void UncaughtException(JObject& jexception) {
  JObject* process = GetProcess();

  JObject jonuncaughtexception(
                      process->GetProperty(JSCT("_onUncaughtExcecption")));
  IOTJS_ASSERT(jonuncaughtexception.IsFunction());

  JArgList args(1);
  args.Add(jexception);

  JResult jres = jonuncaughtexception.Call(*process, args);
  IOTJS_ASSERT(jres.IsOk());
}


void ProcessEmitExit(int code) {
  JObject* process = GetProcess();

  JObject jexit(process->GetProperty(JSCT("emitExit")));
  IOTJS_ASSERT(jexit.IsFunction());

  JArgList args(1);
  args.Add(JVal::Number(code));

  JResult jres = jexit.Call(JObject::Null(), args);
  if (!jres.IsOk()) {
    exit(2);
  }
}


// Calls next tick callbacks registered via `process.nextTick()`.
bool ProcessNextTick() {
  JObject* process = GetProcess();

  JObject jon_next_tick(process->GetProperty(JSCT("_onNextTick")));
  IOTJS_ASSERT(jon_next_tick.IsFunction());

  JResult jres = jon_next_tick.Call(JObject::Null(), JArgList::Empty());
  IOTJS_ASSERT(jres.IsOk());
  IOTJS_ASSERT(jres.value().IsBoolean());

  return jres.value().GetBoolean();
}


// Make a callback for the given `function` with `this_` binding and `args`
// arguments. The next tick callbacks registered via `process.nextTick()`
// will be called after the callback function `function` returns.
JObject MakeCallback(JObject& function, JObject& this_, JArgList& args) {
  // Calls back the function.
  JResult jres = function.Call(this_, args);
  if (jres.IsException()) {
    UncaughtException(jres.value());
  }

  // Calls the next tick callbacks.
  ProcessNextTick();

  // Return value.
  return jres.value();
}


JHANDLER_FUNCTION(Binding, handler) {
  IOTJS_ASSERT(handler.GetArgLength() == 1);
  IOTJS_ASSERT(handler.GetArg(0)->IsNumber());

  int module_kind = handler.GetArg(0)->GetInt32();

  Module* module = GetBuiltinModule(static_cast<ModuleKind>(module_kind));
  IOTJS_ASSERT(module != NULL);

  if (module->module == NULL) {
    IOTJS_ASSERT(module->fn_register != NULL);
    module->module = module->fn_register();
    IOTJS_ASSERT(module->module);
  }

  handler.Return(*module->module);

  return true;
}


static JResult WrapEval(jschar* source) {
  static const jschar* wrapper[2] = {
      JSCT("(function (a, b, c) { function wwwwrap(exports,require, module) {"),
      JSCT(" }; wwwwrap(a, b, c); });") };

  int len1 = jstrlen(wrapper[0]);
  int len2 = jstrlen(source);
  int len3 = jstrlen(wrapper[1]);

  LocalString code(len1 + len2 + len3 + 1);

  jstrcpy(code, wrapper[0]);
  jstrcat(code + len1, source);
  jstrcat(code + len1 + len2, wrapper[1]);

  return JObject::Eval(code);
}


JHANDLER_FUNCTION(Compile, handler){
  IOTJS_ASSERT(handler.GetArgLength() == 1);
  IOTJS_ASSERT(handler.GetArg(0)->IsString());

  LocalString source(handler.GetArg(0)->GetByteString());

  JResult jres = WrapEval(source);

  if (jres.IsOk()) {
    handler.Return(jres.value());
  } else {
    handler.Throw(jres.value());
  }

  return !handler.HasThrown();
}


JHANDLER_FUNCTION(CompileNativePtr, handler){
  IOTJS_ASSERT(handler.GetArgLength() == 1);
  IOTJS_ASSERT(handler.GetArg(0)->IsObject());

  jschar* source = (jschar*)(handler.GetArg(0)->GetNative());

  JResult jres = WrapEval(source);

  if (jres.IsOk()) {
    handler.Return(jres.value());
  } else {
    handler.Throw(jres.value());
  }

  return !handler.HasThrown();
}


JHANDLER_FUNCTION(ReadSource, handler){
  IOTJS_ASSERT(handler.GetArgLength() == 1);
  IOTJS_ASSERT(handler.GetArg(0)->IsString());

  LocalString path(handler.GetArg(0)->GetByteString());
  LocalString code(ReadFile(path));

  JObject ret(code);
  handler.Return(ret);

  return true;
}


JHANDLER_FUNCTION(Cwd, handler){
  IOTJS_ASSERT(handler.GetArgLength() == 0);

  char path[IOTJS_MAX_PATH_SIZE];
  size_t size_path = sizeof(path);
  int err = uv_cwd(path, &size_path);
  if (err) {
    JHANDLER_THROW_RETURN(handler, Error, JSCT("cwd error"));
  }
  JObject ret(JSCT(path));
  handler.Return(ret);

  return true;
}


JHANDLER_FUNCTION(DoExit, handler) {
  IOTJS_ASSERT(handler.GetArgLength() == 1);
  IOTJS_ASSERT(handler.GetArg(0)->IsNumber());

  int exit_code = handler.GetArg(0)->GetInt32();

  exit(exit_code);
}


void SetNativeSources(JObject* native_sources) {
  for (int i = 0; natives[i].name; i++) {
    JObject native_source;
    native_source.SetNative((uintptr_t)(natives[i].source), NULL);
    native_sources->SetProperty(natives[i].name, native_source);
  }
}


void SetProcessEnv(JObject* process){
  const char *homedir;
  homedir = getenv("HOME");
  if (homedir == NULL) {
    homedir = "";
  }
  JObject home(JSCT(homedir));
  JObject env;
  env.SetProperty(JSCT("HOME"), home);
  process->SetProperty(JSCT("env"), env);
}


JObject* InitProcess() {
  Module* module = GetBuiltinModule(MODULE_PROCESS);
  JObject* process = module->module;

  if (process == NULL) {
    process = new JObject();
    process->SetMethod(JSCT("binding"), Binding);
    process->SetMethod(JSCT("compile"), Compile);
    process->SetMethod(JSCT("compileNativePtr"), CompileNativePtr);
    process->SetMethod(JSCT("readSource"), ReadSource);
    process->SetMethod(JSCT("cwd"), Cwd);
    process->SetMethod(JSCT("doExit"), DoExit);
    SetProcessEnv(process);

    // process.native_sources
    JObject native_sources;
    SetNativeSources(&native_sources);
    process->SetProperty(JSCT("native_sources"), native_sources);

    // process.platform
    JObject platform(JSCT(PLATFORM));
    process->SetProperty(JSCT("platform"), platform);

    // process.arch
    JObject arch(JSCT(ARCHITECTURE));
    process->SetProperty(JSCT("arch"), arch);

    // Binding module id.
    JObject jbinding = process->GetProperty(JSCT("binding"));

#define ENUMDEF_MODULE_LIST(upper, Camel, lower) \
    jbinding.SetProperty(JSCT(# lower), JVal::Number(MODULE_ ## upper));

    MAP_MODULE_LIST(ENUMDEF_MODULE_LIST)

#undef ENUMDEF_MODULE_LIST

    module->module = process;
  }

  return process;
}

void SetProcessIotjs(JObject* process) {
  // IoT.js specific
  JObject iotjs;
  process->SetProperty(JSCT("iotjs"), iotjs);
}


} // namespace iotjs