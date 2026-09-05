#include <cassert>
#include <iostream>

#include "Prompt.hpp"
#include "Workspace.hpp"

void prompt_smoke_test() {
    Prompt p("Reviewer", "Review this requirement.");
    assert(p.title() == "Reviewer");
    assert(!p.empty());
}

void workspace_smoke_test() {
    Workspace w("Demo");
    w.addPrompt(Prompt("Reviewer", "Review this requirement."));
    assert(w.promptCount() == 1);
}

void workspace_access_test() {
    Workspace workspace("test_space");
    workspace.addDocument(Document("title", "contents"));
    workspace.addPrompt(Prompt("prompt", "contents"));
    workspace.addMessage(Message(MessageRole::System, "contents"));

    assert(workspace.documentCount() == 1);
    assert(workspace.promptCount() == 1);
    assert(workspace.messageCount() == 1);
    assert(workspace.documentAt(0).title() == "title");
    assert(workspace.promptAt(0).title() == "prompt");
    assert(workspace.messageAt(0).role() == MessageRole::System);
    
    // should throw exception
    bool fail = false;
    try {
        workspace.documentAt(1);
    }
    catch(const std::out_of_range& e) {
        fail = true;
    }
    
    assert(fail == true);
}

void workspace_copy_move_test() {
    Workspace workspace1("test_space");
    Workspace workspace2 = workspace1;
    
    assert(workspace1 == workspace2);

    Workspace workspace3 = std::move(workspace1);
    workspace1.setName("test_space_two");

    assert(workspace1 != workspace2);
    assert(workspace2 == workspace3);   
}

void workspace_const_correctness_test() {
    Workspace workspace1("test_space");
    workspace1.addDocument(Document("title", "contents"));
    workspace1.addPrompt(Prompt("prompt", "contents"));
    workspace1.addMessage(Message(MessageRole::System, "contents"));
    const Workspace workspace2 = workspace1;

    assert(workspace1 == workspace2);

    auto& doc_ref = workspace1.documentAt(0);
    doc_ref.setTitle("doc_ref");

    assert(workspace1 != workspace2);
}


void document_initialize_test() {
    Document doc("title", "contents");
    assert(doc.title() == "title");
    assert(doc.contents() == "contents");
}

void document_equality_empty_test() {
    Document doc1("title", "contents");
    Document doc2("title", "contents");
    assert((doc1 == doc2) == true);
    assert((doc1 != doc2) == false);
    assert(doc1.empty() == false);
}

void document_load_success_test() {
    Document doc;
    assert(doc.load("../text/sample.txt") == true);
}

void document_load_empty() {
    Document doc;
    doc.load("../text/empty.txt");
    assert(doc.empty() == true);
}

void document_load_fail_test() {
    Document doc;
    assert(doc.load("sample.txt") == false);
    assert(doc.title().empty() == true);
    assert(doc.sourcePath().empty() == true);
    assert(doc.empty() == true);
}

int main() {
    prompt_smoke_test();
    workspace_smoke_test();

    workspace_access_test();
    workspace_copy_move_test();
    workspace_const_correctness_test();
    
    document_initialize_test();
    document_equality_empty_test();
    document_load_success_test();
    document_load_empty();
    document_load_fail_test();

    std::cout << "M0 tests passed\n";
    return 0;
}
