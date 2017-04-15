#ifndef EINA_STRBUF_H
#define EINA_STRBUF_H

#include <stddef.h>
#include <stdarg.h>

#include "eina_types.h"
#include "eina_slice.h"


/**
 * @page tutorial_strbuf Eina_Strbuf example
 * @dontinclude eina_strbuf_01.c
 *
 * First thing always is including Eina:
 * @skipline #include
 * @until #include
 *
 * Next we initialize eina and create a string buffer to play with:
 * @until strbuf_new
 *
 * Here you can see two different ways of creating a buffer with the same
 * contents. We could create them in simpler ways, but this gives us an
 * opportunity to demonstrate several functions in action:
 * @until strbuf_reset
 * @until strbuf_reset
 *
 * Next we use the printf family of functions to create a formated string,
 * add, remove and replace some content:
 * @until strbuf_string_get
 * @until strbuf_string_get
 * @until strbuf_string_get
 *
 * Once done we free our string buffer, shut down Eina and end the application:
 * @until }
 *
 * @example eina_strbuf_01.c
 */
/**
 * @addtogroup Eina_String_Buffer_Group String Buffer
 *
 * @brief These functions provide string buffers management.
 *
 * The String Buffer data type is designed to be a mutable string,
 * allowing to append, prepend or insert a string to a buffer.
 *
 * For more information see @ref tutorial_strbuf "this example".
 */

/**
 * @addtogroup Eina_Data_Types_Group Data Types
 *
 * @{
 */

/**
 * @defgroup Eina_String_Buffer_Group String Buffer
 *
 * @{
 */

/**
 * @typedef Eina_Strbuf
 * Type for a string buffer.
 */
typedef struct _Eina_Strbuf Eina_Strbuf;

/**
 * @brief Creates a new string buffer.
 *
 * @return Newly allocated string buffer instance.
 *
 * This function creates a new string buffer. On error, @c NULL is
 * returned. To free the resources, use eina_strbuf_free().
 *
 * @see eina_strbuf_free()
 * @see eina_strbuf_append()
 * @see eina_strbuf_string_get()
 */
EAPI Eina_Strbuf *eina_strbuf_new(void) EINA_MALLOC EINA_WARN_UNUSED_RESULT;

/**
 * @brief Creates a new string buffer using the passed string. The passed
 * string is used directly as the buffer, it's somehow the opposite function of
 * @ref eina_strbuf_string_steal . The passed string must be malloced.
 *
 * @param str The string to manage
 * @return Newly allocated string buffer instance.
 *
 * This function creates a new string buffer. On error, @c NULL is
 * returned. To free the resources, use eina_strbuf_free().
 *
 * @see eina_strbuf_free()
 * @see eina_strbuf_append()
 * @see eina_strbuf_string_get()
 * @since 1.1.0
 */
EAPI Eina_Strbuf *eina_strbuf_manage_new(char *str) EINA_MALLOC EINA_WARN_UNUSED_RESULT;

/**
 * @brief Creates a new string buffer using the passed string. The passed
 * string is used directly as the buffer, it's somehow the opposite function of
 * @ref eina_strbuf_string_steal . The passed string must be malloced.
 *
 * @param str The string to manage
 * @param length The length of the string.
 * @return Newly allocated string buffer instance.
 *
 * This function creates a new string buffer. On error, @c NULL is
 * returned. To free the resources, use eina_strbuf_free().
 *
 * @see eina_strbuf_manage_new()
 * @since 1.2.0
 */
EAPI Eina_Strbuf *eina_strbuf_manage_new_length(char *str, size_t length) EINA_MALLOC EINA_WARN_UNUSED_RESULT;

/**
 * @brief Creates a new string buffer using the passed string. The passed
 * string is used directly as the buffer, it's somehow the opposite function of
 * @ref eina_strbuf_string_steal . The passed string must be malloced.
 *
 * @param str The string to manage
 * @param length The length of the string.
 * @return Newly allocated string buffer instance.
 *
 * This function creates a new string buffer. On error, @c NULL is
 * returned. To free the resources, use eina_strbuf_free().
 *
 * @see eina_strbuf_manage_new()
 * @since 1.9.0
 */
EAPI Eina_Strbuf *eina_strbuf_manage_read_only_new_length(const char *str, size_t length) EINA_MALLOC EINA_WARN_UNUSED_RESULT;

/**
 * @brief Frees a string buffer.
 *
 * @param buf The string buffer to free.
 *
 * This function frees the memory of @p buf. @p buf must have been
 * created by eina_strbuf_new().
 */
EAPI void eina_strbuf_free(Eina_Strbuf *buf) EINA_ARG_NONNULL(1);

/**
 * @brief Resets a string buffer.
 *
 * @param buf The string buffer to reset.
 *
 * This function reset @p buf: the buffer len is set to 0, and the
 * string is set to '\\0'. No memory is free'd.
 */
EAPI void eina_strbuf_reset(Eina_Strbuf *buf) EINA_ARG_NONNULL(1);

/**
 * @brief Appends a string to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to append to.
 * @param str The string to append.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function appends @p str to @p buf. It computes the length of
 * @p str, so is slightly slower than eina_strbuf_append_length(). If
 * the length is known beforehand, consider using that variant. If
 * @p buf can't append it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 *
 * @see eina_strbuf_append()
 * @see eina_strbuf_append_length()
 */
EAPI Eina_Bool eina_strbuf_append(Eina_Strbuf *buf, const char *str) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Appends an escaped string to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to append to.
 * @param str The string to append.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function escapes and then appends the string @p str to @p buf. If @p str
 * can not be appended, #EINA_FALSE is returned, otherwise, #EINA_TRUE is
 * returned.
 */
EAPI Eina_Bool eina_strbuf_append_escaped(Eina_Strbuf *buf, const char *str) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Appends a string to a buffer, reallocating as necessary,
 * limited by the given length.
 *
 * @param buf The string buffer to append to.
 * @param str The string to append.
 * @param maxlen The maximum number of characters to append.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function appends at most @p maxlen characters of @p str to
 * @p buf. It can't append more than the length of @p str. It
 * computes the length of @p str, so it is slightly slower than
 * eina_strbuf_append_length(). If the length is known beforehand,
 * consider using that variant (@p maxlen should then be checked so
 * that it is greater than the size of @p str). If @p str can not be
 * appended, #EINA_FALSE is returned, otherwise, #EINA_TRUE is
 * returned.
 *
 * @see eina_strbuf_append()
 * @see eina_strbuf_append_length()
 */
EAPI Eina_Bool eina_strbuf_append_n(Eina_Strbuf *buf, const char *str, size_t maxlen) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Appends a string of exact length to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to append to.
 * @param str The string to append.
 * @param length The exact length to use.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function appends @p str to @p buf. @p str must be of size at
 * most @p length. It is slightly faster than eina_strbuf_append() as
 * it does not compute the size of @p str. It is useful when dealing
 * with strings of known size, such as eina_stringshare. If @p buf
 * can't append it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 *
 * @see eina_stringshare_length()
 * @see eina_strbuf_append()
 * @see eina_strbuf_append_n()
 */
EAPI Eina_Bool eina_strbuf_append_length(Eina_Strbuf *buf, const char *str, size_t length) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Append a slice to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to append to.
 * @param slice The slice to append.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function appends @p slice to @p buf. If @p buf can't append
 * it, #EINA_FALSE is returned, otherwise #EINA_TRUE is returned.
 *
 * @since 1.19.0
 */
EAPI Eina_Bool eina_strbuf_append_slice(Eina_Strbuf *buf, const Eina_Slice slice) EINA_ARG_NONNULL(1);

/**
 * @brief Appends an Eina_Strbuf to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to append to.
 * @param data The string buffer to append.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function appends @p data to @p buf. @p data must be allocated and
 * different from @NULL. It is slightly faster than eina_strbuf_append() as
 * it does not compute the size of @p str. If @p buf can't append it,
 * #EINA_FALSE is returned, otherwise #EINA_TRUE is returned.
 *
 * @see eina_strbuf_append()
 * @see eina_strbuf_append_n()
 * @see eina_strbuf_append_length()
 * @since 1.9.0
 */
EAPI Eina_Bool eina_strbuf_append_buffer(Eina_Strbuf *buf, const Eina_Strbuf *data) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Appends a character to a string buffer, reallocating as
 * necessary.
 *
 * @param buf The string buffer to append to.
 * @param c The char to append.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function inserts @p c to @p buf. If it can not insert it, #EINA_FALSE
 * is returned, otherwise #EINA_TRUE is returned.
 */
EAPI Eina_Bool eina_strbuf_append_char(Eina_Strbuf *buf, char c) EINA_ARG_NONNULL(1);

/**
 * @brief Appends a string to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to append to.
 * @param fmt The string to append.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function appends the string defined by the format @p fmt to @p buf. @p
 * fmt must be of a valid format for printf family of functions. If it can't
 * insert it, #EINA_FALSE is returned, otherwise #EINA_TRUE is returned.
 *
 * @see eina_strbuf_append()
 */
EAPI Eina_Bool eina_strbuf_append_printf(Eina_Strbuf *buf, const char *fmt, ...) EINA_ARG_NONNULL(1, 2) EINA_PRINTF(2, 3);

/**
 * @brief Appends a string to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to append to.
 * @param fmt The string to append.
 * @param args The variable arguments.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * @see eina_strbuf_append_printf()
 */
EAPI Eina_Bool eina_strbuf_append_vprintf(Eina_Strbuf *buf, const char *fmt, va_list args) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Inserts a string to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to insert.
 * @param str The string to insert.
 * @param pos The position to insert the string.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function inserts @p str to @p buf at position @p pos. It
 * computes the length of @p str, so is slightly slower than
 * eina_strbuf_insert_length(). If  the length is known beforehand,
 * consider using that variant. If @p buf can't insert it, #EINA_FALSE
 * is returned, otherwise #EINA_TRUE is returned.
 */
EAPI Eina_Bool eina_strbuf_insert(Eina_Strbuf *buf, const char *str, size_t pos) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Inserts an escaped string to a buffer, reallocating as
 * necessary.
 *
 * @param buf The string buffer to insert to.
 * @param str The string to insert.
 * @param pos The position to insert the string.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function escapes and inserts the string @p str to @p buf at
 * position @p pos. If @p buf can't insert @p str, #EINA_FALSE is
 * returned, otherwise #EINA_TRUE is returned.
 */
EAPI Eina_Bool eina_strbuf_insert_escaped(Eina_Strbuf *buf, const char *str, size_t pos) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Inserts a string to a buffer, reallocating as necessary. Limited by maxlen.
 *
 * @param buf The string buffer to insert to.
 * @param str The string to insert.
 * @param maxlen The maximum number of chars to insert.
 * @param pos The position to insert the string.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function inserts @p str to @p buf at position @p pos, with at
 * most @p maxlen bytes. The number of inserted characters can not be
 * greater than the length of @p str. It computes the length of
 * @p str, so is slightly slower than eina_strbuf_insert_length(). If the
 * length is known beforehand, consider using that variant (@p maxlen
 * should then be checked so that it is greater than the size of
 * @p str). If @p str can not be inserted, #EINA_FALSE is returned,
 * otherwise, #EINA_TRUE is returned.
 */
EAPI Eina_Bool eina_strbuf_insert_n(Eina_Strbuf *buf, const char *str, size_t maxlen, size_t pos) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Inserts a string of exact length to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to insert to.
 * @param str The string to insert.
 * @param length The exact length to use.
 * @param pos The position to insert the string.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function inserts @p str to @p buf. @p str must be of size at
 * most @p length. It is slightly faster than eina_strbuf_insert() as
 * it does not compute the size of @p str. It is useful when dealing
 * with strings of known size, such as eina_strngshare. If @p buf
 * can't insert it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 *
 * @see eina_stringshare_length()
 * @see eina_strbuf_insert()
 * @see eina_strbuf_insert_n()
 */
EAPI Eina_Bool eina_strbuf_insert_length(Eina_Strbuf *buf, const char *str, size_t length, size_t pos) EINA_ARG_NONNULL(1, 2);

/**
 * @brief Insert a slice to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to insert to.
 * @param slice The slice to insert.
 * @param pos The position to insert the string.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function inserts @p slice to @p buf at position @p pos. If @p
 * buf can't insert it, #EINA_FALSE is returned, otherwise #EINA_TRUE
 * is returned.
 *
 * @since 1.19.0
 */
EAPI Eina_Bool eina_strbuf_insert_slice(Eina_Strbuf *buf, const Eina_Slice slice, size_t pos) EINA_ARG_NONNULL(1);

/**
 * @brief Inserts a character to a string buffer, reallocating as
 * necessary.
 *
 * @param buf The string buffer to insert to.
 * @param c The char to insert.
 * @param pos The position to insert the char.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function inserts @p c to @p buf at position @p pos. If @p buf
 * can't append it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 */
EAPI Eina_Bool eina_strbuf_insert_char(Eina_Strbuf *buf, char c, size_t pos) EINA_ARG_NONNULL(1);

/**
 * @brief Inserts a string to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to insert.
 * @param fmt The string to insert.
 * @param pos The position to insert the string.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function insert a string as described by the format @p fmt to @p buf at
 * the position @p pos. @p fmt must be of a valid format for printf family of
 * functions. If it can't insert it, #EINA_FALSE is returned,
 * otherwise #EINA_TRUE is returned.
 */
EAPI Eina_Bool eina_strbuf_insert_printf(Eina_Strbuf *buf, const char *fmt, size_t pos, ...) EINA_ARG_NONNULL(1, 2) EINA_PRINTF(2, 4);

/**
 * @brief Inserts a string to a buffer, reallocating as necessary.
 *
 * @param buf The string buffer to insert.
 * @param fmt The string to insert.
 * @param pos The position to insert the string.
 * @param args The variable arguments.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * @see eina_strbuf_insert_printf
 */
EAPI Eina_Bool eina_strbuf_insert_vprintf(Eina_Strbuf *buf, const char *fmt, size_t pos, va_list args) EINA_ARG_NONNULL(1, 2);

/**
 * @def eina_strbuf_prepend(buf, str)
 * @brief Prepends the given string to the given buffer.
 *
 * @param buf The string buffer to prepend to.
 * @param str The string to prepend.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This macro is calling eina_strbuf_insert() at position 0. If @p buf
 * can't prepend it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 */
#define eina_strbuf_prepend(buf, str)                eina_strbuf_insert(buf, str, 0)

/**
 * @def eina_strbuf_prepend_escaped(buf, str)
 * @brief Prepends the given escaped string to the given buffer.
 *
 * @param buf The string buffer to prepend to.
 * @param str The string to prepend.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This macro is calling eina_strbuf_insert_escaped() at position 0. If
 * @p buf can't prepend it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 */
#define eina_strbuf_prepend_escaped(buf, str)        eina_strbuf_insert_escaped(buf, str, 0)

/**
 * @def eina_strbuf_prepend_n(buf, str)
 * @brief Prepends the given escaped string to the given buffer.
 *
 * @param buf The string buffer to prepend to.
 * @param str The string to prepend.
 * @param maxlen The maximum number of chars to prepend.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This macro is calling eina_strbuf_insert_n() at position 0. If
 * @p buf can't prepend it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 */
#define eina_strbuf_prepend_n(buf, str, maxlen)      eina_strbuf_insert_n(buf, str, maxlen, 0)

/**
 * @def eina_strbuf_prepend_length(buf, str)
 * @brief Prepends the given escaped string to the given buffer.
 *
 * @param buf The string buffer to prepend to.
 * @param str The string to prepend.
 * @param length The exact length to use.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This macro is calling eina_strbuf_insert_length() at position 0. If
 * @p buf can't prepend it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 */
#define eina_strbuf_prepend_length(buf, str, length) eina_strbuf_insert_length(buf, str, length, 0)

/**
 * @def eina_strbuf_prepend_char(buf, str)
 * @brief Prepends the given character to the given buffer.
 *
 * @param buf The string buffer to prepend to.
 * @param c The character to prepend.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This macro is calling eina_strbuf_insert_char() at position 0. If
 * @p buf can't prepend it, #EINA_FALSE is returned, otherwise #EINA_TRUE
 * is returned.
 */
#define eina_strbuf_prepend_char(buf, c)             eina_strbuf_insert_char(buf, c, 0)

/**
 * @def eina_strbuf_prepend_printf(buf, fmt, ...)
 * @brief Prepends the given string to the given buffer.
 *
 * @param buf The string buffer to prepend to.
 * @param fmt The string to prepend.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This macro is calling eina_strbuf_insert_printf() at position 0. If @p buf
 * can't prepend it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 */
#define eina_strbuf_prepend_printf(buf, fmt, ...)    eina_strbuf_insert_printf(buf, fmt, 0, ## __VA_ARGS__)

/**
 * @def eina_strbuf_prepend_vprintf(buf, fmt, args)
 * @brief Prepends the given string to the given buffer.
 *
 * @param buf The string buffer to prepend to.
 * @param fmt The string to prepend.
 * @param args The variable arguments.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This macro is calling eina_strbuf_insert_vprintf() at position 0.If @p buf
 * can't prepend it, #EINA_FALSE is returned, otherwise #EINA_TRUE is
 * returned.
 */
#define eina_strbuf_prepend_vprintf(buf, fmt, args)  eina_strbuf_insert_vprintf(buf, fmt, 0, args)

/**
 * @brief Removes a slice of the given string buffer.
 *
 * @param buf The string buffer to remove a slice.
 * @param start The initial (inclusive) slice position to start
 *        removing, in bytes.
 * @param end The final (non-inclusive) slice position to finish
 *        removing, in bytes.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function removes a slice of @p buf, starting at @p start
 * (inclusive) and ending at @p end (non-inclusive). Both values are
 * in bytes. It returns #EINA_FALSE on failure, #EINA_TRUE otherwise.
 */

EAPI Eina_Bool eina_strbuf_remove(Eina_Strbuf *buf, size_t start, size_t end) EINA_ARG_NONNULL(1);

/**
 * @brief Retrieves a pointer to the contents of a string buffer.
 *
 * @param buf The string buffer.
 * @return The current string in the string buffer.
 *
 * This function returns the string contained in @p buf. The returned
 * value must not be modified and will no longer be valid if @p buf is
 * modified. In other words, any eina_strbuf_append() or similar will
 * make that pointer invalid. The pointer returned by this function <b>must
 * not</b> be freed.
 *
 * @see eina_strbuf_string_steal()
 */
EAPI const char *eina_strbuf_string_get(const Eina_Strbuf *buf) EINA_ARG_NONNULL(1) EINA_WARN_UNUSED_RESULT;

/**
 * @brief Steals the contents of a string buffer.
 *
 * @param buf The string buffer to steal.
 * @return The current string in the string buffer.
 *
 * This function returns the string contained in @p buf. @p buf is
 * then initialized and does not own the returned string anymore. The
 * caller must release the memory of the returned string by calling
 * free().
 *
 * @see eina_strbuf_string_get()
 */
EAPI char *eina_strbuf_string_steal(Eina_Strbuf *buf) EINA_MALLOC EINA_WARN_UNUSED_RESULT EINA_ARG_NONNULL(1);

/**
 * @brief Frees the contents of a string buffer but not the buffer.
 *
 * @param buf The string buffer to free the string of.
 *
 * This function frees the string contained in @p buf without freeing
 * @p buf.
 */
EAPI void eina_strbuf_string_free(Eina_Strbuf *buf) EINA_ARG_NONNULL(1);

/**
 * @brief Retrieves the length of the string buffer content.
 *
 * @param buf The string buffer.
 * @return The current length of the string, in bytes.
 *
 * This function returns the length of @p buf.
 */
EAPI size_t    eina_strbuf_length_get(const Eina_Strbuf *buf) EINA_ARG_NONNULL(1) EINA_WARN_UNUSED_RESULT;


/**
 * @brief Replaces the n-th string with an other string.
 *
 * @param buf The string buffer to work with.
 * @param str The string to replace.
 * @param with The replacing string.
 * @param n The number of the fitting string.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function replaces the n-th occurrence of @p str in @p buf with
 * @p with. It returns #EINA_FALSE on failure, #EINA_TRUE otherwise.
 */
EAPI Eina_Bool eina_strbuf_replace(Eina_Strbuf *buf, const char *str, const char *with, unsigned int n) EINA_ARG_NONNULL(1, 2, 3);

/**
 * @def eina_strbuf_replace_first(buf, str, with)
 * @brief Prepends the given character to the given buffer.
 *
 * @param buf The string buffer to work with.
 * @param str The string to replace.
 * @param with The replaceing string.
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This macro is calling eina_strbuf_replace() with the n-th occurrence
 * equal to @c 1. If @p buf can't replace it, #EINA_FALSE is returned,
 * otherwise #EINA_TRUE is returned.
 */
#define eina_strbuf_replace_first(buf, str, with) eina_strbuf_replace(buf, str, with, 1)


/**
 * @brief Replaces all strings with an other string.

 * @param buf The string buffer to work with.
 * @param str The string to replace.
 * @param with The replacing string.
 * @return How often the string was replaced.
 *
 * This function replaces all the occurrences of @p str in @p buf with
 * the string @p with. This function returns the number of times @p str
 * has been replaced. On failure, it returns @c 0.
 */
EAPI int eina_strbuf_replace_all(Eina_Strbuf *buf, const char *str, const char *with) EINA_ARG_NONNULL(1, 2, 3);

/**
 * @brief Trims the string buffer.

 * @param buf The string buffer to work with.
 *
 * This function skips whitespaces in the beginning and the end of the buffer.
 */
EAPI void eina_strbuf_trim(Eina_Strbuf *buf) EINA_ARG_NONNULL(1);

/**
 * @brief Left trims the string buffer.

 * @param buf The string buffer to work with.
 *
 * This function skips whitespaces in the beginning of the buffer.
 */
EAPI void eina_strbuf_ltrim(Eina_Strbuf *buf) EINA_ARG_NONNULL(1);

/**
 * @brief Right trims the string buffer.

 * @param buf The string buffer to work with.
 *
 * This function skips whitespaces in the end of the buffer.
 */
EAPI void eina_strbuf_rtrim(Eina_Strbuf *buf) EINA_ARG_NONNULL(1);

/**
 * @brief Converts the string to lower case.
 *
 * @param buf The string buffer to work with.
 *
 * This function converts all the characters in the strbuf to lower case.
 *
 * @since 1.17
 */
EAPI void eina_strbuf_tolower(Eina_Strbuf *buf) EINA_ARG_NONNULL(1);

/**
 * @brief Obtains substring from the src.
 *
 * @param buf The src string.
 * @param pos The position in the source string from which the substring
 *            should be created. The first character is denoted by a
 *            value of 0 (not 1).
 * @param len The length from pos that should be copied to substring.
 *
 * This function creates a Eina_Strbuf which is a substring of buf which
 * is passed from pos position with len length.
 * If buf is NULL then returns NULL.
 * If len is 0, then an empty strbuf is returned.
 *
 * @since 1.17
 */
EAPI Eina_Strbuf * eina_strbuf_substr_get(Eina_Strbuf *buf, size_t pos, size_t len) EINA_MALLOC EINA_WARN_UNUSED_RESULT;

/**
 * @brief Gets a read-only slice representing the current strbuf contents.
 *
 * @param buf The source string.
 * @return A read-only slice for the current contents. It may become
 *         invalid as soon as the @a buf is changed.
 *
 * @since 1.19
 */
EAPI Eina_Slice eina_strbuf_slice_get(const Eina_Strbuf *buf) EINA_WARN_UNUSED_RESULT EINA_ARG_NONNULL(1);

/**
 * @brief Gets a read-write slice representing the current strbuf contents.
 *
 * @param buf The source string.
 * @return A read-write slice for the current contents. It may become
 *         invalid as soon as the @a buf is changed with calls such as
 *         eina_strbuf_append(), eina_strbuf_remove()
 *
 * @since 1.19.0
 */
EAPI Eina_Rw_Slice eina_strbuf_rw_slice_get(const Eina_Strbuf *buf) EINA_WARN_UNUSED_RESULT EINA_ARG_NONNULL(1);


/**
 * @brief Gets the string of the buffer and free the buffer
 *
 * @param buf The buffer to get the string from and which will be freed
 *
 * @return The string contained by bug. The caller must release the memory of the returned string by calling
 * free().
 *
 * @since 1.19
 */
EAPI char* eina_strbuf_release(Eina_Strbuf *buf) EINA_WARN_UNUSED_RESULT EINA_ARG_NONNULL(1);

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#endif /* EINA_STRBUF_H */
