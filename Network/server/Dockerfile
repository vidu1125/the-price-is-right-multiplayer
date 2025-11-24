FROM gcc:12-alpine
RUN apk add --no-cache cmake make musl-dev linux-headers
WORKDIR /app
COPY . .
RUN mkdir -p build && cd build && cmake .. && make
EXPOSE 8888
CMD ["./build/tpir_server"]
