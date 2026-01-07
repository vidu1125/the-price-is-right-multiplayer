import { useEffect, useRef } from "react";
import { initSocket, isConnected } from "../network/socketClient";
import { reconnectSession } from "./authService";

// Bootstraps socket connection and attempts a single reconnect using stored session_id
export function useAuthBootstrap() {
  const reconnectTried = useRef(false);

  useEffect(() => {
    initSocket();

    const interval = setInterval(async () => {
      if (reconnectTried.current) return;

      const sessionId = localStorage.getItem("session_id");
      if (!sessionId) {
        reconnectTried.current = true;
        clearInterval(interval);
        return;
      }

      if (isConnected()) {
        reconnectTried.current = true;
        try {
          await reconnectSession({ sessionId });
        } catch (err) {
          console.warn("Reconnect failed", err);
        }
        clearInterval(interval);
      }
    }, 700);

    return () => clearInterval(interval);
  }, []);
}
