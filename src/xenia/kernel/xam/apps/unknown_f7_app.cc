/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2015 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/kernel/xam/apps/unknown_f7_app.h"

#include "xenia/base/logging.h"
#include "xenia/base/threading.h"

namespace xe {
namespace kernel {
namespace xam {
namespace apps {

UnknownF7App::UnknownF7App(KernelState* kernel_state)
    : App(kernel_state, 0xF7) {}

X_RESULT UnknownF7App::DispatchMessageSync(uint32_t message,
                                           uint32_t buffer_ptr,
                                           uint32_t buffer_length) {
  // NOTE: buffer_length may be zero or valid.
  auto buffer = memory_->TranslateVirtual(buffer_ptr);
  switch (message) {
    case 0x00200002: {
      // Used in blades dashboard v5759 (marketplace update) and up with netplay
      XELOGD("UnknownF7AppUnk200002, unimplemented");
      return X_E_SUCCESS;
    }
  }
  XELOGE(
      "Unimplemented 0xF7 message app=%.8X, msg=%.8X, arg1=%.8X, "
      "arg2=%.8X",
      app_id(), message, buffer_ptr, buffer_length);
  return X_STATUS_UNSUCCESSFUL;
}

}  // namespace apps
}  // namespace xam
}  // namespace kernel
}  // namespace xe
