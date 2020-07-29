/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "ecma-globals.h"
#include "ecma-promise-object.h"

#if ENABLED (JERRY_BUILTIN_PROMISE)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-promise-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID promise_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup promiseprototype ECMA Promise.prototype object built-in
 * @{
 */

/**
 * Promise routine: then.
 *
 * See also: 25.4.5.3
 *
 * @return ecma value of a new promise object.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_promise_prototype_then (ecma_value_t this_arg, /**< this argument */
                                     ecma_value_t on_fulfilled, /**< on_fulfilled function */
                                     ecma_value_t on_rejected) /**< on_rejected function */
{
  return ecma_promise_then (this_arg,
                            on_fulfilled,
                            on_rejected);
} /* ecma_builtin_promise_prototype_then */

/**
 * Promise routine: catch.
 *
 * See also: 25.4.5.1
 *
 * @return ecma value of a new promise object.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_promise_prototype_catch (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t on_rejected) /**< on_rejected function */
{
  ecma_value_t args[] = {ECMA_VALUE_UNDEFINED, on_rejected};
  return ecma_op_invoke_by_magic_id (this_arg, LIT_MAGIC_STRING_THEN, args, 2);
} /* ecma_builtin_promise_prototype_catch */

/**
 * Promise routine: finally.
 *
 * See also:
 *          ECMA-262 v11, 25.6.5.3
 *
 * @return ecma value of a new promise object.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_promise_prototype_finally (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t on_finally) /**< on_finally function */
{
  return ecma_promise_finally (this_arg, on_finally);
} /* ecma_builtin_promise_prototype_finally */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_PROMISE) */
