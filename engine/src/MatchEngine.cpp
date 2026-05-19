#include "MatchEngine.h"
#include <algorithm>

void MatchEngine::index_profile(const ProfileData& profile) {
    std::unique_lock lock(mutex_);
    forward_[profile.id] = profile;
    for (const auto& skill : profile.skills)
        inverted_[skill].insert(profile.id);
    recompute_idf();
}

// Smoothed IDF: log(1 + N / (1 + df))
// Smoothing prevents division by zero and reduces score of universal skills.
void MatchEngine::recompute_idf() {
    const double N = static_cast<double>(forward_.size());
    for (const auto& [skill, ids] : inverted_) {
        double df = static_cast<double>(ids.size());
        idf_[skill] = std::log1p(N / (1.0 + df));
    }
}

float MatchEngine::score_profile(const ProfileData& p, const MatchQuery& q) const {
    float s = 0.0f;
    for (const auto& sk : q.required_skills)
        if (p.skills.count(sk)) s += static_cast<float>(2.0 * idf_.at(sk));
    for (const auto& sk : q.preferred_skills)
        if (p.skills.count(sk)) s += static_cast<float>(1.0 * idf_.at(sk));
    return s;
}

std::vector<MatchResult> MatchEngine::find_matches(const MatchQuery& query) const {
    std::shared_lock lock(mutex_);

    // Candidate set: union of inverted index hits for all query skills
    std::unordered_set<std::string> candidates;
    for (const auto& sk : query.required_skills)
        if (inverted_.count(sk))
            for (const auto& id : inverted_.at(sk)) candidates.insert(id);
    for (const auto& sk : query.preferred_skills)
        if (inverted_.count(sk))
            for (const auto& id : inverted_.at(sk)) candidates.insert(id);

    std::vector<MatchResult> results;
    results.reserve(candidates.size());

    for (const auto& id : candidates) {
        const auto& profile = forward_.at(id);
        float score = score_profile(profile, query);
        if (score <= 0.0f) continue;

        std::vector<std::string> matched;
        for (const auto& sk : query.required_skills)
            if (profile.skills.count(sk)) matched.push_back(sk);
        for (const auto& sk : query.preferred_skills)
            if (profile.skills.count(sk)) matched.push_back(sk);

        results.push_back({id, score, std::move(matched)});
    }

    // Top-K via partial_sort — O(N log K) instead of O(N log N)
    const int K = std::min<int>(query.max_results, static_cast<int>(results.size()));
    std::partial_sort(results.begin(), results.begin() + K, results.end(),
        [](const MatchResult& a, const MatchResult& b) { return a.score > b.score; });
    results.resize(K);
    return results;
}

std::size_t MatchEngine::profile_count() const {
    std::shared_lock lock(mutex_);
    return forward_.size();
}

std::size_t MatchEngine::skill_count() const {
    std::shared_lock lock(mutex_);
    return inverted_.size();
}
