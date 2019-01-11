/* Copyright 2016-present Samsung Electronics Co., Ltd. and other contributors
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
#include "iotjs_module_nucleo-f767zi.h"


jerry_value_t InitStm32f4dis() {
  jerry_value_t nucleo-f767zi = jerry_create_object();

#if defined(__NUTTX__)

  iotjs_nucleo-f767zi_pin_initialize(nucleo-f767zi);

#endif

  return nucleo-f767zi;
}
