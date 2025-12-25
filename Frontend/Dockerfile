FROM node:18-alpine

WORKDIR /app
RUN apk add --no-cache python3

COPY . .

# sinh opcode
RUN mkdir -p /app/network/include/protocol
COPY Network/include/protocol/opcode.h /app/network/include/protocol/opcode.h
COPY tools /app/tools
RUN python3 /app/tools/gen_opcode_js.py

# frontend
WORKDIR /app/Frontend
RUN npm install

EXPOSE 3000
CMD ["npm", "start"]
