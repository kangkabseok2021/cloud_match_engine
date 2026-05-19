package com.matchengine.model;

import java.util.List;

public record MatchRequestDto(
        String queryId,
        List<String> requiredSkills,
        List<String> preferredSkills,
        int maxResults
) {
    public MatchRequestDto {
        if (queryId == null || queryId.isBlank()) throw new IllegalArgumentException("queryId must not be blank");
        if (requiredSkills == null)  requiredSkills  = List.of();
        if (preferredSkills == null) preferredSkills = List.of();
        if (maxResults <= 0)         maxResults      = 10;
    }
}
