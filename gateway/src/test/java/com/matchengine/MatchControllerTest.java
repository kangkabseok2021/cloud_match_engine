package com.matchengine;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.matchengine.controller.MatchController;
import com.matchengine.model.MatchRequestDto;
import com.matchengine.model.MatchResponseDto;
import com.matchengine.model.MatchResultDto;
import com.matchengine.service.MatchGrpcClient;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.http.MediaType;
import org.springframework.test.web.servlet.MockMvc;

import java.util.List;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.*;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.*;

@WebMvcTest(MatchController.class)
class MatchControllerTest {

    @Autowired MockMvc mockMvc;
    @Autowired ObjectMapper objectMapper;
    @MockBean  MatchGrpcClient grpcClient;

    @Test
    void healthEndpointReturnsOk() throws Exception {
        mockMvc.perform(get("/api/v1/health"))
               .andExpect(status().isOk())
               .andExpect(jsonPath("$.status").value("ok"));
    }

    @Test
    void matchEndpointReturnsMockedResults() throws Exception {
        var result  = new MatchResultDto("profile-1", 0.95f, List.of("C++", "gRPC"));
        var response = new MatchResponseDto(List.of(result), 42L);
        when(grpcClient.findMatches(any())).thenReturn(response);

        var req = new MatchRequestDto("q1", List.of("C++"), List.of("gRPC"), 5);
        mockMvc.perform(post("/api/v1/match")
                       .contentType(MediaType.APPLICATION_JSON)
                       .content(objectMapper.writeValueAsString(req)))
               .andExpect(status().isOk())
               .andExpect(jsonPath("$.results[0].profileId").value("profile-1"))
               .andExpect(jsonPath("$.results[0].score").value(0.95))
               .andExpect(jsonPath("$.latencyUs").value(42));
    }

    @Test
    void matchEndpointReturnsEmptyResults() throws Exception {
        when(grpcClient.findMatches(any()))
               .thenReturn(new MatchResponseDto(List.of(), 5L));

        var req = new MatchRequestDto("q2", List.of("UnknownSkill"), List.of(), 10);
        mockMvc.perform(post("/api/v1/match")
                       .contentType(MediaType.APPLICATION_JSON)
                       .content(objectMapper.writeValueAsString(req)))
               .andExpect(status().isOk())
               .andExpect(jsonPath("$.results").isEmpty());
    }

    @Test
    void matchEndpointRequiresQueryId() throws Exception {
        // queryId is @NotBlank — missing → 400
        String badBody = """
                {"queryId":"","requiredSkills":["C++"],"preferredSkills":[],"maxResults":5}
                """;
        mockMvc.perform(post("/api/v1/match")
                       .contentType(MediaType.APPLICATION_JSON)
                       .content(badBody))
               .andExpect(status().isBadRequest());
    }
}
