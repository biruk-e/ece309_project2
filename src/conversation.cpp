#include "core/conversation.h"

#include <limits>
#include <stdexcept>

#include <utility>

Conversation::Conversation() {} // Starting values are already supplied in the header

Conversation::~Conversation() {
    delete[] data_;
}

std::size_t Conversation::size() const noexcept {
    return size_;
}

const Message* Conversation::begin() const noexcept {
    return data_;
}

const Message* Conversation::end() const noexcept {
    if (size_ == 0) {
        return data_;
    }

    return data_ + size_;
}

void Conversation::append(Message m) {
    // Allocate more space if every available slot is occupied
    if (size_ == capacity_) {
        std::size_t new_capacity;

        if (capacity_ == 0) {
            new_capacity = 1;
        } else {
            // Prevent overflow when doubling capacity
            if (capacity_ > std::numeric_limits<std::size_t>::max() / 2) {
                throw std::length_error("Conversation is too large");
            }

            new_capacity = capacity_ * 2;
        }

        Message* new_data = new Message[new_capacity];

        try {
            for (std::size_t i = 0; i < size_; ++i) {
                new_data[i] = data_[i];
            }
        } catch (...) {
            delete[] new_data;
            throw;
        }

        // Replace the old array after copying succeeds
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }

    data_[size_] = m;
    ++size_;
}

const Message& Conversation::at(std::size_t i) const {
    if (i >= size_) {
        throw std::out_of_range("Conversation index out of range");
    }

    return data_[i];
}

Conversation::Conversation(const Conversation& other) {
    // The default member initializers already make this object empty
    if (other.size_ == 0) {
        return;
    }

    Message* new_data = new Message[other.capacity_];

    try {
        for (std::size_t i = 0; i < other.size_; ++i) {
            new_data[i] = other.data_[i];
        }
    } catch (...) {
        delete[] new_data;
        throw;
    }

    data_ = new_data;
    size_ = other.size_;
    capacity_ = other.capacity_;
}

Conversation& Conversation::operator=(const Conversation& other) {
    // Assigning an object to itself requires no changes
    if (this == &other) {
        return *this;
    }

    // Use our copy constructor to create an independent copy
    Conversation temporary(other);

    // Exchange this object's storage with the temporary's storage
    std::swap(data_, temporary.data_);
    std::swap(size_, temporary.size_);
    std::swap(capacity_, temporary.capacity_);

    return *this;
}

Conversation::Conversation(Conversation&& other) noexcept
    : data_(other.data_),
      size_(other.size_),
      capacity_(other.capacity_) {

    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

Conversation& Conversation::operator=(Conversation&& other) noexcept {
    // Protect against moving an object into itself
    if (this == &other) {
        return *this;
    }

    // Release the array this object currently owns.
    delete[] data_;

    // Take ownership of the source's array
    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;

    // Leave the source valid and empty
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;

    return *this;
}