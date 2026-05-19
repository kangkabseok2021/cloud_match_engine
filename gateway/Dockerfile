FROM maven:3.9-eclipse-temurin-21 AS builder
WORKDIR /src
COPY pom.xml .
COPY ../proto /proto
RUN mvn dependency:go-offline -q
COPY src/ src/
RUN mvn package -DskipTests -q

FROM eclipse-temurin:21-jre-jammy
RUN useradd --system --no-create-home gateway
COPY --from=builder /src/target/gateway-*.jar /app/gateway.jar
USER gateway
EXPOSE 8080
ENTRYPOINT ["java", "-jar", "/app/gateway.jar"]
