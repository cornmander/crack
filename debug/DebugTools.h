// Copyright 2011 Google Inc.

#ifndef _crack_debug_DebugTools_h_
#define _crack_debug_DebugTools_h_

#include <string>

namespace crack { namespace debug {

/** Register the debug info for a function in the lookup tables. */
void registerDebugInfo(void *address, const std::string &funcName,
                       const std::string &fileName,
                       int lineNumber
                       );

/** Register an entire function table.  */
void registerFuncTable(const char **table);

/**
 * Finds the source location for the specified address.
 * Source info is an array of three elements:
 * - the function name
 * - the source file name
 * - the line number (stored as an integer in the pointer).
 * All three values may be null if the source information can not be obtained.
 */
void getLocation(void *address, const char *info[3]);

}} // namespace crack::debug

#endif
