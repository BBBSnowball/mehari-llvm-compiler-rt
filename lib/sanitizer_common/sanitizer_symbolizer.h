//===-- sanitizer_symbolizer.h ----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Symbolizer is intended to be used by both
// AddressSanitizer and ThreadSanitizer to symbolize a given
// address. It is an analogue of addr2line utility and allows to map
// instruction address to a location in source code at run-time.
//
// Symbolizer is planned to use debug information (in DWARF format)
// in a binary via interface defined in "llvm/DebugInfo/DIContext.h"
//
// Symbolizer code should be called from the run-time library of
// dynamic tools, and generally should not call memory allocation
// routines or other system library functions intercepted by those tools.
// Instead, Symbolizer code should use their replacements, defined in
// "compiler-rt/lib/sanitizer_common/sanitizer_libc.h".
//===----------------------------------------------------------------------===//
#ifndef SANITIZER_SYMBOLIZER_H
#define SANITIZER_SYMBOLIZER_H

#include "sanitizer_allocator_internal.h"
#include "sanitizer_internal_defs.h"
#include "sanitizer_libc.h"
// WARNING: Do not include system headers here. See details above.

namespace __sanitizer {

struct AddressInfo {
  uptr address;
  char *module;
  uptr module_offset;
  char *function;
  char *file;
  int line;
  int column;

  AddressInfo() {
    internal_memset(this, 0, sizeof(AddressInfo));
  }

  // Deletes all strings and sets all fields to zero.
  void Clear() {
    InternalFree(module);
    InternalFree(function);
    InternalFree(file);
    internal_memset(this, 0, sizeof(AddressInfo));
  }

  void FillAddressAndModuleInfo(uptr addr, const char *mod_name,
                                uptr mod_offset) {
    address = addr;
    module = internal_strdup(mod_name);
    module_offset = mod_offset;
  }
};

struct DataInfo {
  uptr address;
  char *module;
  uptr module_offset;
  char *name;
  uptr start;
  uptr size;
};

class Symbolizer {
 public:
  /// Returns platform-specific implementation of Symbolizer. The symbolizer
  /// must be initialized (with init or disable) before calling this function.
  static Symbolizer *Get();
  /// Returns platform-specific implementation of Symbolizer, or null if not
  /// initialized.
  static Symbolizer *GetOrNull();
  /// Returns platform-specific implementation of Symbolizer.  Will
  /// automatically initialize symbolizer as if by calling Init(0) if needed.
  static Symbolizer *GetOrInit();
  /// Initialize and return the symbolizer, given an optional path to an
  /// external symbolizer.  The path argument is only required for legacy
  /// reasons as this function will check $PATH for an external symbolizer.  Not
  /// thread safe.
  static Symbolizer *Init(const char* path_to_external = 0);
  /// Initialize the symbolizer in a disabled state.  Not thread safe.
  static Symbolizer *Disable();
  // Fills at most "max_frames" elements of "frames" with descriptions
  // for a given address (in all inlined functions). Returns the number
  // of descriptions actually filled.
  virtual uptr SymbolizeCode(uptr address, AddressInfo *frames,
                             uptr max_frames) {
    return 0;
  }
  virtual bool SymbolizeData(uptr address, DataInfo *info) {
    return false;
  }
  virtual bool IsAvailable() {
    return false;
  }
  virtual bool IsExternalAvailable() {
    return false;
  }
  // Release internal caches (if any).
  virtual void Flush() {}
  // Attempts to demangle the provided C++ mangled name.
  virtual const char *Demangle(const char *name) {
    return name;
  }
  virtual void PrepareForSandboxing() {}

 private:
  /// Platform-specific function for creating a Symbolizer object.
  static Symbolizer *PlatformInit(const char *path_to_external);
  /// Create a symbolizer and store it to symbolizer_ without checking if one
  /// already exists.  Not thread safe.
  static Symbolizer *CreateAndStore(const char *path_to_external);

  static atomic_uintptr_t symbolizer_;

 protected:
  static LowLevelAllocator symbolizer_allocator_;
};

}  // namespace __sanitizer

#endif  // SANITIZER_SYMBOLIZER_H
