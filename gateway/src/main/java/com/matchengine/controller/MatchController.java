package com.matchengine.controller;

import com.matchengine.model.MatchRequestDto;
import com.matchengine.model.MatchResponseDto;
import com.matchengine.service.MatchGrpcClient;

import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/v1")
public class MatchController {

    private final MatchGrpcClient grpcClient;

    public MatchController(MatchGrpcClient grpcClient) {
        this.grpcClient = grpcClient;
    }

    @PostMapping("/match")
    public ResponseEntity<MatchResponseDto> findMatches(
            @RequestBody MatchRequestDto request) {
        return ResponseEntity.ok(grpcClient.findMatches(request));
    }

    @PostMapping("/profiles")
    public ResponseEntity<Map<String, Object>> indexProfile(
            @RequestParam String profileId,
            @RequestBody List<String> skills) {
        long size = grpcClient.indexProfile(profileId, skills);
        return ResponseEntity.ok(Map.of("indexSize", size));
    }

    @GetMapping("/health")
    public ResponseEntity<Map<String, String>> health() {
        return ResponseEntity.ok(Map.of("status", "ok"));
    }
}
