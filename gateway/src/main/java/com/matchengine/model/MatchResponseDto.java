package com.matchengine.model;

import java.util.List;

public record MatchResponseDto(
        List<MatchResultDto> results,
        long latencyUs
) {}
