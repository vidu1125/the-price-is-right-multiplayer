# ==============================
# Build stage
# ==============================
FROM gcc:13 AS builder

WORKDIR /app

# Install build dependencies with verbose output
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libpq-dev \
    postgresql-client \
    libcjson-dev && \
    rm -rf /var/lib/apt/lists/*

# Verify libpq headers are installed
RUN find /usr -name "libpq-fe.h" 2>/dev/null && echo "Found libpq-fe.h" || echo "ERROR: libpq-fe.h not found"

COPY . .

RUN make clean && make


# ==============================
# Runtime stage
# ==============================
FROM debian:bookworm-slim

WORKDIR /app

RUN apt-get update && \
    apt-get install -y \
    libpq5 \
    libcjson1 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/network_server .

EXPOSE 5500
CMD ["./network_server"]
