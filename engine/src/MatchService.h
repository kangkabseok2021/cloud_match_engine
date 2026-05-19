#pragma once
#ifdef HAS_GRPC
#include <grpcpp/grpcpp.h>
#include "match.grpc.pb.h"
#include "MatchEngine.h"
#include <memory>
#include <atomic>

class MatchServiceImpl final : public match::MatchService::Service {
public:
    explicit MatchServiceImpl(std::shared_ptr<MatchEngine> engine);

    grpc::Status FindMatches(grpc::ServerContext*,
                             const match::MatchRequest*,
                             match::MatchResponse*) override;

    grpc::Status IndexProfile(grpc::ServerContext*,
                              const match::ProfileRequest*,
                              match::IndexResponse*) override;

    grpc::Status GetStats(grpc::ServerContext*,
                          const match::StatsRequest*,
                          match::StatsResponse*) override;

private:
    std::shared_ptr<MatchEngine> engine_;
    std::atomic<int64_t>         requests_{0};
};
#endif  // HAS_GRPC
