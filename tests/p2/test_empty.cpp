#include <cassert>
#include <iostream>
#include "core/conversation.h"

int main() {
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