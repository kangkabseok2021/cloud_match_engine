#pragma once
#include <cmath>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct ProfileData {
    std::string                      id;
    std::unordered_set<std::string>  skills;
};

struct MatchResult {
    std::string              profile_id;
    float                    score;
    std::vector<std::string> matched_skills;
};

struct MatchQuery {
    std::string              query_id;
    std::vector<std::string> required_skills;   // weight 2.0
    std::vector<std::string> preferred_skills;  // weight 1.0
    int                      max_results = 10;
};

// Thread-safe in-memory match engine.
// Inverted index:  skill  → set of profile IDs
// Forward index:   profile_id → ProfileData
// Ranking:         TF-IDF-like — Σ weight(skill) × idf(skill) for each matched skill
class MatchEngine {
public:
    // Index one profile. Recomputes IDF after every insertion.
    void index_profile(const ProfileData& profile);

    // Return top-K ranked profiles for the query. O(N log K).
    std::vector<MatchResult> find_matches(const MatchQuery& query) const;

    std::size_t profile_count()  const;
    std::size_t skill_count()    const;

private:
    mutable std::shared_mutex                                         mutex_;
    std::unordered_map<std::string, std::unordered_set<std::string>> inverted_;  // skill → profile IDs
    std::unordered_map<std::string, ProfileData>                     forward_;   // profile ID → data
    std::unordered_map<std::string, double>                          idf_;       // precomputed IDF

    void recompute_idf();

    // score = Σ weight × idf for each matched skill
    float score_profile(const ProfileData& p, const MatchQuery& q) const;
};
