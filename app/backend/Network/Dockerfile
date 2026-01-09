# ==============================
# Build stage
# ==============================
FROM gcc:13 AS builder

WORKDIR /app

RUN apt-get update && \
    apt-get install -y \
        libcurl4-openssl-dev \
        libcjson-dev && \
    rm -rf /var/lib/apt/lists/*

COPY . .

RUN make clean && make


# ==============================
# Runtime stage
# ==============================
FROM debian:bookworm-slim

WORKDIR /app

RUN apt-get update && \
    apt-get install -y \
        libcurl4 \
        libcjson1 && \
    rm -rf /var/lib/apt/lists/*

# ===== Supabase ENV =====
ENV SUPABASE_URL=https://ecnfnieobrpfbosucbdz.supabase.co
ENV SUPABASE_KEY=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImVjbmZuaWVvYnJwZmJvc3VjYmR6Iiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTc2NDY3MDE0OSwiZXhwIjoyMDgwMjQ2MTQ5fQ.GpNNqaxwqC2-dCGjiYuGb2MsplCW4esT_w35kakC5Kg

COPY --from=builder /app/network_server .

EXPOSE 5500
CMD ["./network_server"]
