# M4 Decision Record

1.

GUI thread: MainWindow, all widgets, SearchResultModel, CorpusBuilder, and IPCServer::start and stop are on the GUI thread.

MainWIndow owns the Workspace, the current ProcessingCore, corpus_state_, workspace_id_, and ipc_server_. corpus_builder_ is a Qt child of MainWindow. CorpusBuilder owns its BuildJobs through std::vector<std::unique_ptr<BuildJob>> and each BuildJob owns its std::thread, its std::future<ProcessingCore> and its cancel flag. A worker gets a moved copy of the Workspace, so it never touches MainWindows\ No thread gets detached, they are always joined, the CorpusBUilder::take function joins a worker before reading the result, and the destructor sets every cancel flag and joins each worker, and the ipc server stop function joins the ipc thread before the socket file is removed. 

2.

CorpusBuilder::start records the id the build was started with and add document and reset increment workspace_id_. When a worker finishes it posts job_done and CorpusBuilder emits build_finished and MainWindow::build_finished calls take(job_id) which joins the worker. If result workspace_id doesnt equal the workspace_id_ the result is stale and is discarded without changing any state. Otherwise an errror moves to Failed and shows a build failed message, and success moves the core into processing_core_ and staten goes to Current. An older build that finishes after a newer one has an old ID