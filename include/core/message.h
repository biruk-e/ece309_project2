#pragma once // header guard

#include <string>

enum class Role { System, User, Assistant };

class Message {
public:
    // Default-constructs an empty System message with empty content.
    // Needed so Conversation can allocate raw array slots before
    // append() fills them in.

    // If no arguments are passed, role is system and content is an empty string
    Message() : role_(Role::System), content_("") {}

    // Initializes private members with the supplied role and content
    Message(Role role, std::string content) : role_(role), content_(content) {}

    // Returns who sent the message
    Role role() const noexcept {
        return role_;
    }

    // Returns the message
    const std::string& content() const noexcept {
        return content_;
    }

private:
    Role        role_;
    std::string content_;
};