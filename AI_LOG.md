Student Name: Tanish Ambala
Student ID:
Project / Milestone: Milestone 4

Part 1: Summary of AI Tools Used

            Tools Used: ChatGPT free version

Part 2: Detailed Log of Substantial AI Assistance

            What the AI did:
            
            Boilerplate for some functions, while I write the actual code myself and then I used AI to check if my functions are correct for verification then I build and test. Used it to generate the CorpusBuilder::~CorpusBuilder function.

            Used AI to help me write tests, it gave me ideas on edge cases and formatting. I had an issue with trying to use the given data/public/ files for my tests, so I used AI to fix it, and it used Qt's specific types and specific functions which fixed my issue. It made the helper functions which I reformatted and used in my tests that I wrote.
            
            It also came up with the QSignalSpy idea which was helpful for testing when a build has finished since the finished.count() goes up by one. 
            
            What I fixed + what the AI got wrong
            
            Sometimes it would not find the error, and I would find it after building and testing, so I make sure to both have AI check it then do it myself as well. I debug the entire program myself after an AI code check, this verifies it really does work, and maybe pick up an error that the AI missed.

            The CorpusBuilder destructor it generated was blocking, it would set the cancel member function to true and immediately call join on the worker, but
            this is blocking since join blocks until the thread joins. So instead I have 2 for loops, the first one sets all worker->cancel members to true,
            then checks if the worker is joinable with joinable() then I call join() on them.

            CR-1 PAIR PROGRAMMING SECTION
            My partner did not respond to my emails or the TA's email so I did this part by myself, using the free version of ChatGPT to assist me. I used it to help outline the code and some functions, and I used it to check my ideas against the requirements.
            In the MainWindow::start_ipc function I used it to clean up my code, originally I was using 2 functions extra for the invokeMethod and add_document_file request, because of a method I saw online, but the AI suggested using a lambda instead so I used that. For aiws_send it gave me suggested on error checking and status checking and what types of data structures to use. The main thing the AI did was giving me ideas or hints for what types of structs I would need, or what headers and functions I would need to implement. It also suggested small tweaks, like in IPCServer::stop() function I wasn't aware of the shutdown() function but it recommended to add that before closing the fd.

            At the end of CR1 once I had finally completed everything, I used AI to check my code and refactor it while keeping the application state and my code's original functions.

            It made several changes, like

Part 3: Student Verification Statement

I affirm that I have thoroughly reviewed all AI-assisted code listed above, that I fully understand how it operates within the broader application architecture, and that I am able to modify or debug it independently.
