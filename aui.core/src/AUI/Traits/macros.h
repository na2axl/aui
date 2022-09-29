#pragma once


/**
 * @brief Forbids object from being optimized out by compiler.
 * @param object Target object. Can be pointer-to-function, pointer-to-member, any variable.
 * @ingroup useful_macros
 * @details
 * Performs compiler hacks in order to prevent the compiler/linker from optimizing out the specified object. It's useful
 * for debugging purposes, when the function is expected to exist for being called.
 *
 * The macro should be called in any function/method which is not being optimized out (i.e. in destructor).
 *
 * Basic usage:
 * @code{cpp}
 * struct SomeClass {
 * public:
 *   ~SomeClass() {
 *       AUI_NO_OPTIMIZE_OUT(SomeClass::debuggerMethod)
 *   }
 *
 *   int debuggerMethod() {
 *     ...
 *   }
 * }
 * @endcode
 */
#define AUI_NO_OPTIMIZE_OUT(object) { auto unused = &object ; }