FROM gcc:12

RUN apt-get update && apt-get install -y cmake make musl-dev linux-libc-dev

WORKDIR /app

COPY CMakeLists.txt ./
COPY server/ ./server/
COPY protocol/ ./protocol/

RUN mkdir -p build && cd build && cmake .. && make

EXPOSE 8888

CMD ["./build/tpir_server"]