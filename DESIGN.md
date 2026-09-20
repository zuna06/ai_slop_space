# M1 DESIGN.md

Replace this template with your own concise engineering explanation.

## 1. System structure
Describe the major responsibilities in your M1 subsystem and how they interact.

The different components are TextProcessor, Chunker, CorpusIndex, RetrievalENgine, ContextBuilder, and ProcessingCore. The TextProcessor is in charge of parsing text for normalization, tokenization, and extracting terms. It cleans up text inputs so they're easier to process and handle. Chunker takes a document that has been parsed (tokenized) and splits it into 120 token chunks. CorpusIndex maps terms to chunks that have them, it stores a posting (a chunk index, and frequency) for each unique term. It allows you to search for how often a term appears in a specific chunk or multiple chunks. RetrievalEngine is in charge of scoriong the chunks using the formula given in the M1 rubric. It also sorts based on scores and has its own tiebreaking method. ContextBuilder walks the scored results and packs them based on a certain token budget. If there are 10 chunks with 100 tokens each and the token budget is 550, then only the first 5 chunks and the first 50 tokens from the 6th chunk are saved into the context vector. ProcessingCore is the the top level component. It basically controls the rest of the components and you're able to do things like search, query, check for term or document frequencies, and more because the ProcessingCore functions are able to use each component. 

## 2. Design decisions
Explain the principal data structures, interfaces, and ownership decisions in your implementation and why you selected them.
Other than std::vector for chunks I mainly used std::unordered_map.
The vectors were really useful for saving the insertion order, but with unordered_map I had pairs of data which was especially helpful for the postings and chunk id lookup. unorderd_set was also useful for preventing duplicate chunk indexes or document ids, and I didn't care about the order. For ownernship in the Impl struct, I had all the components there, including a Chunks vector. Having one struct to hold that was easier since it keeps the main components in one place. 

## 3. Correctness and consistency
Identify the important invariants or failure cases your design must preserve and explain how your design addresses them.
The most important invariant is the normalization. Processing the same text should have the same normalized tokens if it goes through either the document route or query route for normalization. They both go through the same TextProcessor's normalization path. If they were different then it would be redudant and might have caused an issue if there was a small error in one implementation. That would end up being seen in a query or search and the rest of the component functions as well. Some other things that are good to preserve is checking for duplicate document ids, and preserving states if the inpuit is invalid. For example, if a vector is empty when it shouldnt be or when a number is not positive when it should be, the function stops and doesn't change the state of any of the data. At the same time if there was an issue in the middle of the code the data would be preserved, because I used temporary structures to operate on and I only moved or copied them to the main state (Impl for most cases) at the end, making sure the state was only changed when the process completed successfully.

## 4. Testing strategy
Explain what your tests cover and which risks or boundaries you considered most important.
They cover text normalization, corpus queries, search ranking, token budget, verifying chunking, and error handling. It goes through ProcessingCore and other components directly to make sure the routes are correct and there are no errors along the way. The highest risks are the normalization because if it was off then the rest of the components operate on faulty data which would return faulty output for scoring, queries, frequencies, and more. 

## 5. Alternatives considered
Discuss at least two plausible design alternatives and why you did not choose them.

I was considering just having the text in chunks instead of normalized and I did start implementing them. I scrapped that idea because I realized I would have to normalize them anyways during indexing and searching. It would have been redundant and have a higher runtime.
Another thing I considered was using unordered_map instead of std::vector for the retrieval engine. I'm just a little more used to unordered_maps and I thought it could be an option because of its O(1) insertion and lookup time. However, I decided to use std::vector instead because I would need to sort after and std::sort works on vector not unordered map. It was easier to use vector and sort lambda. With the map I would have to copy to a vector or an array anyways to sort it which would just add an extra step. 
