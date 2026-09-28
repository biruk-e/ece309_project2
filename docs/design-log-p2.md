# Design Log — Project 2

## Growth factor and amortized cost

My `Conversation` stores messages in a dynamically allocated array, with separate members for its pointer, size, and capacity. Size counts conversation messages; capacity counts allocated slots. A new conversation starts with a null pointer and zero size and capacity. The first append allocates one slot. Later, when the array is full, capacity doubles. This produces capacities of 1, 2, 4, 8, and so on. Appending preserves message order, including a system message placed first by the harness.

Doubling makes append amortized O(1) when counting message operations. For n appends, let C be the final capacity. For n greater than zero, n <= C < 2n. Growth copies 1 + 2 + 4 + ... + C/2 = C - 1 existing messages, which is less than 2n. Adding the n assignments that insert incoming messages still gives O(n) total work. Default construction of allocated slots also forms a geometric sum, 1 + 2 + ... + C = 2C - 1, so it does not change the bound. One append that grows the array takes O(n) message operations, but their average cost is O(1).

This analysis counts message operations rather than individual characters. Copying a message's string can additionally depend on its length. Doubling trades unused capacity for fewer allocations. The growth test checks the first nine capacities against 1, 2, 4, 4, 8, 8, 8, 8, 16 and verifies preserved contents.

## Rule of Five evidence

`Conversation` owns its array and releases it with `delete[]`. Deleting a null pointer is safe, so destruction also works for empty and moved-from objects. Individual `Message` objects use `std::string` to manage their text and need no custom ownership functions.

The copy constructor allocates independent storage and copies each occupied slot. If copying throws, it releases the temporary array and rethrows. This cleanup matters because the destructor of an incompletely constructed `Conversation` will not run. Tests verify distinct array addresses, matching messages, and continued validity after the copy is destroyed.

Copy assignment uses copy-and-swap. It first constructs an independent temporary, then swaps the pointer, size, and capacity. The temporary's destructor releases the destination's previous array. If creating the copy fails, the destination remains unchanged. Self-assignment is handled explicitly.

The move constructor transfers the pointer and counts without copying messages. Move assignment first checks for self-move and releases the destination's old array. Both leave the source with a null pointer and zero size and capacity. Tests check the transferred address and reuse moved-from objects. Both operations are `noexcept` because they do not allocate or copy strings. During append, failed copying into replacement storage also triggers cleanup; size increases only after the incoming message is stored successfully.

## Sentinel scanner: bounded pending_ proof

Let m be the nonempty sentinel's length. The constructor rejects an empty sentinel. Each `feed()` combines the previous pending suffix with the incoming chunk and searches for a complete sentinel. A match releases only preceding text, clears pending storage, and records that scanning has stopped. Later calls emit nothing.

Without a match, the scanner retains min(m - 1, combined length) trailing characters. Therefore pending length is always between zero and m - 1 after each call. Initially it is zero, and every update either clears it or assigns a suffix satisfying this bound. Any incomplete sentinel crossing a chunk boundary contains at most m - 1 characters from the earlier input, so retaining that suffix is sufficient. `flush()` releases unfinished text when the stream ends without a match.

Persistent pending storage is O(m), constant for the fixed sentinel. Temporary combined text and returned output additionally depend on chunk size; they are not constant-size buffers. Tests cover all two-chunk split points, one-character chunks, false matches, and flushing. A 4 MiB repeating partial-sentinel stream checks the pending bound after every byte and verifies that all text is preserved.

## What I would change differently

I would extract the duplicated allocation-and-copy cleanup logic from append and the copy constructor into a private helper. That would reduce maintenance work while preserving ownership behavior. The completed CMake test run also checks harness stopping, EOF, and transcript replay. All twelve test groups passed with the configured sanitizers and no reported runtime errors. These results cover the exercised cases, rather than proving correctness for every possible input.
