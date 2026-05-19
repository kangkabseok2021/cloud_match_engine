#include <gtest/gtest.h>
#include "../src/MatchEngine.h"
#include <thread>
#include <vector>

// Helper: build a profile from a list of skills
static ProfileData make_profile(const std::string& id,
                                std::initializer_list<std::string> skills) {
    ProfileData p;
    p.id = id;
    for (const auto& s : skills) p.skills.insert(s);
    return p;
}

// Helper: build a query
static MatchQuery make_query(std::initializer_list<std::string> required,
                             std::initializer_list<std::string> preferred = {},
                             int max = 10) {
    MatchQuery q;
    q.required_skills  = std::vector<std::string>(required);
    q.preferred_skills = std::vector<std::string>(preferred);
    q.max_results = max;
    return q;
}

// ── Basic indexing ─────────────────────────────────────────────────────────

TEST(MatchEngineTest, ProfileCountAfterIndexing) {
    MatchEngine e;
    EXPECT_EQ(e.profile_count(), 0u);
    e.index_profile(make_profile("p1", {"C++", "Python"}));
    EXPECT_EQ(e.profile_count(), 1u);
    e.index_profile(make_profile("p2", {"Java"}));
    EXPECT_EQ(e.profile_count(), 2u);
}

TEST(MatchEngineTest, SkillCountTracksUniqueSkills) {
    MatchEngine e;
    e.index_profile(make_profile("p1", {"C++", "Python"}));
    e.index_profile(make_profile("p2", {"C++", "Java"}));  // C++ already known
    EXPECT_EQ(e.skill_count(), 3u);
}

// ── Matching ───────────────────────────────────────────────────────────────

TEST(MatchEngineTest, FindsProfileWithRequiredSkill) {
    MatchEngine e;
    e.index_profile(make_profile("p1", {"C++", "gRPC"}));
    e.index_profile(make_profile("p2", {"Python"}));
    auto results = e.find_matches(make_query({"C++"}));
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].profile_id, "p1");
}

TEST(MatchEngineTest, EmptyQueryReturnsNoResults) {
    MatchEngine e;
    e.index_profile(make_profile("p1", {"C++"}));
    auto results = e.find_matches(make_query({}));
    EXPECT_TRUE(results.empty());
}

TEST(MatchEngineTest, UnknownSkillReturnsNoResults) {
    MatchEngine e;
    e.index_profile(make_profile("p1", {"C++"}));
    auto results = e.find_matches(make_query({"Haskell"}));
    EXPECT_TRUE(results.empty());
}

TEST(MatchEngineTest, MaxResultsRespected) {
    MatchEngine e;
    for (int i = 0; i < 20; ++i)
        e.index_profile(make_profile("p" + std::to_string(i), {"C++"}));
    auto results = e.find_matches(make_query({"C++"}, {}, 5));
    EXPECT_EQ(results.size(), 5u);
}

// ── Ranking ───────────────────────────────────────────────────────────────

TEST(MatchEngineTest, RequiredSkillsRankedHigherThanPreferred) {
    MatchEngine e;
    // p1 matches required C++ only
    // p2 matches preferred Python only → score = 1.0 × idf(Python)
    // With IDF and weights, p1 (required×2.0) should beat p2 (preferred×1.0 each)
    // when the IDF of C++ == IDF of Python == IDF of Rust (one profile each)
    e.index_profile(make_profile("p1", {"C++"}));
    e.index_profile(make_profile("p2", {"Python"}));
    auto results = e.find_matches(make_query({"C++"}, {"Python"}));
    ASSERT_GE(results.size(), 2u);
    EXPECT_EQ(results[0].profile_id, "p1");
}

TEST(MatchEngineTest, IDFPenalizeCommonSkill) {
    MatchEngine e;
    // "C++" appears in all 5 profiles → low IDF
    // "Rust" appears in 1 profile → high IDF
    for (int i = 0; i < 5; ++i)
        e.index_profile(make_profile("p" + std::to_string(i), {"C++"}));
    e.index_profile(make_profile("rust-expert", {"C++", "Rust"}));
    // Query for both: rust-expert should score highest (rare Rust skill + IDF)
    auto results = e.find_matches(make_query({"C++"}, {"Rust"}));
    EXPECT_EQ(results[0].profile_id, "rust-expert");
}

TEST(MatchEngineTest, MatchedSkillsListIsCorrect) {
    MatchEngine e;
    e.index_profile(make_profile("p1", {"C++", "gRPC", "Python"}));
    auto results = e.find_matches(make_query({"C++", "gRPC"}, {"Python"}));
    ASSERT_EQ(results.size(), 1u);
    const auto& matched = results[0].matched_skills;
    EXPECT_NE(std::find(matched.begin(), matched.end(), "C++"), matched.end());
    EXPECT_NE(std::find(matched.begin(), matched.end(), "gRPC"), matched.end());
}

TEST(MatchEngineTest, ScoreIsPositive) {
    MatchEngine e;
    e.index_profile(make_profile("p1", {"C++"}));
    auto results = e.find_matches(make_query({"C++"}));
    ASSERT_EQ(results.size(), 1u);
    EXPECT_GT(results[0].score, 0.0f);
}

// ── Concurrency ───────────────────────────────────────────────────────────

TEST(MatchEngineTest, ConcurrentIndexAndQueryIsThreadSafe) {
    MatchEngine e;
    // Pre-index so queries have data
    e.index_profile(make_profile("seed", {"C++"}));

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&e, i] {
            e.index_profile(make_profile("writer-" + std::to_string(i), {"Java"}));
        });
        threads.emplace_back([&e] {
            e.find_matches(make_query({"C++"}));
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_GE(e.profile_count(), 5u);  // seed + 4 writers
}
