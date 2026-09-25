// tests/p2/test_p2.cpp
//
// YOUR test suite goes here. At least 12 assert-based test cases — see
// spec §5 for the required categories and the sample test for the
// expected level of rigor.

#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "core/conversation.h"
#include "core/message.h"
#include "core/sentinel_scanner.h"

#include <memory>
#include <sstream>

#include "harness/harness.h"
#include "model/scripted_client.h"

#include <fstream>

#include "model/replay_client.h"

class TestInput : public InputSource {
public:
    explicit TestInput(const std::string& text)
        : stream_(text) {}

    std::string read_line() override {
        std::string line;

        if (!std::getline(stream_, line)) {
            eof_ = true;
        }

        return line;
    }

    bool is_eof() const override {
        return eof_;
    }

private:
    std::istringstream stream_;
    bool eof_ = false;
};

class TestOutput : public OutputSink {
public:
    void write(std::string_view text) override {
        text_.append(text);
    }

    const std::string& text() const {
        return text_;
    }

private:
    std::string text_;
};

struct P2TestAccess {
    static std::size_t capacity(const Conversation& conversation) {
        return conversation.capacity_;
    }

    static std::size_t pending_size(const SentinelScanner& scanner) {
        return scanner.pending_.size();
    }
};

void test_empty() {
    {
        Conversation history;

        // A new conversation contains no messages.
        assert(history.size() == 0);

        // No array has been allocated yet.
        assert(history.begin() == nullptr);

        // An empty conversation has identical begin/end pointers.
        assert(history.begin() == history.end());

        // Iterating over an empty conversation visits no messages.
        std::size_t visited = 0;

        for (const Message& message : history) {
            (void)message;  // Avoid an unused-variable warning.
            ++visited;
        }

        assert(visited == 0);

    } // history's destructor runs here.

    std::cout << "Empty Conversation test passed.\n";
}

void test_append() {
    {
        Conversation history;

        // Accessing an empty conversation must throw.
        bool empty_access_threw = false;

        try {
            history.at(0);
        } catch (const std::out_of_range&) {
            empty_access_threw = true;
        }

        assert(empty_access_threw);

        // Keep a system message first.
        history.append(Message(Role::System, "Be concise."));

        assert(history.size() == 1);
        assert(history.at(0).role() == Role::System);
        assert(history.at(0).content() == "Be concise.");

        // Append enough messages to trigger several reallocations.
        for (std::size_t i = 0; i < 10; ++i) {
            history.append(
                Message(Role::User, std::to_string(i))
            );

            assert(history.size() == i + 2);

            // Verify that growth preserves the first message.
            assert(history.at(0).role() == Role::System);
            assert(history.at(0).content() == "Be concise.");

            // Verify every user message added so far.
            for (std::size_t j = 0; j <= i; ++j) {
                assert(history.at(j + 1).role() == Role::User);
                assert(history.at(j + 1).content() == std::to_string(j));
            }
        }

        // Check iteration order and count.
        std::size_t index = 0;

        for (const Message& message : history) {
            assert(&message == &history.at(index));
            ++index;
        }

        assert(index == history.size());

        // The index equal to size() is just past the last message.
        bool past_end_threw = false;

        try {
            history.at(history.size());
        } catch (const std::out_of_range&) {
            past_end_threw = true;
        }

        assert(past_end_threw);

    } // The destructor now releases an allocated array.

    std::cout << "Append and access tests passed.\n";
}

void test_copy() {
    {
        // Copy an empty conversation.
        Conversation empty;
        Conversation empty_copy(empty);

        assert(empty_copy.size() == 0);
        assert(empty_copy.begin() == nullptr);

        // The empty copy must still be usable.
        empty_copy.append(Message(Role::User, "First"));
        assert(empty_copy.size() == 1);
        assert(empty.size() == 0);

        Conversation original;
        original.append(Message(Role::System, "Be concise."));
        original.append(Message(Role::User, "Hello"));

        {
            Conversation copied(original);

            // Same messages, separate arrays.
            assert(copied.size() == original.size());
            assert(copied.begin() != original.begin());

            for (std::size_t i = 0; i < original.size(); ++i) {
                assert(copied.at(i).role() == original.at(i).role());
                assert(copied.at(i).content() == original.at(i).content());
            }

            // Growing the copy must not damage the original.
            copied.append(Message(Role::Assistant, "Hi!"));

            assert(copied.size() == 3);
            assert(copied.at(2).content() == "Hi!");
            assert(original.size() == 2);
            assert(original.at(1).content() == "Hello");

        } // copied is destroyed here.

        // The original must remain valid after the copy is destroyed.
        assert(original.at(0).content() == "Be concise.");
        assert(original.at(1).content() == "Hello");

    } // Remaining conversations are destroyed here.

    std::cout << "Copy constructor tests passed.\n";
}

void test_copy_assignment() {
    {
        Conversation destination;
        destination.append(Message(Role::User, "Old message"));

        {
            Conversation original;
            original.append(Message(Role::System, "Be concise."));
            original.append(Message(Role::User, "Hello"));

            // Replace an existing conversation.
            Conversation& result = (destination = original);

            // Assignment returns the destination itself.
            assert(&result == &destination);

            // Same messages, independent arrays.
            assert(destination.size() == original.size());
            assert(destination.begin() != original.begin());

            for (std::size_t i = 0; i < original.size(); ++i) {
                assert(destination.at(i).role() == original.at(i).role());
                assert(destination.at(i).content() == original.at(i).content());
            }

            // Growing the destination must not affect the source.
            destination.append(Message(Role::Assistant, "Hi!"));

            assert(destination.size() == 3);
            assert(original.size() == 2);
            assert(original.at(1).content() == "Hello");

        } // original is destroyed.

        // The destination must still own valid messages.
        assert(destination.at(0).content() == "Be concise.");
        assert(destination.at(1).content() == "Hello");
        assert(destination.at(2).content() == "Hi!");

        // Self-assignment must preserve the conversation.
        const Message* previous_data = destination.begin();

        destination = destination;

        assert(destination.begin() == previous_data);
        assert(destination.size() == 3);
        assert(destination.at(2).content() == "Hi!");

        // Assign an empty conversation over a nonempty one.
        Conversation empty;
        destination = empty;

        assert(destination.size() == 0);
        assert(destination.begin() == nullptr);
        assert(destination.begin() == destination.end());

        // The destination must remain usable.
        destination.append(Message(Role::User, "Start again"));
        assert(destination.at(0).content() == "Start again");

    } // Remaining objects are destroyed.

    std::cout << "Copy assignment tests passed.\n";
}

void test_move() {
    {
        Conversation source;
        source.append(Message(Role::User, "Hello"));

        const Message* original_data = source.begin();

        // Move constructor: create a new destination.
        Conversation destination(std::move(source));

        assert(destination.begin() == original_data);
        assert(destination.size() == 1);
        assert(destination.at(0).role() == Role::User);
        assert(destination.at(0).content() == "Hello");

        assert(source.size() == 0);
        assert(source.begin() == nullptr);
        assert(source.begin() == source.end());

        // A moved-from object must remain usable.
        source.append(Message(Role::User, "New conversation"));

        assert(source.size() == 1);
        assert(source.at(0).content() == "New conversation");
        assert(destination.at(0).content() == "Hello");

        // Move assignment: replace an existing destination's array.
        Conversation assigned;
        assigned.append(Message(Role::Assistant, "Old message"));

        const Message* transferred_data = destination.begin();

        Conversation& result = (assigned = std::move(destination));

        assert(&result == &assigned);
        assert(assigned.begin() == transferred_data);
        assert(assigned.size() == 1);
        assert(assigned.at(0).content() == "Hello");

        assert(destination.size() == 0);
        assert(destination.begin() == nullptr);

        // Reuse the source of move assignment.
        destination.append(Message(Role::User, "Reused"));
        assert(destination.at(0).content() == "Reused");
        assert(assigned.at(0).content() == "Hello");

        // Self-move assignment must preserve a valid object.
        assigned = std::move(assigned);

        assert(assigned.begin() == transferred_data);
        assert(assigned.size() == 1);
        assert(assigned.at(0).content() == "Hello");

        // Moving an empty conversation must also work.
        Conversation empty;
        Conversation moved_empty(std::move(empty));

        assert(moved_empty.size() == 0);
        assert(moved_empty.begin() == nullptr);
        assert(empty.begin() == nullptr);

        // Move an empty conversation over a nonempty one.
        assigned = std::move(moved_empty);

        assert(assigned.size() == 0);
        assert(assigned.begin() == nullptr);
        assert(moved_empty.begin() == nullptr);

    } // All objects are destroyed here.

    std::cout << "Move tests passed.\n";
}

void test_scanner() {
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

void test_capacity_growth() {
    {
        Conversation history;

        assert(P2TestAccess::capacity(history) == 0);

        // Expected capacities after each of the first nine appends.
        const std::size_t expected[] = {
            1, 2, 4, 4, 8, 8, 8, 8, 16
        };

        for (std::size_t i = 0; i < 9; ++i) {
            history.append(Message(Role::User, std::to_string(i)));

            assert(history.size() == i + 1);
            assert(P2TestAccess::capacity(history) == expected[i]);

            // Confirm that every earlier message survives growth.
            for (std::size_t j = 0; j <= i; ++j) {
                assert(history.at(j).role() == Role::User);
                assert(history.at(j).content() == std::to_string(j));
            }
        }
    }

    std::cout << "Capacity growth test passed.\n";
}

void test_scanner_bounded_memory() {
    const std::string sentinel = "<|end_conversation|>";
    const std::string pattern = "<|end_";
    const std::size_t total_bytes = 4 * 1024 * 1024;

    SentinelScanner scanner(sentinel);

    assert(P2TestAccess::pending_size(scanner) == 0);

    std::size_t emitted = 0;

    for (std::size_t i = 0; i < total_bytes; ++i) {
        // Generate one character of a repeating partial sentinel.
        char character = pattern[i % pattern.size()];

        auto result = scanner.feed(std::string(1, character));

        // This repeating pattern never contains the full sentinel.
        assert(!result.sentinel_found);

        // Check the required bound after every character.
        assert(P2TestAccess::pending_size(scanner)
               <= sentinel.size() - 1);

        // Verify emitted text without accumulating the whole stream.
        for (char output_character : result.safe_text) {
            assert(emitted < i + 1);
            assert(output_character == pattern[emitted % pattern.size()]);
            ++emitted;
        }

        // Every received character is either emitted or still pending.
        assert(emitted + P2TestAccess::pending_size(scanner) == i + 1);
    }

    // Release and verify the final pending characters.
    auto last = scanner.flush();

    assert(!last.sentinel_found);

    for (char output_character : last.safe_text) {
        assert(emitted < total_bytes);
        assert(output_character == pattern[emitted % pattern.size()]);
        ++emitted;
    }

    assert(emitted == total_bytes);
    assert(P2TestAccess::pending_size(scanner) == 0);

    std::cout << "Scanner bounded-memory test passed.\n";
}

void test_harness_turn_limit() {
    auto model =
        std::make_unique<ScriptedModelClient>("scripts/greeting.script");

    HarnessConfig config;
    config.max_turns = 1;
    config.system_message = model->system_message();

    Harness harness(std::move(model), config);

    TestInput input("hello\nunused\n");
    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::TurnLimit);

    const Conversation& conversation = harness.conversation();

    // One system message plus one user/assistant exchange.
    assert(conversation.size() == 3);

    assert(conversation.at(0).role() == Role::System);
    assert(conversation.at(0).content() == "Be concise.");

    assert(conversation.at(1).role() == Role::User);
    assert(conversation.at(1).content() == "hello");

    assert(conversation.at(2).role() == Role::Assistant);
    assert(conversation.at(2).content()
           == "I am doing well, thank you! How can I help you?");

    // The harness must stop before reading a second user message.
    assert(input.read_line() == "unused");

    std::cout << "Harness turn-limit test passed.\n";
}

void test_harness_sentinel() {
    auto model =
        std::make_unique<ScriptedModelClient>("scripts/greeting.script");

    HarnessConfig config;
    config.system_message = model->system_message();

    Harness harness(std::move(model), config);

    TestInput input("hello\nhelp\nbye\nunused\n");
    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::Sentinel);

    const Conversation& conversation = harness.conversation();

    // One system message plus three user/assistant exchanges.
    assert(conversation.size() == 7);
    assert(conversation.at(0).role() == Role::System);
    assert(conversation.at(0).content() == "Be concise.");

    assert(conversation.at(6).role() == Role::Assistant);

    // The saved conversation retains the sentinel for replay.
    assert(conversation.at(6).content()
           == "Goodbye!<|end_conversation|>");

    // The user sees the farewell, but not the sentinel.
    assert(output.text().find("assistant> Goodbye!\n")
           != std::string::npos);

    assert(output.text().find("<|end_conversation|>")
           == std::string::npos);

    // No fourth user message should have been consumed.
    assert(input.read_line() == "unused");

    std::cout << "Harness sentinel test passed.\n";
}

void test_harness_eof() {
    auto model =
        std::make_unique<ScriptedModelClient>("scripts/greeting.script");

    HarnessConfig config;
    config.system_message = model->system_message();

    Harness harness(std::move(model), config);

    // Only one user message is available.
    TestInput input("hello\n");
    TestOutput output;

    StopReason reason = harness.run(input, output);

    assert(reason.kind == StopReason::Kind::UserExit);
    assert(input.is_eof());

    const Conversation& conversation = harness.conversation();

    // Completed messages remain available after EOF.
    assert(conversation.size() == 3);
    assert(conversation.at(0).role() == Role::System);
    assert(conversation.at(1).content() == "hello");
    assert(conversation.at(2).content()
           == "I am doing well, thank you! How can I help you?");

    std::cout << "Harness EOF test passed.\n";
}

void test_transcript_round_trip() {
    const std::string user_input = "hello\nhelp\nbye\n";
    const std::string path = "build/p2_roundtrip_test.txt";

    // Run the original conversation using the provided scripted client.
    auto scripted_model =
        std::make_unique<ScriptedModelClient>("scripts/greeting.script");

    HarnessConfig original_config;
    original_config.system_message = scripted_model->system_message();

    Harness original_harness(std::move(scripted_model), original_config);

    TestInput original_input(user_input);
    TestOutput original_output;

    StopReason original_reason =
        original_harness.run(original_input, original_output);

    assert(original_reason.kind == StopReason::Kind::Sentinel);

    const Conversation& original = original_harness.conversation();

    // Save the conversation using the specified transcript format.
    {
        std::ofstream file(path);
        assert(file.is_open());

        bool first = true;

        for (const Message& message : original) {
            if (!first) {
                file << "---\n";
            }
            first = false;

            switch (message.role()) {
                case Role::System:
                    file << "role: system\n";
                    break;

                case Role::User:
                    file << "role: user\n";
                    break;

                case Role::Assistant:
                    file << "role: assistant\n";
                    break;
            }

            file << message.content() << "\n";
        }

        file.close();
        assert(!file.fail());
    }

    // Load the saved transcript through the provided replay client.
    auto replay_model = std::make_unique<ReplayModelClient>(path);

    HarnessConfig replay_config;
    replay_config.system_message = replay_model->system_message();

    assert(replay_config.system_message == original_config.system_message);

    Harness replay_harness(std::move(replay_model), replay_config);

    // Replay supplies assistant replies; we supply the same user input.
    TestInput replay_input(user_input);
    TestOutput replay_output;

    StopReason replay_reason =
        replay_harness.run(replay_input, replay_output);

    // Both sessions must stop for the same reason and at the same turn.
    assert(replay_reason.kind == original_reason.kind);
    assert(replay_reason.detail == original_reason.detail);

    // The displayed conversation must match exactly.
    assert(replay_output.text() == original_output.text());

    // Every stored role and message must also match.
    const Conversation& replayed = replay_harness.conversation();

    assert(replayed.size() == original.size());

    for (std::size_t i = 0; i < original.size(); ++i) {
        assert(replayed.at(i).role() == original.at(i).role());
        assert(replayed.at(i).content() == original.at(i).content());
    }

    std::cout << "Transcript round-trip test passed.\n";
}

int main() {
    test_empty();
    test_append();
    test_copy();
    test_copy_assignment();
    test_move();
    test_scanner();

    test_capacity_growth();
    test_scanner_bounded_memory();

    test_harness_turn_limit();
    test_harness_sentinel();
    test_harness_eof();

    test_transcript_round_trip();

    std::cout << "All current P2 tests passed.\n";
}
