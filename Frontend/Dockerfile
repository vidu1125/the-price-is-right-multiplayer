FROM node:18-alpine

WORKDIR /app
RUN apk add --no-cache python3

# Copy only necessary files for opcode generation
RUN mkdir -p /app/Network/include/protocol
COPY Network/include/protocol/opcode.h /app/Network/include/protocol/opcode.h
COPY tools /app/tools

# Create Frontend directory for opcode output
RUN mkdir -p /app/Frontend/src/network

# Generate opcode.js
RUN python3 /app/tools/gen_opcode_js.py

# Setup frontend
WORKDIR /app/Frontend

# Copy package files first for better caching
COPY Frontend/package*.json ./

# Install dependencies as node user
RUN npm install

# Copy frontend source code (excluding node_modules from host)
COPY Frontend/src ./src
COPY Frontend/public ./public

# Fix permissions for node_modules
RUN chown -R node:node /app/Frontend/node_modules

# Switch to node user
USER node

EXPOSE 3000
CMD ["npm", "start"]
