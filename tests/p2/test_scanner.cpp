#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "core/sentinel_scanner.h"

int main() {
    const std::string sentinel = "<|end_conversation|>";

    // 1. Ordinary text survives unchanged after flushing.
    {
        SentinelScanner scanner(sentinel);

        auto first = scanner.feed("Hello, world!");
        auto last = scanner.flush();

        assert(first.safe_text + last.safe_text == "Hello, world!");
        assert(!first.sentinel_found);
        assert(!last.sentinel_found);
    }

    // 2. A complete sentinel stops output, including trailing text.
    {
        SentinelScanner scanner(sentinel);

        auto result = scanner.feed("Goodbye." + sentinel + "Ignore this");

        assert(result.safe_text == "Goodbye.");
        assert(result.sentinel_found);

        auto later = scanner.feed("Ignore this too");
        auto last = scanner.flush();

        assert(later.safe_text.empty());
        assert(later.sentinel_found);
        assert(last.safe_text.empty());
        assert(last.sentinel_found);
    }

    // 3. Detect the sentinel at every possible two-chunk split.
    {
        const std::string text = "Goodbye." + sentinel;

        for (std::size_t split = 0; split <= text.size(); ++split) {
            SentinelScanner scanner(sentinel);

            auto first = scanner.feed(text.substr(0, split));
            auto second = scanner.feed(text.substr(split));
            auto last = scanner.flush();

            assert(first.sentinel_found || second.sentinel_found);
            assert(first.safe_text + second.safe_text
                   + last.safe_text == "Goodbye.");
        }
    }

    // 4. Detect a sentinel arriving one character at a time.
    {
        SentinelScanner scanner(sentinel);
        const std::string text = "Bye." + sentinel;
        std::string output;

        for (std::size_t i = 0; i < text.size(); ++i) {
            auto result = scanner.feed(text.substr(i, 1));
            output += result.safe_text;

            // The match must occur only on the final character.
            assert(result.sentinel_found == (i == text.size() - 1));
        }

        assert(output == "Bye.");
        assert(scanner.flush().safe_text.empty());
    }

    // 5. Similar-looking text must not trigger a match.
    {
        SentinelScanner scanner(sentinel);
        const std::string text = "Hello <|end_world|>!";
        std::string output;

        for (char character : text) {
            auto result = scanner.feed(std::string(1, character));
            output += result.safe_text;
            assert(!result.sentinel_found);
        }

        auto last = scanner.flush();
        output += last.safe_text;

        assert(!last.sentinel_found);
        assert(output == text);
    }

    // 6. An unfinished sentinel is ordinary text at stream end.
    {
        SentinelScanner scanner(sentinel);

        auto first = scanner.feed("Bye.<|end_");
        auto last = scanner.flush();

        assert(first.safe_text + last.safe_text == "Bye.<|end_");
        assert(!first.sentinel_found);
        assert(!last.sentinel_found);

        // Flushing again must not duplicate text.
        assert(scanner.flush().safe_text.empty());
    }

    // 7. Empty input produces no output or match.
    {
        SentinelScanner scanner(sentinel);

        auto result = scanner.feed("");
        auto last = scanner.flush();

        assert(result.safe_text.empty());
        assert(!result.sentinel_found);
        assert(last.safe_text.empty());
        assert(!last.sentinel_found);
    }

    // 8. An empty sentinel is rejected.
    {
        bool threw = false;

        try {
            SentinelScanner scanner("");
        } catch (const std::invalid_argument&) {
            threw = true;
        }

        assert(threw);
    }

    std::cout << "Scanner tests passed.\n";
}