package com.matchengine.model;

import java.util.List;

public record MatchResultDto(
        String profileId,
        float score,
        List<String> matchedSkills
) {}
