// SPDX-License-Identifier: Apache-2.0
// Fallback stub when Cairo is not available at build time.
#include "image_export.h"
#include "Movie.h"

bool exportTop100Image(const std::vector<Movie>&, const std::string&, const std::string&) {
    // Not supported without Cairo; return false to signal failure.
    return false;
}
