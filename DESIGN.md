# M0 Design and Understanding Note

Answer briefly in your own words. This is not intended to be a long report.

1. What responsibility belongs to `Workspace`, and what responsibilities belong to `Document`, `Prompt`, and `Message` instead?
*`Workspace` is the top level class that holds and manages `Document`, `Prompt`, and `Message` objects in corresponding vectors. `Document` stores file data, `Prompt` holds the message to be passed to an llm, and `Message` handles generic messages from roles `System`, `User`, `Assistant`.*

2. Why are the collections inside `Workspace` private? Explain the purpose of the const and non-const `At` overloads.
*For abstraction, which makes sure the user can only access and modify the data through methods (member functions) the program has defined. The purpose of the const and non const overloads maintain const correctness for const or non const `Workspace` objects.*

3. Explain one meaningful test you added. What behavior does it check, and what implementation error could it catch?
*I added a test to check that `Document::empty` returns true when an empty file is loaded in. An implementation error it could catch is if the stream leaves a newline or garbage data in `contents_`.*

4. Describe one implementation decision that you verified, tested, or revised before submitting your work.
*For the `Document::load` function I used `std::ifstream` and `std::ostringstream` for streaming the specificed files data into `contents_`. I was originally using `std::istringstream` since I used it previously for a different program, but `std::ifstream` and `std::ostringstream` made it simple and easy to stream data without using a loop*

5. If generative AI was used, disclose it as required by course policy. If no generative AI was used, state that. The disclosure itself is not used as proof of authorship or understanding.

> *AI Use:*
> -   *Syntax checking*
> -   *Debugging*
> -   *Zero code generation*