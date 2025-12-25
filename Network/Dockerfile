# ==============================
# Build stage
# ==============================
FROM gcc:13 AS builder

WORKDIR /app

# Copy toàn bộ source
COPY . .

# Build network server
RUN make clean && make

# ==============================
# Runtime stage
# ==============================
FROM debian:bookworm-slim

WORKDIR /app

# Copy binary từ build stage
COPY --from=builder /app/network_server .

# Expose TCP port
EXPOSE 5500

# Run server
CMD ["./network_server"]
