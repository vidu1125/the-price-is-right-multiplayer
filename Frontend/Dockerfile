FROM node:18-alpine

WORKDIR /app

# ===== System deps =====
RUN apk add --no-cache python3

# ===== Opcode generation =====
RUN mkdir -p /app/Network/include/protocol
COPY Network/include/protocol/opcode.h /app/Network/include/protocol/opcode.h
COPY tools /app/tools

RUN mkdir -p /app/Frontend/src/network
RUN python3 /app/tools/gen_opcode_js.py

# ===== Frontend =====
WORKDIR /app/Frontend

# Copy package.json FIRST (cache npm install)
COPY Frontend/package*.json ./

# Install deps (must include framer-motion in package.json)
RUN npm install

# Copy source code
COPY Frontend/src ./src
COPY Frontend/public ./public

EXPOSE 3000
CMD ["npm", "start"]
