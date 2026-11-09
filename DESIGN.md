# M4 Decision Record

1.

GUI thread: MainWindow, all widgets, SearchResultModel, CorpusBuilder, and IPCServer::start and stop are on the GUI thread.

MainWIndow owns the Workspace, the current ProcessingCore, corpus_state_, workspace_id_, and ipc_server_. corpus_builder_ is a Qt child of MainWindow. CorpusBuilder owns its BuildJobs through std::vector<std::unique_ptr<BuildJob>> and each BuildJob owns its std::thread, its std::future<ProcessingCore> and its cancel flag. A worker gets a moved copy of the Workspace, so it never touches MainWindows\ No thread gets detached, they are always joined, the CorpusBUilder::take function joins a worker before reading the result, and the destructor sets every cancel flag and joins each worker, and the ipc server stop function joins the ipc thread before the socket file is removed. 

2.

CorpusBuilder::start records the id the build was started with and add document and reset increment workspace_id_. When a worker finishes it posts job_done and CorpusBuilder emits build_finished and MainWindow::build_finished calls take(job_id) which joins the worker. If result workspace_id doesnt equal the workspace_id_ the result is stale and is discarded without changing any state. Otherwise an errror moves to Failed and shows a build failed message, and success moves the core into processing_core_ and staten goes to Current. An older build that finishes after a newer one has an old id, so it can never replace the newer corpus.

3. changes since checkpoint

- CorpusBuilder doesnt decide anything abour results, it only reports thta a job has finished, and MainWindow decides what to do with the result. This makes sure that there is only one place where the result is judged. The exception is carried through promise.set_exception and rethrown on the GUI thread, I added a build cancelled exception which is thrown by DelayingChunker when the cancel flag is set, so exit stops a build within about 10 ms. Build Corpus is disabled when the state is building, after add document or reset it is enabled again so a newer build can start while an old one is still running.
Lastly, I added the destructor for MainWindow for CR 1.

4. CR1 Impact

I had to add and change MainWindow::start_ipc, ~MainWindow, main.cpp, and CMakeLists.txt. main.cpp handles parsing the ipc socket flag. I didn't really have to change the corpus or build functions or structure. A remote add gets queued to the gui thread and calls add_document_file which is the same function the add document button uses, so it already increments workspace_id_. IPCServer::handle_client checks that the file is a regular readable file with stat and access and replies before queuing the add, so the reply never waits for the gui thread.

Message format:

For a request the first 4 bytes is the length and the rest of the bytes are the actual payload which consists of "add " + the absolute file path. Since the length is sent first, paths can contain spaces, non ascii characters, or newlines, and the server keeps reading until it has all bytes, so a request that gets split across writes still works. For the reply, the first byte is the status, 0 is accepted and 1 is rejected. AFter a rejection the rest of the bytes until the server closes the connection are an error message. aiws_send turns a relative file into an absolute path using its own working directory, and exists 0 when accepted, 1 when rejected, and 2 when the server can't be reached within 2 seconds or communicastion fails within 5 seconds. For decisions, the requests are handled one at a time one the single ipc thread, since each one only takes a stat and a short reply.

5.

AIWS_FAULT_ACCEPT_STALE, in MainWindow::build_finished, removes the workspace_id check and AIWS_FAULT_LOSE_FAILURE returns before a failure is shown, so the state stays building.