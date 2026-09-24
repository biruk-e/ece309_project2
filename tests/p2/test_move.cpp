#include <cassert>
#include <iostream>
#include <utility>

#include "core/conversation.h"

int main() {
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