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

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "lit-char-helpers.h"
#include "jrt.h"

#if ENABLED (JERRY_BUILTIN_ARRAY)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_ARRAY_PROTOTYPE_ROUTINE_START = ECMA_BUILTIN_ID__COUNT - 1,
  /* Note: these 2 routine ids must be in this order */
#if !ENABLED (JERRY_ESNEXT)
  ECMA_ARRAY_PROTOTYPE_TO_STRING,
#endif /* !ENABLED (JERRY_ESNEXT) */
  ECMA_ARRAY_PROTOTYPE_CONCAT,
  ECMA_ARRAY_PROTOTYPE_TO_LOCALE_STRING,
  ECMA_ARRAY_PROTOTYPE_JOIN,
  ECMA_ARRAY_PROTOTYPE_POP,
  ECMA_ARRAY_PROTOTYPE_PUSH,
  ECMA_ARRAY_PROTOTYPE_REVERSE,
  ECMA_ARRAY_PROTOTYPE_SHIFT,
  ECMA_ARRAY_PROTOTYPE_SLICE,
  ECMA_ARRAY_PROTOTYPE_SORT,
  ECMA_ARRAY_PROTOTYPE_SPLICE,
  ECMA_ARRAY_PROTOTYPE_UNSHIFT,
  ECMA_ARRAY_PROTOTYPE_INDEX_OF,
  ECMA_ARRAY_PROTOTYPE_LAST_INDEX_OF,
  /* Note these 3 routines must be in this order */
  ECMA_ARRAY_PROTOTYPE_EVERY,
  ECMA_ARRAY_PROTOTYPE_SOME,
  ECMA_ARRAY_PROTOTYPE_FOR_EACH,
  ECMA_ARRAY_PROTOTYPE_MAP,
  ECMA_ARRAY_PROTOTYPE_FILTER,
  /* Note these 2 routines must be in this order */
  ECMA_ARRAY_PROTOTYPE_REDUCE,
  ECMA_ARRAY_PROTOTYPE_REDUCE_RIGHT,
  ECMA_ARRAY_PROTOTYPE_FIND,
  ECMA_ARRAY_PROTOTYPE_FIND_INDEX,
  ECMA_ARRAY_PROTOTYPE_ENTRIES,
  ECMA_ARRAY_PROTOTYPE_KEYS,
  ECMA_ARRAY_PROTOTYPE_SYMBOL_ITERATOR,
  ECMA_ARRAY_PROTOTYPE_FILL,
  ECMA_ARRAY_PROTOTYPE_COPY_WITHIN,
  ECMA_ARRAY_PROTOTYPE_INCLUDES,
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-array-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID array_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup arrayprototype ECMA Array.prototype object built-in
 * @{
 */

/**
 * Helper function to set an object's length property
 *
 * @return ecma value (return value of the [[Put]] method)
 *         Calling ecma_free_value on the returned value is optional if it is not abrupt completion.
 */
static ecma_value_t
ecma_builtin_array_prototype_helper_set_length (ecma_object_t *object, /**< object*/
                                                ecma_number_t length) /**< new length */
{
  ecma_value_t length_value = ecma_make_number_value (length);
  ecma_value_t ret_value = ecma_op_object_put (object,
                                               ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH),
                                               length_value,
                                               true);

  ecma_free_value (length_value);

  JERRY_ASSERT (ecma_is_value_boolean (ret_value)
                || ecma_is_value_empty (ret_value)
                || ECMA_IS_VALUE_ERROR (ret_value));
  return ret_value;
} /* ecma_builtin_array_prototype_helper_set_length */

/**
 * The Array.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_to_locale_string (ecma_object_t *obj_p, /**< array object */
                                                      uint32_t length) /**< array object's length */
{
  /* 5. */
  if (length == 0)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  /* 7-8. */
  ecma_string_t *first_string_p = ecma_builtin_helper_get_to_locale_string_at_index (obj_p, 0);

  if (JERRY_UNLIKELY (first_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (first_string_p);
  ecma_deref_ecma_string (first_string_p);

  /* 9-10. */
  for (uint32_t k = 1; k < length; k++)
  {
    /* 4. Implementation-defined: set the separator to a single comma character. */
    ecma_stringbuilder_append_byte (&builder, LIT_CHAR_COMMA);

    ecma_string_t *next_string_p = ecma_builtin_helper_get_to_locale_string_at_index (obj_p, k);

    if (JERRY_UNLIKELY (next_string_p == NULL))
    {
      ecma_stringbuilder_destroy (&builder);
      return ECMA_VALUE_ERROR;
    }

    ecma_stringbuilder_append (&builder, next_string_p);
    ecma_deref_ecma_string (next_string_p);
  }

  return ecma_make_string_value (ecma_stringbuilder_finalize (&builder));
} /* ecma_builtin_array_prototype_object_to_locale_string */

/**
 * The Array.prototype object's 'concat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_concat (const ecma_value_t args[], /**< arguments list */
                                            uint32_t args_number, /**< number of arguments */
                                            ecma_object_t *obj_p) /**< array object */
{
  /* 2. */
#if ENABLED (JERRY_ESNEXT)
  ecma_value_t new_array = ecma_op_array_species_create (obj_p, 0);

  if (ECMA_IS_VALUE_ERROR (new_array))
  {
    return new_array;
  }
#else /* !ENABLED (JERRY_ESNEXT) */
  ecma_value_t new_array = ecma_op_create_array_object (NULL, 0, false);
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array));
#endif /* ENABLED (JERRY_ESNEXT) */

  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);
  uint32_t new_length = 0;

  /* 5.b - 5.c for this_arg */
  ecma_value_t concat_this_value = ecma_builtin_helper_array_concat_value (new_array_p,
                                                                           &new_length,
                                                                           ecma_make_object_value (obj_p));
  if (ECMA_IS_VALUE_ERROR (concat_this_value))
  {
    ecma_deref_object (new_array_p);
    return concat_this_value;
  }

  JERRY_ASSERT (ecma_is_value_empty (concat_this_value));

  /* 5. */
  for (uint32_t arg_index = 0; arg_index < args_number; arg_index++)
  {
    ecma_value_t concat_value = ecma_builtin_helper_array_concat_value (new_array_p, &new_length, args[arg_index]);

    if (ECMA_IS_VALUE_ERROR (concat_value))
    {
      ecma_deref_object (new_array_p);
      return concat_value;
    }

    JERRY_ASSERT (ecma_is_value_empty (concat_value));
  }

  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (new_array_p,
                                                                                  ((ecma_number_t) new_length));
  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    ecma_deref_object (new_array_p);
    return set_length_value;
  }

  return new_array;
} /* ecma_builtin_array_prototype_object_concat */

/**
 * The Array.prototype.toString's separator creation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2 4th step
 *
 * @return NULL - if the conversion fails
 *         ecma_string_t * - otherwise
 */

static ecma_string_t *
ecma_op_array_get_separator_string (ecma_value_t separator) /**< possible separator */
{
  if (ecma_is_value_undefined (separator))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);
  }

  return ecma_op_to_string (separator);
} /* ecma_op_array_get_separator_string */

/**
 * The Array.prototype's 'toString' single element operation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2
 *
 * @return NULL - if the conversion fails
 *         ecma_string_t * - otherwise
 */
static ecma_string_t *
ecma_op_array_get_to_string_at_index (ecma_object_t *obj_p, /**< this object */
                                      uint32_t index) /**< array index */
{
  ecma_value_t index_value = ecma_op_object_get_by_uint32_index (obj_p, index);

  if (ECMA_IS_VALUE_ERROR (index_value))
  {
    return NULL;
  }

  if (ecma_is_value_undefined (index_value)
      || ecma_is_value_null (index_value))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_string_t *ret_str_p = ecma_op_to_string (index_value);

  ecma_free_value (index_value);

  return ret_str_p;
} /* ecma_op_array_get_to_string_at_index */

/**
 * The Array.prototype object's 'join' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_join (ecma_value_t separator_arg, /**< separator argument */
                                   ecma_object_t *obj_p, /**< array object */
                                   uint32_t length) /**< array object's length */
{
  /* 4-5. */
  ecma_string_t *separator_string_p = ecma_op_array_get_separator_string (separator_arg);

  if (JERRY_UNLIKELY (separator_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  if (length == 0)
  {
    /* 6. */
    ecma_deref_ecma_string (separator_string_p);
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  /* 7-8. */
  ecma_string_t *first_string_p = ecma_op_array_get_to_string_at_index (obj_p, 0);

  if (JERRY_UNLIKELY (first_string_p == NULL))
  {
    ecma_deref_ecma_string (separator_string_p);
    return ECMA_VALUE_ERROR;
  }

  ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (first_string_p);
  ecma_deref_ecma_string (first_string_p);

  /* 9-10. */
  for (uint32_t k = 1; k < length; k++)
  {
    /* 10.a */
    ecma_stringbuilder_append (&builder, separator_string_p);

    /* 10.d */
    ecma_string_t *next_string_p = ecma_op_array_get_to_string_at_index (obj_p, k);

    if (JERRY_UNLIKELY (next_string_p == NULL))
    {
      ecma_deref_ecma_string (separator_string_p);
      ecma_stringbuilder_destroy (&builder);
      return ECMA_VALUE_ERROR;
    }

    ecma_stringbuilder_append (&builder, next_string_p);
    ecma_deref_ecma_string (next_string_p);
  }

  ecma_deref_ecma_string (separator_string_p);
  return ecma_make_string_value (ecma_stringbuilder_finalize (&builder));
} /* ecma_builtin_array_prototype_join */

/**
 * The Array.prototype object's 'pop' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_pop (ecma_object_t *obj_p, /**< array object */
                                         uint32_t len) /**< array object's length */
{
   /* 4. */
  if (len == 0)
  {
    /* 4.a */
    ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (obj_p, ECMA_NUMBER_ZERO);

    /* 4.b */
    return ECMA_IS_VALUE_ERROR (set_length_value) ? set_length_value : ECMA_VALUE_UNDEFINED;
  }

  /* 5.b */
  len--;
  ecma_value_t get_value = ecma_op_object_get_by_uint32_index (obj_p, len);

  if (ECMA_IS_VALUE_ERROR (get_value))
  {
    return get_value;
  }

  if (ecma_op_object_is_fast_array (obj_p))
  {
    if (!ecma_op_ordinary_object_is_extensible (obj_p))
    {
      ecma_free_value (get_value);
      return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument type."));
    }

    ecma_delete_fast_array_properties (obj_p, len);

    return get_value;
  }

  /* 5.c */
  ecma_value_t del_value = ecma_op_object_delete_by_uint32_index (obj_p, len, true);

  if (ECMA_IS_VALUE_ERROR (del_value))
  {
    ecma_free_value (get_value);
    return del_value;
  }

  ecma_free_value (del_value);

  /* 5.d */
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (obj_p, ((ecma_number_t) len));

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    ecma_free_value (get_value);
    return set_length_value;
  }

  return get_value;
} /* ecma_builtin_array_prototype_object_pop */

/**
 * The Array.prototype object's 'push' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_push (const ecma_value_t *argument_list_p, /**< arguments list */
                                          uint32_t arguments_number, /**< number of arguments */
                                          ecma_object_t *obj_p, /**< array object */
                                          uint32_t length) /**< array object's length */
{
  ecma_number_t n = (ecma_number_t) length;

  if (ecma_op_object_is_fast_array (obj_p))
  {
    if (!ecma_op_ordinary_object_is_extensible (obj_p))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument type."));
    }

    if ((ecma_number_t) (length + arguments_number) > UINT32_MAX)
    {
      return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid array length"));
    }

    if (arguments_number == 0)
    {
      return ecma_make_uint32_value (length);
    }

    uint32_t new_length = length + arguments_number;
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;
    ecma_value_t *buffer_p = ecma_fast_array_extend (obj_p, new_length) + length;

    for (uint32_t index = 0; index < arguments_number; index++)
    {
      buffer_p[index] = ecma_copy_value_if_not_object (argument_list_p[index]);
    }

    ext_obj_p->u.array.u.hole_count -= ECMA_FAST_ARRAY_HOLE_ONE * arguments_number;

    return ecma_make_uint32_value (new_length);
  }

  /* 5. */
  for (uint32_t index = 0; index < arguments_number; index++, n++)
  {
    /* 5.b */
    ecma_value_t put_value = ecma_op_object_put_by_number_index (obj_p, n, argument_list_p[index], true);

    if (ECMA_IS_VALUE_ERROR (put_value))
    {
      return put_value;
    }
  }

  /* 6. */
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (obj_p, n);

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    return set_length_value;
  }

  return ecma_make_number_value (n);
} /* ecma_builtin_array_prototype_object_push */

/**
 * The Array.prototype object's 'reverse' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_reverse (ecma_value_t this_arg, /**< this argument */
                                             ecma_object_t *obj_p, /**< array object */
                                             uint32_t len) /**< array object's length */
{
  uint32_t middle = len / 2;

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.u.hole_count < ECMA_FAST_ARRAY_HOLE_ONE
        && len != 0
        && ecma_op_ordinary_object_is_extensible (obj_p))
    {
      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, obj_p->u1.property_list_cp);

      for (uint32_t i = 0; i < middle; i++)
      {
        ecma_value_t tmp = buffer_p[i];
        buffer_p[i] = buffer_p[len - 1 - i];
        buffer_p[len - 1 - i] = tmp;
      }

      return ecma_copy_value (this_arg);
    }
  }

  for (uint32_t lower = 0; lower < middle; lower++)
  {
    uint32_t upper = len - lower - 1;
    ecma_value_t ret_value = ECMA_VALUE_ERROR;

    ecma_string_t *lower_str_p = ecma_new_ecma_string_from_uint32 (lower);
    ecma_string_t *upper_str_p = ecma_new_ecma_string_from_uint32 (upper);

#if ENABLED (JERRY_ESNEXT)
    ecma_value_t lower_value = ECMA_VALUE_EMPTY;
    ecma_value_t upper_value = ECMA_VALUE_EMPTY;

    ecma_value_t has_lower = ecma_op_object_has_property (obj_p, lower_str_p);

#if ENABLED (JERRY_BUILTIN_PROXY)
    if (ECMA_IS_VALUE_ERROR (has_lower))
    {
      goto clean_up;
    }
#endif /* ENABLED (JERRY_BUILTIN_PROXY) */

    bool lower_exist = ecma_is_value_true (has_lower);

    if (lower_exist)
    {
      lower_value = ecma_op_object_get (obj_p, lower_str_p);

      if (ECMA_IS_VALUE_ERROR (lower_value))
      {
        goto clean_up;
      }
    }

    ecma_value_t has_upper = ecma_op_object_has_property (obj_p, upper_str_p);

#if ENABLED (JERRY_BUILTIN_PROXY)
    if (ECMA_IS_VALUE_ERROR (has_upper))
    {
      goto clean_up;
    }
#endif /* ENABLED (JERRY_BUILTIN_PROXY) */

    bool upper_exist = ecma_is_value_true (has_upper);

    if (upper_exist)
    {
      upper_value = ecma_op_object_get (obj_p, upper_str_p);

      if (ECMA_IS_VALUE_ERROR (upper_value))
      {
        goto clean_up;
      }
    }
#else /* !ENABLED (JERRY_ESNEXT) */
    ecma_value_t lower_value = ecma_op_object_get (obj_p, lower_str_p);

    if (ECMA_IS_VALUE_ERROR (lower_value))
    {
      ecma_deref_ecma_string (lower_str_p);
      ecma_deref_ecma_string (upper_str_p);
      return ret_value;
    }

    ecma_value_t upper_value = ecma_op_object_get (obj_p, upper_str_p);

    if (ECMA_IS_VALUE_ERROR (upper_value))
    {
      goto clean_up;
    }

    ecma_value_t has_lower = ecma_op_object_has_property (obj_p, lower_str_p);
    ecma_value_t has_upper = ecma_op_object_has_property (obj_p, upper_str_p);

    bool lower_exist = ecma_is_value_true (has_lower);
    bool upper_exist = ecma_is_value_true (has_upper);
#endif /* ENABLED (JERRY_ESNEXT) */

    if (lower_exist && upper_exist)
    {
      ecma_value_t outer_put_value = ecma_op_object_put (obj_p, lower_str_p, upper_value, true);

      if (ECMA_IS_VALUE_ERROR (outer_put_value))
      {
        goto clean_up;
      }

      ecma_value_t inner_put_value = ecma_op_object_put (obj_p, upper_str_p, lower_value, true);

      if (ECMA_IS_VALUE_ERROR (inner_put_value))
      {
        goto clean_up;
      }
    }
    else if (!lower_exist && upper_exist)
    {
      ecma_value_t put_value = ecma_op_object_put (obj_p, lower_str_p, upper_value, true);

      if (ECMA_IS_VALUE_ERROR (put_value))
      {
        goto clean_up;
      }

      ecma_value_t del_value = ecma_op_object_delete (obj_p, upper_str_p, true);

      if (ECMA_IS_VALUE_ERROR (del_value))
      {
        goto clean_up;
      }
    }
    else if (lower_exist)
    {
      ecma_value_t del_value = ecma_op_object_delete (obj_p, lower_str_p, true);

      if (ECMA_IS_VALUE_ERROR (del_value))
      {
        goto clean_up;
      }

      ecma_value_t put_value = ecma_op_object_put (obj_p, upper_str_p, lower_value, true);

      if (ECMA_IS_VALUE_ERROR (put_value))
      {
        goto clean_up;
      }
    }

    ret_value = ECMA_VALUE_EMPTY;

clean_up:
    ecma_free_value (upper_value);
    ecma_free_value (lower_value);
    ecma_deref_ecma_string (lower_str_p);
    ecma_deref_ecma_string (upper_str_p);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      return ret_value;
    }
  }

  return ecma_copy_value (this_arg);
} /* ecma_builtin_array_prototype_object_reverse */

/**
 * The Array.prototype object's 'shift' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_shift (ecma_object_t *obj_p, /**< array object */
                                           uint32_t len) /**< array object's length */
{
  /* 4. */
  if (len == 0)
  {
    ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (obj_p, ECMA_NUMBER_ZERO);

    return ECMA_IS_VALUE_ERROR (set_length_value) ? set_length_value : ECMA_VALUE_UNDEFINED;
  }

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.u.hole_count < ECMA_FAST_ARRAY_HOLE_ONE
        && len != 0
        && ecma_op_ordinary_object_is_extensible (obj_p))
    {
      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, obj_p->u1.property_list_cp);
      ecma_value_t ret_value = buffer_p[0];

      if (ecma_is_value_object (ret_value))
      {
        ecma_ref_object (ecma_get_object_from_value (ret_value));
      }

      memmove (buffer_p, buffer_p + 1, sizeof (ecma_value_t) * (len - 1));

      buffer_p[len - 1] = ECMA_VALUE_UNDEFINED;
      ecma_delete_fast_array_properties (obj_p, len - 1);

      return ret_value;
    }
  }

  /* 5. */
  ecma_value_t first_value = ecma_op_object_get_by_uint32_index (obj_p, 0);

  if (ECMA_IS_VALUE_ERROR (first_value))
  {
    return first_value;
  }

  /* 6. and 7. */
  for (uint32_t k = 1; k < len; k++)
  {
    /* 7.a - 7.c */
    ecma_value_t curr_value = ecma_op_object_find_by_uint32_index (obj_p, k);

    if (ECMA_IS_VALUE_ERROR (curr_value))
    {
      ecma_free_value (first_value);
      return curr_value;
    }

    /* 7.b */
    uint32_t to = k - 1;
    ecma_value_t operation_value;

    if (ecma_is_value_found (curr_value))
    {
      /* 7.d.i, 7.d.ii */
      operation_value = ecma_op_object_put_by_uint32_index (obj_p, to, curr_value, true);
      ecma_free_value (curr_value);
    }
    else
    {
      /* 7.e.i */
      operation_value = ecma_op_object_delete_by_uint32_index (obj_p, to, true);
    }

    if (ECMA_IS_VALUE_ERROR (operation_value))
    {
      ecma_free_value (first_value);
      return operation_value;
    }
  }

  /* 8. */
  ecma_value_t del_value = ecma_op_object_delete_by_uint32_index (obj_p, --len, true);

  if (ECMA_IS_VALUE_ERROR (del_value))
  {
    ecma_free_value (first_value);
    return del_value;
  }

  /* 9. */
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (obj_p, ((ecma_number_t) len));

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    ecma_free_value (first_value);
    return set_length_value;
  }

  /* 10. */
  return first_value;
} /* ecma_builtin_array_prototype_object_shift */

/**
 * The Array.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_slice (ecma_value_t arg1, /**< start */
                                           ecma_value_t arg2, /**< end */
                                           ecma_object_t *obj_p, /**< array object */
                                           uint32_t len) /**< array object's length */
{
  uint32_t start = 0, end = len;

  /* 5. 6.*/
  if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (arg1,
                                                                      len,
                                                                      &start)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 7. */
  if (ecma_is_value_undefined (arg2))
  {
    end = len;
  }
  else
  {
    /* 7. part 2, 8.*/
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (arg2,
                                                                        len,
                                                                        &end)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  JERRY_ASSERT (start <= len && end <= len);

  bool use_fast_path = ecma_op_object_is_fast_array (obj_p);
  uint32_t copied_length = (end > start) ? end - start : 0;
#if ENABLED (JERRY_ESNEXT)
  ecma_value_t new_array = ecma_op_array_species_create (obj_p, copied_length);

  if (ECMA_IS_VALUE_ERROR (new_array))
  {
    return new_array;
  }
  use_fast_path &= ecma_op_object_is_fast_array (ecma_get_object_from_value (new_array));
#else /* !ENABLED (JERRY_ESNEXT) */
  ecma_value_t new_array = ecma_op_create_array_object (NULL, 0, false);
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array));
#endif /* ENABLED (JERRY_ESNEXT) */

  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

  /* 9. */
  uint32_t n = 0;

  if (use_fast_path && copied_length > 0)
  {
    ecma_extended_object_t *ext_from_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_from_obj_p->u.array.u.hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (JERRY_UNLIKELY (obj_p->u1.property_list_cp == JMEM_CP_NULL))
      {
        /**
         * Very unlikely case: the buffer copied from is a fast buffer and the property list was deleted.
         * There is no need to do any copy.
         */
        return new_array;
      }

      ecma_extended_object_t *ext_to_obj_p = (ecma_extended_object_t *) new_array_p;

#if ENABLED (JERRY_ESNEXT)
      uint32_t target_length = ext_to_obj_p->u.array.length;
      ecma_value_t *to_buffer_p;
      if (copied_length == target_length)
      {
        to_buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, new_array_p->u1.property_list_cp);
      }
      else if (copied_length > target_length)
      {
        to_buffer_p = ecma_fast_array_extend (new_array_p, copied_length);
      }
      else
      {
        ecma_delete_fast_array_properties (new_array_p, copied_length);
        to_buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, new_array_p->u1.property_list_cp);
      }
#else /* !ENABLED (JERRY_ESNEXT) */
      ecma_value_t *to_buffer_p = ecma_fast_array_extend (new_array_p, copied_length);
#endif /* ENABLED (JERRY_ESNEXT) */

      ecma_value_t *from_buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, obj_p->u1.property_list_cp);

      for (uint32_t k = start; k < end; k++, n++)
      {
#if ENABLED (JERRY_ESNEXT)
        ecma_free_value_if_not_object (to_buffer_p[n]);
#endif /* ENABLED (JERRY_ESNEXT) */
        to_buffer_p[n] = ecma_copy_value_if_not_object (from_buffer_p[k]);
      }

      ext_to_obj_p->u.array.u.hole_count &= ECMA_FAST_ARRAY_HOLE_ONE - 1;

      return new_array;
    }
  }

  /* 10. */
  for (uint32_t k = start; k < end; k++, n++)
  {
    /* 10.c */
    ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, k);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      ecma_deref_object (new_array_p);
      return get_value;
    }

    if (ecma_is_value_found (get_value))
    {
      /* 10.c.ii */
      ecma_value_t put_comp = ecma_builtin_helper_def_prop_by_index (new_array_p,
                                                                     n,
                                                                     get_value,
                                                                     ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
      ecma_free_value (get_value);

#if ENABLED (JERRY_ESNEXT)
      if (ECMA_IS_VALUE_ERROR (put_comp))
      {
        ecma_deref_object (new_array_p);
        return put_comp;
      }
#else /* !ENABLED (JERRY_ESNEXT) */
      JERRY_ASSERT (ecma_is_value_true (put_comp));
#endif /* ENABLED (JERRY_ESNEXT) */
    }
  }

#if ENABLED (JERRY_ESNEXT)
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (new_array_p, ((ecma_number_t) n));

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    ecma_deref_object (new_array_p);
    return set_length_value;
  }
#endif /* ENABLED (JERRY_ESNEXT) */

  return new_array;
} /* ecma_builtin_array_prototype_object_slice */

/**
 * SortCompare abstract method
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_sort_compare_helper (ecma_value_t lhs, /**< left value */
                                                         ecma_value_t rhs, /**< right value */
                                                         ecma_value_t compare_func) /**< compare function */
{
  /*
   * ECMA-262 v5, 15.4.4.11 NOTE1: Because non-existent property values always
   * compare greater than undefined property values, and undefined always
   * compares greater than any other value, undefined property values always
   * sort to the end of the result, followed by non-existent property values.
   */
  bool lhs_is_undef = ecma_is_value_undefined (lhs);
  bool rhs_is_undef = ecma_is_value_undefined (rhs);

  if (lhs_is_undef)
  {
    return ecma_make_number_value (rhs_is_undef ? ECMA_NUMBER_ZERO : ECMA_NUMBER_ONE);
  }

  if (rhs_is_undef)
  {
    return ecma_make_number_value (ECMA_NUMBER_MINUS_ONE);
  }

  ecma_number_t result = ECMA_NUMBER_ZERO;

  if (ecma_is_value_undefined (compare_func))
  {
    /* Default comparison when no compare_func is passed. */
    ecma_string_t *lhs_str_p = ecma_op_to_string (lhs);
    if (JERRY_UNLIKELY (lhs_str_p == NULL))
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_string_t *rhs_str_p = ecma_op_to_string (rhs);
    if (JERRY_UNLIKELY (rhs_str_p == NULL))
    {
      ecma_deref_ecma_string (lhs_str_p);
      return ECMA_VALUE_ERROR;
    }

    if (ecma_compare_ecma_strings_relational (lhs_str_p, rhs_str_p))
    {
      result = ECMA_NUMBER_MINUS_ONE;
    }
    else if (!ecma_compare_ecma_strings (lhs_str_p, rhs_str_p))
    {
      result = ECMA_NUMBER_ONE;
    }
    else
    {
      result = ECMA_NUMBER_ZERO;
    }

    ecma_deref_ecma_string (rhs_str_p);
    ecma_deref_ecma_string (lhs_str_p);
  }
  else
  {
    /*
     * compare_func, if not undefined, will always contain a callable function object.
     * We checked this previously, before this function was called.
     */
    JERRY_ASSERT (ecma_op_is_callable (compare_func));
    ecma_object_t *comparefn_obj_p = ecma_get_object_from_value (compare_func);

    ecma_value_t compare_args[] = { lhs, rhs };

    ecma_value_t call_value = ecma_op_function_call (comparefn_obj_p,
                                                     ECMA_VALUE_UNDEFINED,
                                                     compare_args,
                                                     2);
    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      return call_value;
    }

    if (!ecma_is_value_number (call_value))
    {
      ecma_number_t ret_num;

      if (ECMA_IS_VALUE_ERROR (ecma_get_number (call_value, &ret_num)))
      {
        ecma_free_value (call_value);
        return ECMA_VALUE_ERROR;
      }

      result = ret_num;
    }
    else
    {
      result = ecma_get_number_from_value (call_value);
    }

    ecma_free_value (call_value);
  }

  return ecma_make_number_value (result);
} /* ecma_builtin_array_prototype_object_sort_compare_helper */

/**
 * The Array.prototype object's 'sort' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_sort (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t arg1, /**< comparefn */
                                          ecma_object_t *obj_p, /**< array object */
                                          uint32_t len) /**< array object's length */
{
  /* Check if the provided compare function is callable. */
  if (!ecma_is_value_undefined (arg1) && !ecma_op_is_callable (arg1))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Compare function is not callable."));
  }

  ecma_collection_t *array_index_props_p = ecma_new_collection ();

  for (uint32_t i = 0; i < len; i++)
  {
    ecma_string_t *prop_name_p = ecma_new_ecma_string_from_uint32 (i);

    ecma_property_descriptor_t prop_desc;
    ecma_value_t get_desc = ecma_op_object_get_own_property_descriptor (obj_p, prop_name_p, &prop_desc);

    if (ECMA_IS_VALUE_ERROR (get_desc))
    {
      ecma_collection_free (array_index_props_p);
      ecma_deref_ecma_string (prop_name_p);
      return get_desc;
    }

    if (ecma_is_value_true (get_desc))
    {
      ecma_ref_ecma_string (prop_name_p);
      ecma_collection_push_back (array_index_props_p, ecma_make_string_value (prop_name_p));
      ecma_free_property_descriptor (&prop_desc);
      continue;
    }
  }

  uint32_t defined_prop_count = array_index_props_p->item_count;

  ecma_value_t ret_value = ECMA_VALUE_ERROR;
  uint32_t copied_num = 0;
  JMEM_DEFINE_LOCAL_ARRAY (values_buffer, defined_prop_count, ecma_value_t);

  ecma_value_t *buffer_p = array_index_props_p->buffer_p;

  /* Copy unsorted array into a native c array. */
  for (uint32_t i = 0; i < array_index_props_p->item_count; i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (buffer_p[i]);

    uint32_t index = ecma_string_get_array_index (property_name_p);
    JERRY_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

    if (index >= len)
    {
      break;
    }

    ecma_value_t index_value = ecma_op_object_get (obj_p, property_name_p);

    if (ECMA_IS_VALUE_ERROR (index_value))
    {
      goto clean_up;
    }

    values_buffer[copied_num++] = index_value;
  }

  JERRY_ASSERT (copied_num == defined_prop_count);

  /* Sorting. */
  if (copied_num > 1)
  {
    const ecma_builtin_helper_sort_compare_fn_t sort_cb = &ecma_builtin_array_prototype_object_sort_compare_helper;
    ecma_value_t sort_value = ecma_builtin_helper_array_merge_sort_helper (values_buffer,
                                                                           (uint32_t) (copied_num),
                                                                           arg1,
                                                                           sort_cb);
    if (ECMA_IS_VALUE_ERROR (sort_value))
    {
      goto clean_up;
    }

    ecma_free_value (sort_value);
  }

  /* Put sorted values to the front of the array. */
  for (uint32_t index = 0; index < copied_num; index++)
  {
    ecma_value_t put_value = ecma_op_object_put_by_uint32_index (obj_p, index, values_buffer[index], true);

    if (ECMA_IS_VALUE_ERROR (put_value))
    {
      goto clean_up;
    }
  }

  ret_value = ECMA_VALUE_EMPTY;

clean_up:
  /* Free values that were copied to the local array. */
  for (uint32_t index = 0; index < copied_num; index++)
  {
    ecma_free_value (values_buffer[index]);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (values_buffer);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_collection_free (array_index_props_p);
    return ret_value;
  }

  JERRY_ASSERT (ecma_is_value_empty (ret_value));

  /* Undefined properties should be in the back of the array. */
  ecma_value_t *buffer_p = array_index_props_p->buffer_p;

  for (uint32_t i = 0; i < array_index_props_p->item_count; i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (buffer_p[i]);

    uint32_t index = ecma_string_get_array_index (property_name_p);
    JERRY_ASSERT (index != ECMA_STRING_NOT_ARRAY_INDEX);

    if (index >= copied_num && index < len)
    {
      ecma_value_t del_value = ecma_op_object_delete (obj_p, property_name_p, true);

      if (ECMA_IS_VALUE_ERROR (del_value))
      {
        ecma_collection_free (array_index_props_p);
        return del_value;
      }
    }
  }

  ecma_collection_free (array_index_props_p);

  return ecma_copy_value (this_arg);
} /* ecma_builtin_array_prototype_object_sort */

/**
 * The Array.prototype object's 'splice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_splice (const ecma_value_t args[], /**< arguments list */
                                            uint32_t args_number, /**< number of arguments */
                                            ecma_object_t *obj_p, /**< array object */
                                            uint32_t len) /**< array object's length */
{
#if ENABLED (JERRY_ESNEXT)
  ecma_value_t new_array = ecma_op_array_species_create (obj_p, 0);

  if (ECMA_IS_VALUE_ERROR (new_array))
  {
    return new_array;
  }
#else /* !ENABLED (JERRY_ESNEXT) */
  ecma_value_t new_array = ecma_op_create_array_object (NULL, 0, false);
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array));
#endif /* ENABLED (JERRY_ESNEXT) */

  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

  uint32_t start = 0;
  uint32_t delete_count = 0;

  if (args_number > 0)
  {
    /* 5. 6. */
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (args[0],
                                                                        len,
                                                                        &start)))
    {
      ecma_deref_object (new_array_p);
      return ECMA_VALUE_ERROR;
    }

    /*
     * If there is only one argument, that will be the start argument,
     * and we must delete the additional elements.
     */
    if (args_number == 1)
    {
      delete_count = len - start;
    }
    else
    {
      /* 7. */
      ecma_number_t delete_num;
      if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (args[1], &delete_num)))
      {
        ecma_deref_object (new_array_p);
        return ECMA_VALUE_ERROR;
      }

      if (!ecma_number_is_nan (delete_num))
      {
        if (ecma_number_is_negative (delete_num))
        {
          delete_count = 0;
        }
        else
        {
          delete_count = ecma_number_is_infinity (delete_num) ? len : ecma_number_to_uint32 (delete_num);

          if (delete_count > len - start)
          {
            delete_count = len - start;
          }
        }
      }
      else
      {
        delete_count = 0;
      }
    }
  }

  /* 8-9. */
  uint32_t k = 0;

  for (uint32_t del_item_idx; k < delete_count; k++)
  {
    /* 9.a - 9.b */
    del_item_idx = k + start;

    ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, del_item_idx);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      ecma_deref_object (new_array_p);
      return get_value;
    }

    if (ecma_is_value_found (get_value))
    {
      /* 9.c.ii */
      ecma_value_t put_comp = ecma_builtin_helper_def_prop_by_index (new_array_p,
                                                                     k,
                                                                     get_value,
                                                                     ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
      ecma_free_value (get_value);
#if ENABLED (JERRY_ESNEXT)
      if (ECMA_IS_VALUE_ERROR (put_comp))
      {
        ecma_deref_object (new_array_p);
        return put_comp;
      }
#else /* !ENABLED (JERRY_ESNEXT) */
      JERRY_ASSERT (ecma_is_value_true (put_comp));
#endif /* ENABLED (JERRY_ESNEXT) */
    }
  }

#if ENABLED (JERRY_ESNEXT)
  ecma_value_t new_set_length_value = ecma_builtin_array_prototype_helper_set_length (new_array_p,
                                                                                      ((ecma_number_t) delete_count));

  if (ECMA_IS_VALUE_ERROR (new_set_length_value))
  {
    ecma_deref_object (new_array_p);
    return new_set_length_value;
  }
#endif /* ENABLED (JERRY_ESNEXT) */

  /* 11. */
  uint32_t item_count;

  if (args_number > 2)
  {
    item_count = (uint32_t) (args_number - 2);
  }
  else
  {
    item_count = 0;
  }

  const uint32_t new_len = len - delete_count + item_count;

  if (item_count != delete_count)
  {
    uint32_t from, to;

    /* 12. */
    if (item_count < delete_count)
    {
      /* 12.b */
      for (k = start; k < (len - delete_count); k++)
      {
        from = k + delete_count;
        to = k + item_count;

        ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, from);

        if (ECMA_IS_VALUE_ERROR (get_value))
        {
          ecma_deref_object (new_array_p);
          return get_value;
        }

        /* 12.b.iii */
        ecma_value_t operation_value;
        if (ecma_is_value_found (get_value))
        {
          /* 12.b.iv */
          operation_value = ecma_op_object_put_by_uint32_index (obj_p, to, get_value, true);
          ecma_free_value (get_value);
        }
        else
        {
          /* 12.b.v */
          operation_value = ecma_op_object_delete_by_uint32_index (obj_p, to, true);
        }

        if (ECMA_IS_VALUE_ERROR (operation_value))
        {
          ecma_deref_object (new_array_p);
          return operation_value;
        }
      }

      /* 12.d */
      for (k = len; k > new_len; k--)
      {
        ecma_value_t del_value = ecma_op_object_delete_by_uint32_index (obj_p, k - 1, true);

        if (ECMA_IS_VALUE_ERROR (del_value))
        {
          ecma_deref_object (new_array_p);
          return del_value;
        }
      }
    }
    /* 13. */
    else
    {
      JERRY_ASSERT (item_count > delete_count);
      /* 13.b */
      for (k = len - delete_count; k > start; k--)
      {
        from = k + delete_count - 1;
        to = k + item_count - 1;
        /* 13.b.iii */
        ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, from);

        if (ECMA_IS_VALUE_ERROR (get_value))
        {
          ecma_deref_object (new_array_p);
          return get_value;
        }

        ecma_value_t operation_value;

        if (ecma_is_value_found (get_value))
        {
          /* 13.b.iv */
          operation_value = ecma_op_object_put_by_uint32_index (obj_p, to, get_value, true);
          ecma_free_value (get_value);
        }
        else
        {
          /* 13.b.v */
          operation_value = ecma_op_object_delete_by_uint32_index (obj_p, to, true);
        }

        if (ECMA_IS_VALUE_ERROR (operation_value))
        {
          ecma_deref_object (new_array_p);
          return operation_value;
        }
      }
    }
  }

  /* 15. */
  uint32_t idx = 0;
  for (uint32_t arg_index = 2; arg_index < args_number; arg_index++, idx++)
  {
    ecma_value_t put_value = ecma_op_object_put_by_uint32_index (obj_p,
                                                                 (uint32_t) (start + idx),
                                                                 args[arg_index],
                                                                 true);

    if (ECMA_IS_VALUE_ERROR (put_value))
    {
      ecma_deref_object (new_array_p);
      return put_value;
    }
  }

  /* 16. */
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (obj_p, ((ecma_number_t) new_len));

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    ecma_deref_object (new_array_p);
    return set_length_value;
  }

  return new_array;
} /* ecma_builtin_array_prototype_object_splice */

/**
 * The Array.prototype object's 'unshift' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_unshift (const ecma_value_t args[], /**< arguments list */
                                             uint32_t args_number, /**< number of arguments */
                                             ecma_object_t *obj_p, /**< array object */
                                             uint32_t len) /**< array object's length */
{

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.u.hole_count < ECMA_FAST_ARRAY_HOLE_ONE
        && len != 0
        && ecma_op_ordinary_object_is_extensible (obj_p))
    {
      if ((ecma_number_t) (len + args_number) > UINT32_MAX)
      {
        return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid array length"));
      }

      if (args_number == 0)
      {
        return ecma_make_uint32_value (len);
      }

      uint32_t new_length = len + args_number;
      ecma_value_t *buffer_p = ecma_fast_array_extend (obj_p, new_length);
      memmove (buffer_p + args_number, buffer_p, sizeof (ecma_value_t) * len);

      uint32_t index = 0;

      while (index < args_number)
      {
        buffer_p[index] = ecma_copy_value_if_not_object (args[index]);
        index++;
      }

      ext_obj_p->u.array.u.hole_count -= args_number * ECMA_FAST_ARRAY_HOLE_ONE;

      return ecma_make_uint32_value (new_length);
    }
  }

  /* 5. and 6. */
  for (uint32_t k = len; k > 0; k--)
  {
    /* 6.a, 6.c*/
    ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, k - 1);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    /* 6.b */
    ecma_number_t new_idx = ((ecma_number_t) k) + ((ecma_number_t) args_number) - 1;
    ecma_value_t operation_value;

    if (ecma_is_value_found (get_value))
    {
      /* 6.d.i, 6.d.ii */
      operation_value = ecma_op_object_put_by_number_index (obj_p, new_idx, get_value, true);
      ecma_free_value (get_value);
    }
    else
    {
      /* 6.e.i */
      operation_value = ecma_op_object_delete_by_number_index (obj_p, new_idx, true);
    }

    if (ECMA_IS_VALUE_ERROR (operation_value))
    {
      return operation_value;
    }
  }

  for (uint32_t arg_index = 0; arg_index < args_number; arg_index++)
  {
    /* 9.b */
    ecma_value_t put_value = ecma_op_object_put_by_uint32_index (obj_p, arg_index, args[arg_index], true);

    if (ECMA_IS_VALUE_ERROR (put_value))
    {
      return put_value;
    }
  }

  /* 10. */
  ecma_number_t new_len = ((ecma_number_t) len) + ((ecma_number_t) args_number);
  ecma_value_t set_length_value = ecma_builtin_array_prototype_helper_set_length (obj_p, new_len);

  if (ECMA_IS_VALUE_ERROR (set_length_value))
  {
    return set_length_value;
  }

  return ecma_make_number_value (new_len);
} /* ecma_builtin_array_prototype_object_unshift */

/**
 * The Array.prototype object's 'indexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.14
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_index_of (const ecma_value_t args[], /**< arguments list */
                                              uint32_t args_number, /**< number of arguments */
                                              ecma_object_t *obj_p, /**< array object */
                                              uint32_t len) /**< array object's length */
{
  /* 4. */
  if (len == 0)
  {
    return ecma_make_integer_value (-1);
  }

  /* 5. */
  ecma_number_t idx = 0;
  if (args_number > 1)
  {
    if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (args[1], &idx)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  /* 6. */
  if (idx >= len)
  {
    return ecma_make_number_value (-1);
  }

  /* 7. */
  ecma_number_t from_idx_num = idx;

  /* 8. */
  if (idx < 0)
  {
    from_idx_num = JERRY_MAX ((ecma_number_t) len + idx, 0);
  }

  JERRY_ASSERT (from_idx_num >= 0 && from_idx_num <= UINT32_MAX);
  uint32_t from_idx = (uint32_t) from_idx_num;

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.u.hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (JERRY_UNLIKELY (obj_p->u1.property_list_cp == JMEM_CP_NULL))
      {
        return ecma_make_integer_value (-1);
      }

      len = JERRY_MIN (ext_obj_p->u.array.length, len);

      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, obj_p->u1.property_list_cp);

      while (from_idx < len)
      {
        if (ecma_op_strict_equality_compare (args[0], buffer_p[from_idx]))
        {
          return ecma_make_uint32_value (from_idx);
        }

        from_idx++;
      }

      return ecma_make_integer_value (-1);
    }
  }

  /* 6. */
  while (from_idx < len)
  {
    /* 9.a */
    ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, from_idx);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    /* 9.b.i, 9.b.ii */
    if (ecma_is_value_found (get_value)
        && ecma_op_strict_equality_compare (args[0], get_value))
    {
      ecma_free_value (get_value);
      return ecma_make_uint32_value (from_idx);
    }

    from_idx++;

    ecma_free_value (get_value);
  }

  return ecma_make_integer_value (-1);
} /* ecma_builtin_array_prototype_object_index_of */

/**
 * The Array.prototype object's 'lastIndexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.15
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_last_index_of (const ecma_value_t args[], /**< arguments list */
                                                   uint32_t args_number, /**< number of arguments */
                                                   ecma_object_t *obj_p, /**< array object */
                                                   uint32_t len) /**< array object's length */
{
  /* 4. */
  if (len == 0)
  {
    return ecma_make_integer_value (-1);
  }

  /* 5. */
  ecma_number_t idx = (ecma_number_t) len - 1;
  if (args_number > 1)
  {
    if (ECMA_IS_VALUE_ERROR (ecma_op_to_integer (args[1], &idx)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  uint32_t from_idx;

  /* 6 */
  if (idx >= 0)
  {
    from_idx = (uint32_t) (JERRY_MIN (idx, len - 1));
  }
  else
  {
    ecma_number_t k = len + idx;
    if (k < 0)
    {
      return ecma_make_integer_value (-1);
    }
    from_idx = (uint32_t) k;
  }

  ecma_value_t search_element = (args_number > 0) ? args[0] : ECMA_VALUE_UNDEFINED;

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.u.hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (JERRY_UNLIKELY (obj_p->u1.property_list_cp == JMEM_CP_NULL))
      {
        return ecma_make_integer_value (-1);
      }

      len = JERRY_MIN (ext_obj_p->u.array.length, len);

      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, obj_p->u1.property_list_cp);

      while (from_idx < len)
      {
        if (ecma_op_strict_equality_compare (search_element, buffer_p[from_idx]))
        {
          return ecma_make_uint32_value (from_idx);
        }
        from_idx--;
      }
      return ecma_make_integer_value (-1);
    }
  }

    /* 8. */
  while (from_idx < len)
  {
    /* 8.a */
    ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, from_idx);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    /* 8.b.i, 8.b.ii */
    if (ecma_is_value_found (get_value)
        && ecma_op_strict_equality_compare (search_element, get_value))
    {
      ecma_free_value (get_value);
      return ecma_make_uint32_value (from_idx);
    }

    from_idx--;

    ecma_free_value (get_value);
  }

  return ecma_make_integer_value (-1);
} /* ecma_builtin_array_prototype_object_last_index_of */

/**
 * Type of array routine.
 */
typedef enum
{
  ARRAY_ROUTINE_EVERY, /**< Array.every: ECMA-262 v5, 15.4.4.16 */
  ARRAY_ROUTINE_SOME, /**< Array.some: ECMA-262 v5, 15.4.4.17 */
  ARRAY_ROUTINE_FOREACH, /**< Array.forEach: ECMA-262 v5, 15.4.4.18 */
  ARRAY_ROUTINE__COUNT /**< count of the modes */
} array_routine_mode;

/**
 * Applies the provided function to each element of the array as long as
 * the return value stays empty. The common function for 'every', 'some'
 * and 'forEach' of the Array prototype.
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.16
 *          ECMA-262 v5, 15.4.4.17
 *          ECMA-262 v5, 15.4.4.18
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_apply (ecma_value_t arg1, /**< callbackfn */
                          ecma_value_t arg2, /**< thisArg */
                          array_routine_mode mode, /**< array routine mode */
                          ecma_object_t *obj_p, /**< array object */
                          uint32_t len) /**< array object's length */

{
  JERRY_ASSERT (mode < ARRAY_ROUTINE__COUNT);

  /* 4. */
  if (!ecma_op_is_callable (arg1))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  /* We already checked that arg1 is callable */
  ecma_object_t *func_object_p = ecma_get_object_from_value (arg1);
  ecma_value_t current_index;

  /* 7. */
  for (uint32_t index = 0; index < len; index++)
  {
    /* 7.a - 7.c */
    ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, index);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    if (ecma_is_value_found (get_value))
    {
      /* 7.c.i */
      current_index = ecma_make_uint32_value (index);

      ecma_value_t call_args[] = { get_value, current_index, ecma_make_object_value (obj_p) };
      /* 7.c.ii */
      ecma_value_t call_value = ecma_op_function_call (func_object_p, arg2, call_args, 3);

      if (ECMA_IS_VALUE_ERROR (call_value))
      {
        ecma_free_value (get_value);
        return call_value;
      }

      bool to_boolean = ecma_op_to_boolean (call_value);

      ecma_free_value (call_value);
      ecma_free_value (get_value);

      /* 7.c.iii */
      if (mode == ARRAY_ROUTINE_EVERY && !to_boolean)
      {
        return ECMA_VALUE_FALSE;
      }
      else if (mode == ARRAY_ROUTINE_SOME && to_boolean)
      {
        return ECMA_VALUE_TRUE;
      }
    }
  }

  /* 8. */

  if (mode == ARRAY_ROUTINE_EVERY)
  {
    return ECMA_VALUE_TRUE;
  }
  else if (mode == ARRAY_ROUTINE_SOME)
  {
    return ECMA_VALUE_FALSE;
  }

  JERRY_ASSERT (mode == ARRAY_ROUTINE_FOREACH);
  return ECMA_VALUE_UNDEFINED;
} /* ecma_builtin_array_apply */

/**
 * The Array.prototype object's 'map' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.19
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_map (ecma_value_t arg1, /**< callbackfn */
                                         ecma_value_t arg2, /**< thisArg */
                                         ecma_object_t *obj_p, /**< array object */
                                         uint32_t len) /**< array object's length */
{
  /* 4. */
  if (!ecma_op_is_callable (arg1))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  /* 6. */
#if ENABLED (JERRY_ESNEXT)
  ecma_value_t new_array = ecma_op_array_species_create (obj_p, len);

  if (ECMA_IS_VALUE_ERROR (new_array))
  {
    return new_array;
  }
#else /* !ENABLED (JERRY_ESNEXT) */
  ecma_value_t length_value = ecma_make_number_value (len);
  ecma_value_t new_array = ecma_op_create_array_object (&length_value, 1, true);
  ecma_free_value (length_value);
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array));
#endif /* ENABLED (JERRY_ESNEXT) */

  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

  JERRY_ASSERT (ecma_is_value_object (arg1));
  ecma_object_t *func_object_p = ecma_get_object_from_value (arg1);

  /* 7-8. */
  ecma_value_t current_index;

  for (uint32_t index = 0; index < len; index++)
  {
    /* 8.a - 8.b */
    ecma_value_t current_value = ecma_op_object_find_by_uint32_index (obj_p, index);

    if (ECMA_IS_VALUE_ERROR (current_value))
    {
      ecma_deref_object (new_array_p);
      return current_value;
    }

    if (ecma_is_value_found (current_value))
    {
      /* 8.c.i, 8.c.ii */
      current_index = ecma_make_uint32_value (index);
      ecma_value_t call_args[] = { current_value, current_index, ecma_make_object_value (obj_p) };

      ecma_value_t mapped_value = ecma_op_function_call (func_object_p, arg2, call_args, 3);

      if (ECMA_IS_VALUE_ERROR (mapped_value))
      {
        ecma_free_value (current_value);
        ecma_deref_object (new_array_p);
        return mapped_value;
      }

      /* 8.c.iii */
      ecma_value_t put_comp = ecma_builtin_helper_def_prop_by_index (new_array_p,
                                                                     index,
                                                                     mapped_value,
                                                                     ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

      ecma_free_value (mapped_value);
      ecma_free_value (current_value);
#if ENABLED (JERRY_ESNEXT)
      if (ECMA_IS_VALUE_ERROR (put_comp))
      {
        ecma_deref_object (new_array_p);
        return put_comp;
      }
#else /* !ENABLED (JERRY_ESNEXT) */
      JERRY_ASSERT (ecma_is_value_true (put_comp));
#endif /* ENABLED (JERRY_ESNEXT) */
    }
  }

  return ecma_make_object_value (new_array_p);
} /* ecma_builtin_array_prototype_object_map */

/**
 * The Array.prototype object's 'filter' routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.20
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_filter (ecma_value_t arg1, /**< callbackfn */
                                            ecma_value_t arg2, /**< thisArg */
                                            ecma_object_t *obj_p, /**< array object */
                                            uint32_t len) /**< array object's length */
{
  /* 4. */
  if (!ecma_op_is_callable (arg1))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  /* 6. */
#if ENABLED (JERRY_ESNEXT)
  ecma_value_t new_array = ecma_op_array_species_create (obj_p, 0);

  if (ECMA_IS_VALUE_ERROR (new_array))
  {
    return new_array;
  }
#else /* !ENABLED (JERRY_ESNEXT) */
  ecma_value_t new_array = ecma_op_create_array_object (NULL, 0, false);
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (new_array));
#endif /* ENABLED (JERRY_ESNEXT) */

  ecma_object_t *new_array_p = ecma_get_object_from_value (new_array);

  /* We already checked that arg1 is callable, so it will always be an object. */
  JERRY_ASSERT (ecma_is_value_object (arg1));
  ecma_object_t *func_object_p = ecma_get_object_from_value (arg1);

  /* 8. */
  uint32_t new_array_index = 0;
  ecma_value_t current_index;

  /* 9. */
  for (uint32_t index = 0; index < len; index++)
  {
    /* 9.a - 9.c */
    ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, index);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      ecma_deref_object (new_array_p);
      return get_value;
    }

    if (ecma_is_value_found (get_value))
    {
      /* 9.c.i */
      current_index = ecma_make_uint32_value (index);

      ecma_value_t call_args[] = { get_value, current_index, ecma_make_object_value (obj_p) };
      /* 9.c.ii */
      ecma_value_t call_value = ecma_op_function_call (func_object_p, arg2, call_args, 3);

      if (ECMA_IS_VALUE_ERROR (call_value))
      {
        ecma_free_value (get_value);
        ecma_deref_object (new_array_p);
        return call_value;
      }

      /* 9.c.iii */
      if (ecma_op_to_boolean (call_value))
      {
        ecma_value_t put_comp = ecma_builtin_helper_def_prop_by_index (new_array_p,
                                                                       new_array_index,
                                                                       get_value,
                                                                       ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
#if ENABLED (JERRY_ESNEXT)
        if (ECMA_IS_VALUE_ERROR (put_comp))
        {
          ecma_free_value (call_value);
          ecma_free_value (get_value);
          ecma_deref_object (new_array_p);

          return put_comp;
        }
#else /* !ENABLED (JERRY_ESNEXT) */
        JERRY_ASSERT (ecma_is_value_true (put_comp));
#endif /* ENABLED (JERRY_ESNEXT) */
        new_array_index++;
      }

      ecma_free_value (call_value);
      ecma_free_value (get_value);
    }
  }

  return new_array;
} /* ecma_builtin_array_prototype_object_filter */

/**
 * The Array.prototype object's 'reduce' and 'reduceRight' routine
 *
 * See also:
 *         ECMA-262 v5, 15.4.4.21
 *         ECMA-262 v5, 15.4.4.22
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_reduce_from (const ecma_value_t args_p[], /**< routine's arguments */
                                uint32_t args_number, /**< arguments list length */
                                bool start_from_left, /**< whether the reduce starts from left or right */
                                ecma_object_t *obj_p, /**< array object */
                                uint32_t len) /**< array object's length */
{
  /* 4. */
  if (!ecma_op_is_callable (args_p[0]))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  /* 5. */
  if (len == 0 && args_number == 1)
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Reduce of empty array with no initial value."));
  }

  JERRY_ASSERT (ecma_is_value_object (args_p[0]));
  ecma_object_t *func_object_p = ecma_get_object_from_value (args_p[0]);

  ecma_value_t accumulator = ECMA_VALUE_UNDEFINED;

  /* 6. */
  uint32_t index = 0;
  const uint32_t last_index = len - 1;

  /* 7.a */
  if (args_number > 1)
  {
    accumulator = ecma_copy_value (args_p[1]);
  }
  else
  {
    /* 8.a */
    bool k_present = false;

    /* 8.b */
    while (!k_present && index < len)
    {
      /* 8.b.i */
      k_present = true;

      /* 8.b.ii-iii */
      ecma_value_t current_value = ecma_op_object_find_by_uint32_index (obj_p, start_from_left ? index :
                                                                                                 last_index - index);

      if (ECMA_IS_VALUE_ERROR (current_value))
      {
        return current_value;
      }

      if (ecma_is_value_found (current_value))
      {
        accumulator = current_value;
      }
      else
      {
        k_present = false;
      }

      /* 8.b.iv */
      index++;
    }

    /* 8.c */
    if (!k_present)
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Missing array element."));
    }
  }
  /* 9. */
  ecma_value_t current_index;

  for (; index < len; index++)
  {
    const uint32_t corrected_index = start_from_left ? index : last_index - index;

    /* 9.a - 9.b */
    ecma_value_t current_value = ecma_op_object_find_by_uint32_index (obj_p, corrected_index);

    if (ECMA_IS_VALUE_ERROR (current_value))
    {
      ecma_free_value (accumulator);
      return current_value;
    }

    if (ecma_is_value_found (current_value))
    {
      /* 9.c.i, 9.c.ii */
      current_index = ecma_make_uint32_value (corrected_index);
      ecma_value_t call_args[] = {accumulator, current_value, current_index, ecma_make_object_value (obj_p)};

      ecma_value_t call_value = ecma_op_function_call (func_object_p,
                                                       ECMA_VALUE_UNDEFINED,
                                                       call_args,
                                                       4);
      ecma_free_value (current_index);
      ecma_free_value (accumulator);
      ecma_free_value (current_value);

      if (ECMA_IS_VALUE_ERROR (call_value))
      {
        return call_value;
      }

      accumulator = call_value;
    }
  }

  return accumulator;
} /* ecma_builtin_array_reduce_from */

#if ENABLED (JERRY_ESNEXT)

/**
 * The Array.prototype object's 'fill' routine
 *
 * Note: this method only supports length up to uint32, instead of max_safe_integer
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_fill (ecma_value_t value, /**< value */
                                   ecma_value_t start_val, /**< start value */
                                   ecma_value_t end_val, /**< end value */
                                   ecma_object_t *obj_p, /**< array object */
                                   uint32_t len) /**< array object's length */
{
  uint32_t k, final;

  /* 5. 6. 7. */
  if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (start_val,
                                                                      len,
                                                                      &k)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 8. */
  if (ecma_is_value_undefined (end_val))
  {
    final = len;
  }
  else
  {
    /* 8 part 2, 9, 10 */
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (end_val,
                                                                        len,
                                                                        &final)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.u.hole_count < ECMA_FAST_ARRAY_HOLE_ONE
        && ecma_op_ordinary_object_is_extensible (obj_p))
    {
      if (JERRY_UNLIKELY (obj_p->u1.property_list_cp == JMEM_CP_NULL))
      {
        ecma_ref_object (obj_p);
        return ecma_make_object_value (obj_p);
      }

      ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, obj_p->u1.property_list_cp);

      while (k < final)
      {
        ecma_free_value_if_not_object (buffer_p[k]);
        buffer_p[k] = ecma_copy_value_if_not_object (value);
        k++;
      }

      ecma_ref_object (obj_p);
      return ecma_make_object_value (obj_p);
    }
  }

  /* 11. */
  while (k < final)
  {
    /* 11.a - 11.b */
    ecma_value_t put_val = ecma_op_object_put_by_number_index (obj_p, k, value, true);

    /* 11. c */
    if (ECMA_IS_VALUE_ERROR (put_val))
    {
      return put_val;
    }

    /* 11.d */
    k++;
  }

  ecma_ref_object (obj_p);
  return ecma_make_object_value (obj_p);
} /* ecma_builtin_array_prototype_fill */

/**
 * The Array.prototype object's 'find' and 'findIndex' routine
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.8
 *          ECMA-262 v6, 22.1.3.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_find (ecma_value_t predicate, /**< callback function */
                                          ecma_value_t predicate_this_arg, /**< this argument for
                                                                            *   invoke predicate */
                                          bool is_find, /**< true - find routine
                                                         *   false - findIndex routine */
                                          ecma_object_t *obj_p, /**< array object */
                                          uint32_t len) /**< array object's length */
{
  /* 5. */
  if (!ecma_op_is_callable (predicate))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  /* We already checked that predicate is callable, so it will always be an object. */
  JERRY_ASSERT (ecma_is_value_object (predicate));
  ecma_object_t *func_object_p = ecma_get_object_from_value (predicate);

  /* 7 - 8. */
  for (uint32_t index = 0; index < len; index++)
  {
    /* 8.a - 8.c */
    ecma_value_t get_value = ecma_op_object_get_by_uint32_index (obj_p, index);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    /* 8.d - 8.e */
    ecma_value_t current_index = ecma_make_uint32_value (index);

    ecma_value_t call_args[] = { get_value, current_index, ecma_make_object_value (obj_p) };

    ecma_value_t call_value = ecma_op_function_call (func_object_p, predicate_this_arg, call_args, 3);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ecma_free_value (get_value);
      return call_value;
    }

    bool call_value_to_bool = ecma_op_to_boolean (call_value);

    ecma_free_value (call_value);

    if (call_value_to_bool)
    {
      /* 8.f */
      if (is_find)
      {
        ecma_free_value (current_index);
        return get_value;
      }

      ecma_free_value (get_value);
      return current_index;
    }

    ecma_free_value (get_value);
    ecma_free_value (current_index);
  }

  /* 9. */
  return is_find ? ECMA_VALUE_UNDEFINED : ecma_make_integer_value (-1);
} /* ecma_builtin_array_prototype_object_find */

/**
 * The Array.prototype object's 'copyWithin' routine
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_array_prototype_object_copy_within (const ecma_value_t args[], /**< arguments list */
                                                 uint32_t args_number, /**< number of arguments */
                                                 ecma_object_t *obj_p, /**< array object */
                                                 uint32_t len) /**< array object's length */
{
  if (args_number == 0)
  {
    return ecma_copy_value (ecma_make_object_value (obj_p));
  }

  /* 5 - 7 */
  uint32_t target;

  if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (args[0], len, &target)))
  {
    return ECMA_VALUE_ERROR;
  }

  uint32_t start = 0;
  uint32_t end = len;

  if (args_number > 1)
  {
    /* 8 - 10 */
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (args[1], len, &start)))
    {
      return ECMA_VALUE_ERROR;
    }

    if (args_number > 2)
    {
      /* 11 */
      if (ecma_is_value_undefined (args[2]))
      {
        end = len;
      }
      else
      {
        /* 11 part 2, 12, 13 */
        if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (args[2], len, &end)))
        {
          return ECMA_VALUE_ERROR;
        }
      }
    }
  }

  if (target >= len || start >= end || end == 0)
  {
    ecma_ref_object (obj_p);
    return ecma_make_object_value (obj_p);
  }

  uint32_t count = JERRY_MIN (end - start, len - target);

  bool forward = true;

  if (start < target && target < start + count)
  {
    start = start + count - 1;
    target = target + count - 1;
    forward = false;
  }

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.u.hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (obj_p->u1.property_list_cp != JMEM_CP_NULL)
      {
        ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, obj_p->u1.property_list_cp);

        for (; count > 0; count--)
        {
          ecma_value_t copy_value = ecma_copy_value_if_not_object (buffer_p[start]);

          ecma_free_value_if_not_object (buffer_p[target]);

          buffer_p[target] = copy_value;

          if (forward)
          {
            start++;
            target++;
          }
          else
          {
            start--;
            target--;
          }
        }
      }

      ecma_ref_object (obj_p);
      return ecma_make_object_value (obj_p);
    }
  }

  while (count > 0)
  {
    ecma_value_t get_value = ecma_op_object_find_by_uint32_index (obj_p, start);

    if (ECMA_IS_VALUE_ERROR (get_value))
    {
      return get_value;
    }

    ecma_value_t op_value;

    if (ecma_is_value_found (get_value))
    {
      op_value = ecma_op_object_put_by_uint32_index (obj_p, target, get_value, true);
    }
    else
    {
      op_value = ecma_op_object_delete_by_uint32_index (obj_p, target, true);
    }

    ecma_free_value (get_value);

    if (ECMA_IS_VALUE_ERROR (op_value))
    {
      return op_value;
    }

    ecma_free_value (op_value);

    if (forward)
    {
      start++;
      target++;
    }
    else
    {
      start--;
      target--;
    }

    count--;
  }

  return ecma_copy_value (ecma_make_object_value (obj_p));
} /* ecma_builtin_array_prototype_object_copy_within */

/**
 * The Array.prototype object's 'includes' routine
 *
 * See also:
 *          ECMA-262 v11, 22.1.3.13
 *
 * @return ECMA_VALUE_ERROR -if the operation fails
 *         ECMA_VALUE_{TRUE/FALSE} - depends on whether the search element is in the array or not
 */
static ecma_value_t
ecma_builtin_array_prototype_includes (const ecma_value_t args[], /**< arguments list */
                                       uint32_t args_number, /**< number of arguments */
                                       ecma_object_t *obj_p, /**< array object */
                                       uint32_t len) /**< array object's length */
{
  /* 3. */
  if (len == 0)
  {
    return ECMA_VALUE_FALSE;
  }

  uint32_t from_index = 0;

  /* 4-7. */
  if (args_number > 1)
  {
    if (ECMA_IS_VALUE_ERROR (ecma_builtin_helper_array_index_normalize (args[1], len, &from_index)))
    {
      return ECMA_VALUE_ERROR;
    }
  }

  /* Fast array path */
  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ext_obj_p->u.array.u.hole_count < ECMA_FAST_ARRAY_HOLE_ONE)
    {
      if (obj_p->u1.property_list_cp != JMEM_CP_NULL)
      {
        len = JERRY_MIN (ext_obj_p->u.array.length, len);

        ecma_value_t *buffer_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, obj_p->u1.property_list_cp);

        while (from_index < len)
        {
          if (ecma_op_same_value_zero (buffer_p[from_index], args[0]))
          {
            return ECMA_VALUE_TRUE;
          }

          from_index++;
        }
      }

      return ECMA_VALUE_FALSE;
    }
  }

  /* 8. */
  while (from_index < len)
  {
    ecma_value_t element = ecma_op_object_get_by_uint32_index (obj_p, from_index);

    if (ECMA_IS_VALUE_ERROR (element))
    {
      return element;
    }

    if (ecma_op_same_value_zero (element, args[0]))
    {
      ecma_free_value (element);
      return ECMA_VALUE_TRUE;
    }

    ecma_free_value (element);
    from_index++;
  }

  /* 9. */
  return ECMA_VALUE_FALSE;
} /* ecma_builtin_array_prototype_includes */
#endif /* ENABLED (JERRY_ESNEXT) */

/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_array_prototype_dispatch_routine (uint16_t builtin_routine_id, /**< built-in wide routine
                                                                            *   identifier */
                                              ecma_value_t this_arg, /**< 'this' argument value */
                                              const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                                    *   passed to routine */
                                              uint32_t arguments_number) /**< length of arguments' list */
{
  ecma_value_t obj_this = ecma_op_to_object (this_arg);

  if (ECMA_IS_VALUE_ERROR (obj_this))
  {
    return obj_this;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);

  if (JERRY_UNLIKELY (builtin_routine_id <= ECMA_ARRAY_PROTOTYPE_CONCAT))
  {
    ecma_value_t ret_value;

#if !ENABLED (JERRY_ESNEXT)
    if (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_TO_STRING)
    {
      ret_value = ecma_array_object_to_string (obj_this);
    }
    else
#endif /* !ENABLED (JERRY_ESNEXT) */
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_CONCAT);
      ret_value = ecma_builtin_array_prototype_object_concat (arguments_list_p,
                                                              arguments_number,
                                                              obj_p);
    }

    ecma_deref_object (obj_p);
    return ret_value;
  }

#if ENABLED (JERRY_ESNEXT)
  if (JERRY_UNLIKELY (builtin_routine_id >= ECMA_ARRAY_PROTOTYPE_ENTRIES
                      && builtin_routine_id <= ECMA_ARRAY_PROTOTYPE_KEYS))
  {
    ecma_value_t ret_value;

    if (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_ENTRIES)
    {
      ret_value = ecma_op_create_array_iterator (obj_p, ECMA_ITERATOR_ENTRIES);
    }
    else
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_KEYS);
      ret_value = ecma_op_create_array_iterator (obj_p, ECMA_ITERATOR_KEYS);
    }

    ecma_deref_object (obj_p);
    return ret_value;
  }
#endif /* ENABLED (JERRY_ESNEXT) */

  uint32_t length;
  ecma_value_t len_value = ecma_op_object_get_length (obj_p, &length);

  if (ECMA_IS_VALUE_ERROR (len_value))
  {
    ecma_deref_object (obj_p);
    return len_value;
  }

  ecma_value_t ret_value;
  ecma_value_t routine_arg_1 = arguments_list_p[0];
  ecma_value_t routine_arg_2 = arguments_list_p[1];

  switch (builtin_routine_id)
  {
    case ECMA_ARRAY_PROTOTYPE_TO_LOCALE_STRING:
    {
      ret_value = ecma_builtin_array_prototype_object_to_locale_string (obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_JOIN:
    {
      ret_value = ecma_builtin_array_prototype_join (routine_arg_1, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_POP:
    {
      ret_value = ecma_builtin_array_prototype_object_pop (obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_PUSH:
    {
      ret_value = ecma_builtin_array_prototype_object_push (arguments_list_p,
                                                            arguments_number,
                                                            obj_p,
                                                            length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_REVERSE:
    {
      ret_value = ecma_builtin_array_prototype_object_reverse (this_arg, obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_SHIFT:
    {
      ret_value = ecma_builtin_array_prototype_object_shift (obj_p, length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_SLICE:
    {
      ret_value = ecma_builtin_array_prototype_object_slice (routine_arg_1,
                                                             routine_arg_2,
                                                             obj_p,
                                                             length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_SORT:
    {
      ret_value = ecma_builtin_array_prototype_object_sort (this_arg,
                                                            routine_arg_1,
                                                            obj_p,
                                                            length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_SPLICE:
    {
      ret_value = ecma_builtin_array_prototype_object_splice (arguments_list_p,
                                                              arguments_number,
                                                              obj_p,
                                                              length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_UNSHIFT:
    {
      ret_value = ecma_builtin_array_prototype_object_unshift (arguments_list_p,
                                                               arguments_number,
                                                               obj_p,
                                                               length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_INDEX_OF:
    {
      ret_value = ecma_builtin_array_prototype_object_index_of (arguments_list_p,
                                                                arguments_number,
                                                                obj_p,
                                                                length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_LAST_INDEX_OF:
    {
      ret_value = ecma_builtin_array_prototype_object_last_index_of (arguments_list_p,
                                                                     arguments_number,
                                                                     obj_p,
                                                                     length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_EVERY:
    case ECMA_ARRAY_PROTOTYPE_SOME:
    case ECMA_ARRAY_PROTOTYPE_FOR_EACH:
    {
      ret_value = ecma_builtin_array_apply (routine_arg_1,
                                            routine_arg_2,
                                            (array_routine_mode) builtin_routine_id - ECMA_ARRAY_PROTOTYPE_EVERY,
                                            obj_p,
                                            length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_MAP:
    {
      ret_value = ecma_builtin_array_prototype_object_map (routine_arg_1,
                                                           routine_arg_2,
                                                           obj_p,
                                                           length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_REDUCE:
    case ECMA_ARRAY_PROTOTYPE_REDUCE_RIGHT:
    {
      ret_value = ecma_builtin_array_reduce_from (arguments_list_p,
                                                  arguments_number,
                                                  builtin_routine_id == ECMA_ARRAY_PROTOTYPE_REDUCE,
                                                  obj_p,
                                                  length);
      break;
    }
#if ENABLED (JERRY_ESNEXT)
    case ECMA_ARRAY_PROTOTYPE_COPY_WITHIN:
    {
      ret_value = ecma_builtin_array_prototype_object_copy_within (arguments_list_p,
                                                                   arguments_number,
                                                                   obj_p,
                                                                   length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_FIND:
    case ECMA_ARRAY_PROTOTYPE_FIND_INDEX:
    {
      ret_value = ecma_builtin_array_prototype_object_find (routine_arg_1,
                                                            routine_arg_2,
                                                            builtin_routine_id == ECMA_ARRAY_PROTOTYPE_FIND,
                                                            obj_p,
                                                            length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_FILL:
    {
      ret_value = ecma_builtin_array_prototype_fill (routine_arg_1,
                                                     routine_arg_2,
                                                     arguments_list_p[2],
                                                     obj_p,
                                                     length);
      break;
    }
    case ECMA_ARRAY_PROTOTYPE_INCLUDES:
    {
      ret_value = ecma_builtin_array_prototype_includes (arguments_list_p,
                                                         arguments_number,
                                                         obj_p,
                                                         length);
      break;
    }
#endif /* ENABLED (JERRY_ESNEXT) */
    default:
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_ARRAY_PROTOTYPE_FILTER);

      ret_value = ecma_builtin_array_prototype_object_filter (routine_arg_1,
                                                              routine_arg_2,
                                                              obj_p,
                                                              length);
      break;
    }
  }

  ecma_free_value (len_value);
  ecma_deref_object (obj_p);

  return ret_value;
} /* ecma_builtin_array_prototype_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_ARRAY) */
