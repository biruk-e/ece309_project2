#include <cassert>
#include <iostream>
#include "core/conversation.h"

int main() {
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