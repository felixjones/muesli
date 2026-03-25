/*
 * BSD 3-Clause License
 * Copyright (c) 2026, Felix Jones
 * See LICENSE file for details.
 */

#pragma once

/**
 * @brief Shared test preamble included automatically by all muesli test targets.
 *
 * On MSVC debug builds, assertion failures and CRT errors show a modal dialog
 * window which blocks automated test runs. This header redirects those reports
 * to stderr so they print to the console instead.
 */

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <cstdlib>

namespace muesli_test_detail {

inline struct preamble_init {
    preamble_init() noexcept {
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _set_error_mode(_OUT_TO_STDERR);
    }
} preamble_instance;

} // namespace muesli_test_detail
#endif

