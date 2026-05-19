# Cloud-Native In-Memory Match Engine

Distributed high-throughput matching microservice. A C++20 engine builds an inverted index over candidate profiles and returns ranked matches with microsecond latency via gRPC. A Java Spring Boot gateway exposes a RESTful API and bridges JVM clients to the native C++ service over Protobuf.

---

## Architecture

```
REST clients
     │  POST /api/v1/match   GET /api/v1/health
┌────▼────────────────────────────────────────────────────┐
│  Java Spring Boot Gateway  (port 8080)                  │
│  MatchController → MatchGrpcClient → gRPC channel       │
└────┬────────────────────────────────────────────────────┘
     │  gRPC FindMatches / IndexProfile  (port 50051)
┌────▼────────────────────────────────────────────────────┐
│  C++20 Match Engine                                     │
│  MatchServiceImpl → MatchEngine                         │
│  Inverted index:  skill  → set<profile_id>              │
│  Forward index:   profile_id → ProfileData              │
│  Ranking:         TF-IDF  score = Σ weight × IDF        │
│  Thread-safety:   std::shared_mutex (reads concurrent)  │
└─────────────────────────────────────────────────────────┘
```

---

## Ranking Algorithm

```
score(profile, query) =
    Σ  (skill ∈ required  ∩ profile.skills)  2.0 × idf[skill]
  + Σ  (skill ∈ preferred ∩ profile.skills)  1.0 × idf[skill]

idf[skill] = log(1 + N / (1 + df[skill]))   # smoothed IDF
```

Top-K selection via `std::partial_sort` — O(N log K) instead of O(N log N) when K << N.

---

## Quick Start

```bash
# Run full stack
docker compose up -d

# Index a profile
curl -X POST http://localhost:8080/api/v1/profiles?profileId=alice \
     -H "Content-Type: application/json" \
     -d '["C++","gRPC","Kubernetes"]'

# Find matches
curl -X POST http://localhost:8080/api/v1/match \
     -H "Content-Type: application/json" \
     -d '{"queryId":"q1","requiredSkills":["C++"],"preferredSkills":["gRPC"],"maxResults":5}'
```

---

## Testing

### C++ — 11 GoogleTests

```bash
cd engine && cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure -V
```

| Suite | n | Tests |
|---|---|---|
| Indexing | 2 | ProfileCountAfterIndexing, SkillCountTracksUniqueSkills |
| Matching | 3 | FindsProfileWithRequiredSkill, EmptyQueryReturnsNoResults, UnknownSkillReturnsNoResults |
| Limits | 1 | MaxResultsRespected |
| Ranking | 3 | RequiredSkillsRankedHigherThanPreferred, IDFPenalizeCommonSkill, MatchedSkillsListIsCorrect |
| Correctness | 1 | ScoreIsPositive |
| Concurrency | 1 | ConcurrentIndexAndQueryIsThreadSafe |

### Java — 4 MockMvc tests

```bash
cd gateway && mvn test -q
```

`healthEndpointReturnsOk`, `matchEndpointReturnsMockedResults`, `matchEndpointReturnsEmptyResults`, `matchEndpointRequiresQueryId` (IllegalArgumentException → 400)

---

## CI

| Job | Tests |
|---|---|
| `cpp-build-test` | cmake + 11 GoogleTests (installs grpc from apt) |
| `java-build-test` | mvn test — 4 Spring MockMvc tests |
| `docker-build` | kubectl dry-run validates K8s manifests |

---

## Project Structure

```
cloud_match_engine/
├── proto/match.proto          Protobuf schema (shared by C++ and Java)
├── engine/
│   ├── src/MatchEngine.h/cpp  Inverted index + TF-IDF ranking (C++20)
│   ├── src/MatchService.h/cpp gRPC service implementation
│   ├── src/main.cpp           gRPC server entry point
│   └── tests/                 11 GoogleTests (no gRPC needed)
├── gateway/
│   ├── src/main/java/         Spring Boot REST → gRPC client
│   └── src/test/java/         4 MockMvc tests
├── k8s/                       Kubernetes Deployment + Service manifests
├── docker-compose.yml         Local dev: match-engine + gateway
└── .github/workflows/ci.yml  3-job CI pipeline
```
