#include <cassert>
#include <iostream>
#include "core/conversation.h"

int main() {
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