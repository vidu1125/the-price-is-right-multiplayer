import React from "react";
import { Navigate } from "react-router-dom";

export function isAuthenticated() {
  const accountId = localStorage.getItem("account_id");
  const sessionId = localStorage.getItem("session_id");
  return Boolean(accountId && sessionId);
}

export function RequireAuth({ children }) {
  return isAuthenticated() ? children : <Navigate to="/login" replace />;
}

export function RedirectIfAuthed({ children }) {
  return isAuthenticated() ? <Navigate to="/lobby" replace /> : children;
}
