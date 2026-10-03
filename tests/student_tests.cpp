#include "aiws/chunking_strategy.hpp"
#include "aiws/retrieval_strategy.hpp"
#include "aiws/context_strategy.hpp"
#include "aiws/processing_core.hpp"
#include "aiws/text_processor.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <memory>

// Student-written M2 tests
//
// Add your own tests to this file. Your tests are part of the submitted work
// and are evaluated under Student-Written Testing & Validation.
//
// Do not modify tests/public_tests.cpp.
//
// Your tests should exercise important M2 behavior beyond the supplied public
// tests. Consider default compatibility, custom strategies, runtime dispatch,
// invalid configuration, ownership/move behavior, and component interactions.

namespace {
    int failures = 0;
    void check(bool ok, const char* name) {
        if (!ok) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
    }

    class OneChunk final : public aiws::ChunkingStrategy {
    public:
        std::vector<aiws::Chunk> chunk(const aiws::Document& d, std::size_t order) const override {
            return {{d.id()+"#custom", d.id(), order, 0, "custom chunk", 2, 0, d.text().size()}};
        }
    };
    class FirstOnly final : public aiws::RetrievalStrategy {
    public:
        std::vector<aiws::SearchResult> search(const std::string&, int k,
            const std::vector<aiws::Chunk>& chunks, const aiws::CorpusIndex&) const override {
            if (k <= 0 || chunks.empty()) return {};
            const auto& c = chunks.front();
            return {{c.id, c.document_id, c.sequence, c.text, 99.0, 1}};
        }
    };
    class PrefixContext final : public aiws::ContextStrategy {
    public:
        std::vector<aiws::ContextItem> build(const std::vector<aiws::SearchResult>& ranked,
                                            std::size_t budget) const override {
            if (ranked.empty() || budget == 0) return {};
            const auto& r = ranked.front();
            return {{r.chunk_id, r.document_id, r.chunk_sequence, "custom", 1, r.score, true}};
        }
    };
}


int main() {
    using namespace aiws;

    check(ProcessingCore::normalize("Hello, WORLD!") == "hello world", "normalize basic");
    check(ProcessingCore::normalize("...").empty(), "normalize punctuation");

    Workspace workspace;
    workspace.add_document(Document{"d1", "one", "alpha alpha beta"});
    workspace.add_document(Document{"d2", "two", "gamma alpha"});

    ProcessingCore core;
    core.rebuild(workspace);
    check(core.chunk_count() == 2, "chunks count");
    check(core.document_frequency("alpha") == 2, "alpha document frequency");
    check(core.term_frequency("alpha", "d1#0") == 2, "alpha term frequency");

    auto res = core.search("alpha beta", 10);
    check(!res.empty() && res[0].document_id == "d1", "core search");
    
    // testing all strategy chunks
    bool threw = false;
    try {
        ProcessingCore bad(nullptr, std::make_unique<FirstOnly>(), std::make_unique<PrefixContext>());
    } catch (const std::invalid_argument&) { threw = true; }
    catch (...) {}
    check(threw, "null chunker throw");

    threw = false;
    try {
        ProcessingCore bad(std::make_unique<OneChunk>(), nullptr, std::make_unique<PrefixContext>());
    } catch (const std::invalid_argument&) { threw = true; }
    catch (...) {}
    check(threw, "null retrieval throw");

    threw = false;
    try {
        ProcessingCore bad(std::make_unique<OneChunk>(), std::make_unique<FirstOnly>(), nullptr);
    } catch (const std::invalid_argument&) { threw = true; }
    catch (...) {}
    check(threw, "null context throw");


    ProcessingCore custom(std::make_unique<OneChunk>(),
                              std::make_unique<FirstOnly>(),
                              std::make_unique<PrefixContext>());
    custom.rebuild(workspace);
    check(custom.chunk_count() == 2, "chunker ran");
    check(custom.chunks()[0].id == "d1#custom", "chunk id");
    check(custom.chunks()[0].text == "custom chunk", "chunk test");

    auto custom_search = custom.search("whatever", 1);
    check(custom_search.size() == 1 && custom_search[0].score == 99.0, "retrieval score");
    auto cbuild = custom.build_context("whatever", 5, 100 );
    check(cbuild.size() == 1 && cbuild[0].text == "custom", "context text");

    Workspace ws2;

    ws2.add_document(Document{"x", "X", "new doc"});
    custom.rebuild(ws2);
    check(custom.chunk_count() == 1, "custom rebuild");
    check(custom.chunks()[0].document_id == "x", "custom rebuild id check");

    Workspace dup_ws;
    dup_ws.add_document(Document{"same", "a", "aaa"});
    dup_ws.add_document(Document{"same", "a", "aaa"});
    threw = false;
    try {
        custom.rebuild(dup_ws);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "dup id throws with custom");
    check(custom.chunk_count() == 1, "failed rebuild");

    ProcessingCore moved(std::move(custom));
    check(moved.chunk_count() == 1, "move constructor works");

    ProcessingCore assigned;
    assigned = std::move(moved);
    check(assigned.chunk_count() == 1, "move assignment works");

    threw = false;
    try {
        core.search("hi", -1);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "negative k throws");
    check(core.search("hi", 0).empty(), "k empty");
    check(core.build_context("hi", 10, 0).empty(), "budget empty");

    Workspace empty;
    ProcessingCore empty_core;
    empty_core.rebuild(empty);
    check(empty_core.chunk_count() == 0, "empty chunks");

    if (failures) {
        std::cerr << failures << " tests failed\n";
        return 1;
    }

    std::cout << "All tests passed";

    return 0;
}
