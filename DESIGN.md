# M2 Technical & Design Understanding

This file is an assessed technical-understanding artifact, not ordinary project documentation.
Answer all four questions using your own submitted implementation. Concise answers are acceptable when they are technically correct and specific.

Generic descriptions of C++ concepts or restatements of the assignment that do not identify and explain corresponding parts of your code will receive limited credit.

## 1. Polymorphism and dynamic dispatch - 1.5 points

Identify one place in your M2 implementation where runtime polymorphism occurs. Name the relevant base interface, derived implementation, and `ProcessingCore` function involved. Trace the call from `ProcessingCore` to the selected strategy implementation and explain why the derived implementation is invoked.

Then explain what would change if the relevant operation were not declared `virtual`.

ProcessingCore::rebuild calls impl_->chunker->chunk(doc, order).
The base interface is ChunkingStrategy which has chunk has a pure virtual function, the derived implementation is Chunker. The core stores a unique pointer with type ChunkingStrategy, when it calls chunk it finds the corresponding function through the virtual lookup table since the function is declared virtual. If chunk was not declared virtual then it would call the base class function and return incorrect information, in this case it wuld return an empty vector no matter what.

## 2. Ownership and lifetime - 1.5 points

Identify where one of the strategy objects is created, where ownership is transferred, and which object ultimately owns it. Explain how `std::unique_ptr` represents that ownership relationship and when the strategy object is destroyed.

Also explain why `ProcessingCore` is move-only and why the strategy base classes require virtual destructors.

When a unique pointer is created to a chunker type and it is passed to the ProcessingCore's constructor, the core moves it into impl_->chunker which is of type ChunkingStrategy, and core ultimately owns it since impl, also a unique ptr, is owned by ProcessingCore. The object gets destroyed when the core is also destroyed, because core's destructor destroys impl_ which destroys Impl, then the unique ptr which deletes the chunker. 

ProcessingCore is move only because it has unique ptrs as its members. Copying would mean needing to copy the unique ptrs which is not allowed and having 2 cores that have the same strategy objects would break the ownership design for this project. The base classes needs virtual destructors because the destruction of the objects are done from a base class pointer. Without it being virtual only the base class's destructor would run and not the derived class's destructor. 

## 3. Architecture, extensibility, and M1 compatibility - 1.5 points

Explain one specific architectural decision in your M2 implementation that makes the processing system extensible while preserving M1 behavior.

Identify the classes or interfaces involved and explain both:
- how the default configuration preserves M1 behavior; and
- how a different implementation can be substituted without changing the normal `ProcessingCore` API.

Include one plausible design alternative and explain why the M2 design is preferable for this milestone. The alternative does not need to be something you actually implemented.

They key decision was separating the strategy interfaces and having ProcessingCore interact through the base interfaces. The default constructors preserve m1 behavior
by creating the m1 classes then storing them as a unique pointer to the base types in ProcessingCore. The behavior is the same, but the way to interact with the objects is 
changed.

A different implementation would be able to send a custom derived class to the 
ProcessingCore's constructor with custom function behavior for chunk, search, or build. This doesn't change any of the base class's functions and allows the caller to
add custom functionality.

An alternative would be using function pointers or callbacks instead of derived classes. You could pass them to ProcessingaCore and it could call them directly. The
M2 design is preferred because it keeps the configuration with the class together, you're able to call chunk() on Chunker directly. With function pointers it would be separate from the state. With the m2 design it's simple and easy to just create a custom strategy when needed and pass it directly without having to worry about state and function pointer.s 

## 4. Testing and defect reasoning - 1.5 points

Select one meaningful test from your `tests/student_tests.cpp`.

Explain:
- what M2 requirement the test validates;
- what specific implementation defect the test could detect; and
- why your test provides useful evidence beyond simply rerunning the supplied public tests.

If your test uses a custom strategy, explain how its observable behavior demonstrates that `ProcessingCore` is actually using runtime substitution.

One test I made creates a ProcessingCore rebuilds a workspace then checks the id of the first chunk to make sure that the core actually used the ChunkingStrategy during the rebuild. The defect it tests for is if the strategy class still calls to the Chunker class internally. If the rebuild calls a Chunker object then the id would have been d1#0 instead of d1#custom is what my test checks since OneChunk creates a
chunk id that another default chunker would not create.

The reason why this goes beyond the public test is that my test verifies that after a rebuild with custom strategies, another rebuild with another workspace still uses the same custom chunker. 