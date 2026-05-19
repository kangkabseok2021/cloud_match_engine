#ifdef HAS_GRPC
#include "MatchService.h"
#include <chrono>

MatchServiceImpl::MatchServiceImpl(std::shared_ptr<MatchEngine> engine)
    : engine_(std::move(engine)) {}

grpc::Status MatchServiceImpl::FindMatches(
    grpc::ServerContext*,
    const match::MatchRequest* req,
    match::MatchResponse* resp)
{
    ++requests_;
    auto t0 = std::chrono::steady_clock::now();

    MatchQuery q;
    q.query_id = req->query_id();
    for (const auto& s : req->required_skills())  q.required_skills.push_back(s);
    for (const auto& s : req->preferred_skills()) q.preferred_skills.push_back(s);
    q.max_results = req->max_results() > 0 ? req->max_results() : 10;

    auto matches = engine_->find_matches(q);

    for (const auto& m : matches) {
        auto* r = resp->add_results();
        r->set_profile_id(m.profile_id);
        r->set_score(m.score);
        for (const auto& sk : m.matched_skills) r->add_matched_skills(sk);
    }

    auto elapsed = std::chrono::steady_clock::now() - t0;
    resp->set_latency_us(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());

    return grpc::Status::OK;
}

grpc::Status MatchServiceImpl::IndexProfile(
    grpc::ServerContext*,
    const match::ProfileRequest* req,
    match::IndexResponse* resp)
{
    ProfileData p;
    p.id = req->profile_id();
    for (const auto& s : req->skills()) p.skills.insert(s);
    engine_->index_profile(p);
    resp->set_success(true);
    resp->set_index_size(static_cast<int64_t>(engine_->profile_count()));
    return grpc::Status::OK;
}

grpc::Status MatchServiceImpl::GetStats(
    grpc::ServerContext*,
    const match::StatsRequest*,
    match::StatsResponse* resp)
{
    resp->set_profile_count(static_cast<int64_t>(engine_->profile_count()));
    resp->set_unique_skills(static_cast<int64_t>(engine_->skill_count()));
    resp->set_requests_total(requests_.load());
    return grpc::Status::OK;
}
#endif  // HAS_GRPC
