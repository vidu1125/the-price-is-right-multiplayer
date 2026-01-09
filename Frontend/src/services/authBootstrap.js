import { useEffect, useRef } from "react";
import { initSocket, isConnected } from "../network/socketClient";
import { reconnectSession } from "./authService";

// Bootstraps socket connection and attempts a single reconnect using stored session_id
export function useAuthBootstrap() {
  const reconnectTried = useRef(false);
  const intervalRef = useRef(null);

  useEffect(() => {
    initSocket();

    // Only try reconnect ONCE on mount if we have a session
    intervalRef.current = setInterval(async () => {
      // Stop if auth was invalidated (401 from server)
      if (window.__authInvalid) {
        if (intervalRef.current) {
          clearInterval(intervalRef.current);
          intervalRef.current = null;
        }
        return;
      }

      // Already tried or successfully reconnected
      if (reconnectTried.current) {
        if (intervalRef.current) {
          clearInterval(intervalRef.current);
          intervalRef.current = null;
        }
        return;
      }

      const sessionId = localStorage.getItem("session_id");
      if (!sessionId) {
        reconnectTried.current = true;
        if (intervalRef.current) {
          clearInterval(intervalRef.current);
          intervalRef.current = null;
        }
        return;
      }

      if (isConnected()) {
        reconnectTried.current = true;
        try {
          const result = await reconnectSession({ sessionId });
          console.log("[AuthBootstrap] Reconnect successful, stopping interval");
        } catch (err) {
          console.warn("[AuthBootstrap] Reconnect failed", err);
        }
        
        // CRITICAL: Stop interval after attempting reconnect
        if (intervalRef.current) {
          clearInterval(intervalRef.current);
          intervalRef.current = null;
        }
      }
    }, 700);

    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
    };
  }, []);
}
