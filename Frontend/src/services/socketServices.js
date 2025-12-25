// Frontend/src/services/socketService.js
/**
 * WebSocket Service
 * Manages connection to WebSocket Proxy Server
 * Proxy forwards messages to C Server via raw TCP socket
 */

class SocketService {
  constructor() {
    this.ws = null;
    this.isConnected = false;
    this.pendingRequests = new Map();
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = 5;
    this.reconnectDelay = 2000;
    this.connectionListeners = [];
  }

  /**
   * Get WebSocket URL from environment or default
   */
  getWebSocketUrl() {
    return process.env.REACT_APP_WS_URL || 'ws://localhost:8080';
  }

  /**
   * Connect to WebSocket proxy server
   */
  async connect(url = null) {
    const wsUrl = url || this.getWebSocketUrl();

    return new Promise((resolve, reject) => {
      // Already connected
      if (this.isConnected && this.ws && this.ws.readyState === WebSocket.OPEN) {
        console.log('[SocketService] Already connected');
        resolve();
        return;
      }

      // Connecting
      if (this.ws && this.ws.readyState === WebSocket.CONNECTING) {
        console.log('[SocketService] Connection in progress...');
        // Wait for existing connection
        setTimeout(() => {
          if (this.isConnected) {
            resolve();
          } else {
            reject(new Error('Connection timeout'));
          }
        }, 5000);
        return;
      }

      try {
        console.log(`[SocketService] Connecting to ${wsUrl}...`);
        this.ws = new WebSocket(wsUrl);

        this.ws.onopen = () => {
          console.log('[SocketService] ✅ Connected');
          this.isConnected = true;
          this.reconnectAttempts = 0;
          this.notifyConnectionListeners(true);
          resolve();
        };

        this.ws.onmessage = (event) => {
          this.handleMessage(event.data);
        };

        this.ws.onerror = (error) => {
          console.error('[SocketService] ❌ WebSocket error:', error);
          reject(error);
        };

        this.ws.onclose = (event) => {
          console.log(`[SocketService] Connection closed: ${event.code} ${event.reason}`);
          this.isConnected = false;
          this.notifyConnectionListeners(false);

          // Reject all pending requests
          this.pendingRequests.forEach((request, action) => {
            clearTimeout(request.timeoutId);
            request.reject(new Error('Connection closed'));
          });
          this.pendingRequests.clear();

          // Auto-reconnect if not manual close
          if (event.code !== 1000) {
            this.attemptReconnect(wsUrl);
          }
        };

      } catch (error) {
        console.error('[SocketService] Connection failed:', error);
        reject(error);
      }
    });
  }

  /**
   * Handle incoming message from server
   */
  handleMessage(data) {
    try {
      const response = JSON.parse(data);
      console.log('[SocketService] 📥 Received:', response);

      const action = response.action;
      const request = this.pendingRequests.get(action);

      if (request) {
        clearTimeout(request.timeoutId);
        this.pendingRequests.delete(action);

        if (response.success) {
          request.resolve(response.data);
        } else {
          request.reject(new Error(response.error || 'Unknown error'));
        }
      } else {
        console.warn('[SocketService] No pending request for action:', action);
      }

    } catch (error) {
      console.error('[SocketService] Failed to parse message:', error);
    }
  }

  /**
   * Send action to server
   */
  async send(action, payload = '') {
    if (!this.isConnected || !this.ws) {
      throw new Error('Not connected to server');
    }

    if (this.ws.readyState !== WebSocket.OPEN) {
      throw new Error('WebSocket not ready');
    }

    return new Promise((resolve, reject) => {
      try {
        // Create message
        const message = JSON.stringify({ action, payload });

        // Setup timeout (10 seconds)
        const timeoutId = setTimeout(() => {
          this.pendingRequests.delete(action);
          reject(new Error(`Request timeout: ${action}`));
        }, 10000);

        // Store pending request
        this.pendingRequests.set(action, {
          resolve,
          reject,
          timeoutId
        });

        // Send via WebSocket
        this.ws.send(message);
        console.log('[SocketService] 📤 Sent:', action);

      } catch (error) {
        this.pendingRequests.delete(action);
        reject(error);
      }
    });
  }

  /**
   * Attempt to reconnect
   */
  attemptReconnect(url) {
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++;
      const delay = this.reconnectDelay * this.reconnectAttempts;

      console.log(
        `[SocketService] 🔄 Reconnecting in ${delay}ms... ` +
        `(${this.reconnectAttempts}/${this.maxReconnectAttempts})`
      );

      setTimeout(() => {
        this.connect(url).catch(err => {
          console.error('[SocketService] Reconnect failed:', err);
        });
      }, delay);
    } else {
      console.error('[SocketService] ❌ Max reconnect attempts reached');
    }
  }

  /**
   * Disconnect from server
   */
  disconnect() {
    if (this.ws) {
      console.log('[SocketService] Disconnecting...');
      this.ws.close(1000, 'Client disconnect');
      this.ws = null;
      this.isConnected = false;
    }
  }

  /**
   * Get connection status
   */
  getStatus() {
    return {
      connected: this.isConnected,
      readyState: this.ws?.readyState,
      reconnectAttempts: this.reconnectAttempts,
      pendingRequests: this.pendingRequests.size
    };
  }

  /**
   * Add connection listener
   */
  onConnectionChange(callback) {
    this.connectionListeners.push(callback);
  }

  /**
   * Remove connection listener
   */
  offConnectionChange(callback) {
    this.connectionListeners = this.connectionListeners.filter(cb => cb !== callback);
  }

  /**
   * Notify connection listeners
   */
  notifyConnectionListeners(connected) {
    this.connectionListeners.forEach(callback => {
      try {
        callback(connected);
      } catch (error) {
        console.error('[SocketService] Listener error:', error);
      }
    });
  }
}

// Export singleton instance
export const socketService = new SocketService();

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

/**
 * View game history
 */
export async function viewHistory() {
  try {
    if (!socketService.isConnected) {
      console.log('[viewHistory] Connecting...');
      await socketService.connect();
    }

    const history = await socketService.send('view_history');
    console.log('[viewHistory] ✅ Success');
    return history;

  } catch (error) {
    console.error('[viewHistory] ❌ Error:', error.message);
    throw error;
  }
}

/**
 * Generic function to send any action
 */
export async function sendGameAction(action, payload = '') {
  try {
    if (!socketService.isConnected) {
      await socketService.connect();
    }

    const result = await socketService.send(action, payload);
    return result;

  } catch (error) {
    console.error(`[sendGameAction] Error with ${action}:`, error);
    throw error;
  }
}

export default socketService;