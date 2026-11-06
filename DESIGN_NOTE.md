# M4 Design Note

Due Wed Oct 28, 11:59 pm. See specification Section 7.

The idea is that each build should run on its own thread, using std::thread, using a copy of the Workspace and a new ProcessingCore. The GUI thread will receive the finished corpus, then accept it only if its workspace id still matches the current one.

Components

MainWindow owns the workspace, current ProcessingCore, and the corpus state, it's in charge of deciding whether a finished build is accepeted or rejected. The idea is to have another member called workspace_id that is of type uint64_t. This member will be the version number for the workspace. When the workspace changes it will increment by one. When a build is finished MainWindow checks the current number with the finished build workspace_id to see if they match or not. If they are equal then the corpus matches the workspaces and it is accepted, but if they are not then the corpus has old data and it is rejected. The Widgets and SearchResultModel can stay the same from M3.

New Components

CorpusBuilder, this object is to start a build and store the running builds data. It should track all builds including their workspace_id. When a job is finished it will join the thread and notify that the build is finished.

BuildJob, this object holds the std::thread, std::future<ProcessingCore>, and which workspace_id the build was started for. 

There will be a separate function that runs in the background that's passed to std::thread when a build is started. The job of this is for it to produce a corpus and then tell the GUI that it is done. It creates a ProcessingCore and called rebuild on it, and when the build is complete it tells the GUI that it is done with Qt functions. It doesn't change the widgets directly.

Another component is DelayingChunker, a new ChunkStrategy. This is how the--build-delay-ms flag is implemented. The rebuild function calls the chunker once per document, so a document delay can be implemented here with a custom ChunkingStrategy. DelayingChunker wraps the Chunker in a function that waits for the specified time then calls it.

Threads

MainWindow, the widgets, and SearcHresultModel run on the GUI thread
CorpusBuilder Run on the gui thread
BuildJob is created and deleted on the GUI thread, only the cancel flag is read by the worker thread
The background build functions run on the worker thread for that build
DelayingChunker and the ProcessingCore being built run on the worker thread inside rebuild

Corpus States

The corpus states are not built, building, current, and failed. The not built state means that the corpus doesn't match the current Workspace, building is for when a build is started and is running, current is when the processing core is built from the current Workspace, and failed is for when the most recent build throws an error.

Events That Move Between The States

Build Corpus clicked -> building state
Build succeeds -> notifies GUI
Build failed -> failed state -> notifies GUI and shows error
Add document -> not built -> workspace_id increases
Exit -> cancels all builds and joins them and application closes

Sequences That Threaten R1 To R4

R1

Long and/or multiple rebuilds causing possible slowdowns or freezes -> this is handled because the build runs on a thread in the background and the GUI doesn't wait on it, so the user can still use the app.

R2

Build -> Add document -> build finishes: the build uses old Workspace and when it finishes the workspace_id doesn't match so the corpus is thrown away
Build -> reset -> finishes: the reset increases the workspace_id so the old corpus getse thrown away
Build A -> add document -> build B -> A finishes first: two builds are running and A finishes first while B matches the most recent workspace_id. If A finishes after B it would overwrite B's corpus which is the correct one, but there's an id check to prevent this
Search, Inspect, or Context during a build are disabled unless the state is current to ensure operations are done on the most current corpus data.

R3

Duplicate IDs make rebuild throw, the exception happens on the background function on the thread. To handle this, you can store the exception and throw it on the GUI to show the error and enter the failed state.

R4

To exit during a build, there's a cancel flag in BuildJob to stop the worker when needed and join it back to return, then close the application.
Multiple rebuilds -> Jobs are handled properly by joining them and deleting them when completed so threads don't pile up.

Walkthroughs

Add document -> a.txt and id is 1 -> user clicks build corpus and the job has id 1
-> another document is added -> b.txt and id is now 2 -> the state becomes not built and the old results are cleared
-> the first job finishes and notifies the gui -> CorpusBuilder joins the first job and MainWindow sees the result and discards the corpus since the ids dont match (1 != 2)

Exit during a build
A build runs with a long build delay per document -> the user clicks Exit then yes
-> the application quits and the mainWindow is destroyed
-> the CorpusBuilder destrcutor sets the jobs cancel flag and calls join
-> the worker function stops the build and returns 
-> join returns and the application closes
the application doesn't hang or freeze and the cancel flag is set for all build jobs

Ownership Path

A worker thread is created and constructs a ProcessingCore with a custom ChunkingStrategy and calls rebuild. Owned by the worker function. It gets moved with std::move when the build is complete, owned by the BuildJob struct which is owned by CorpusBuilder. The corpus is received by the gui thread when the thread is joined and the core is moved. The owner is MainWindow which holds the result. It gets accepted if the ids match and the owner is MainWindow and the previous corpus is destroyed after the core is moved. If the ids don't match the build is rejected and the corpus is destroyed. An accepted corpus is destroyed if the next accepted buiild replaces it, and when Reset clears it, or when MainWindow is destroyed. std::move is used to transfer ownership and the corpus is never copied and no raw pointers are used.

Rejected Alternative

I thought of creating a new corpus, I was originally planning on rebuilding the one the MainWindow uses. I was also considering keeping members in one main struct, similar to how MainWindow holds its members. However, if I did this the GUI would have to keep checking to see if the worker is done or not, and 2 rebuilds running at the same time would result in a garbage output or crash since they are doing work on the same data. The above design works better since it keeps members separate and prevents objects from interacting unless they need to, jobs are kept separate and an id ensures checks before the corpus is updated or destroyed. 