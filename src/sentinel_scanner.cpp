#include "core/sentinel_scanner.h"

#include <stdexcept>

SentinelScanner::SentinelScanner(std::string sentinel)
    : sentinel_(sentinel) {
        if (sentinel_.empty()) {
            throw std::invalid_argument("Sentinel must not be empty");
        }
}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    // Once the sentinel has been found, emit no more text.
    if (found_) {
        return {"", true};
    }

    // Combine the held-back text with the incoming chunk.
    std::string text = pending_;
    text.append(chunk);

    std::size_t position = text.find(sentinel_);

    if (position != std::string::npos) {
        std::string safe_text = text.substr(0, position);

        pending_.clear();
        found_ = true;

        return {safe_text, true};
    }

    // Keep at most sentinel length minus one trailing characters.
    std::size_t keep = sentinel_.size() - 1;

    if (text.size() < keep) {
        keep = text.size();
    }

    std::size_t safe_count = text.size() - keep;

    std::string safe_text = text.substr(0, safe_count);
    pending_ = text.substr(safe_count);

    return {safe_text, false};
}

SentinelScanner::Out SentinelScanner::flush() {
    if (found_) {
        return {"", true};
    }

    std::string safe_text = pending_;
    pending_.clear();

    return {safe_text, false};
}