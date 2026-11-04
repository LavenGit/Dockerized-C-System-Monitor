FROM gcc:14-bookworm AS build
WORKDIR /src
COPY monitor.c .
RUN gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -o monitor monitor.c

FROM debian:bookworm-slim
COPY --from=build /src/monitor /usr/local/bin/monitor
USER 65534:65534
ENTRYPOINT ["/usr/local/bin/monitor"]
