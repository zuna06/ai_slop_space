Student Name: Tanish Ambala
Student ID:
Project / Milestone: Milestone 4

Part 1: Summary of AI Tools Used

            Tools Used: ChatGPT free version

Part 2: Detailed Log of Substantial AI Assistance

            What the AI did:
            
            Boilerplate for some functions, while I write the actual code myself and then I used AI to check if my functions are correct for verification then I build and test. Used it to generate the CorpusBuilder::~CorpusBuilder function.
            
            
            What I fixed + what the AI got wrong
            
            Sometimes it would not find the error, and I would find it after building and testing, so I make sure to both have AI check it then do it myself as well. I debug the entire program myself after an AI code check, this verifies it really does work, and maybe pick up an error that the AI missed.

            The CorpusBuilder destructor it generated was blocking, it would set the cancel member function to true and immediately call join on the worker, but
            this is blocking since join blocks until the thread joins. So instead I have 2 for loops, the first one sets all worker->cancel members to true,
            then checks if the worker is joinable with joinable() then I call join() on them.

Part 3: Student Verification Statement

I affirm that I have thoroughly reviewed all AI-assisted code listed above, that I fully understand how it operates within the broader application architecture, and that I am able to modify or debug it independently.
