package com.matchengine.service;

import com.matchengine.grpc.MatchRequest;
import com.matchengine.grpc.MatchResponse;
import com.matchengine.grpc.MatchServiceGrpc;
import com.matchengine.grpc.ProfileRequest;
import com.matchengine.grpc.IndexResponse;
import com.matchengine.model.MatchRequestDto;
import com.matchengine.model.MatchResponseDto;
import com.matchengine.model.MatchResultDto;
import io.grpc.ManagedChannelBuilder;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class MatchGrpcClient {

    private final MatchServiceGrpc.MatchServiceBlockingStub stub;

    public MatchGrpcClient(
            @Value("${match.engine.host:localhost}") String host,
            @Value("${match.engine.port:50051}") int port) {
        var channel = ManagedChannelBuilder.forAddress(host, port)
                .usePlaintext()
                .build();
        this.stub = MatchServiceGrpc.newBlockingStub(channel);
    }

    public MatchResponseDto findMatches(MatchRequestDto req) {
        MatchRequest grpcReq = MatchRequest.newBuilder()
                .setQueryId(req.queryId())
                .addAllRequiredSkills(req.requiredSkills())
                .addAllPreferredSkills(req.preferredSkills())
                .setMaxResults(req.maxResults())
                .build();
        MatchResponse grpcResp = stub.findMatches(grpcReq);
        List<MatchResultDto> results = grpcResp.getResultsList().stream()
                .map(r -> new MatchResultDto(r.getProfileId(), r.getScore(), r.getMatchedSkillsList()))
                .toList();
        return new MatchResponseDto(results, grpcResp.getLatencyUs());
    }

    public long indexProfile(String profileId, List<String> skills) {
        ProfileRequest req = ProfileRequest.newBuilder()
                .setProfileId(profileId).addAllSkills(skills).build();
        IndexResponse resp = stub.indexProfile(req);
        return resp.getIndexSize();
    }
}
