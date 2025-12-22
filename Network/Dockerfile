FROM ubuntu:22.04

RUN apt update && apt install -y \
    build-essential \
    python3 \
    sqlite3 \
    libsqlite3-dev \
    netcat \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /network

COPY . .

RUN make

EXPOSE 6000

CMD ["./bin/game_server"]
