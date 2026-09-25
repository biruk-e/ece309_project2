#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "core/conversation.h"

int main() {
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