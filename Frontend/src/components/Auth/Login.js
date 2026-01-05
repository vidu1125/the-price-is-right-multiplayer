import React, { useEffect, useMemo, useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import "./Register.css";
import { loginAccount, persistAuth } from "../../services/authService";
import { initSocket, isConnected } from "../../network/socketClient";

export default function Login() {
  const navigate = useNavigate();
  const [form, setForm] = useState({ email: "", password: "" });
  const [status, setStatus] = useState({ loading: false, error: "", success: "" });
  const [socketReady, setSocketReady] = useState(isConnected());

  const cleanEmail = useMemo(() => form.email.trim().toLowerCase(), [form.email]);

  useEffect(() => {
    initSocket();
    const interval = setInterval(() => setSocketReady(isConnected()), 800);
    return () => clearInterval(interval);
  }, []);

  const updateField = (e) => {
    const { name, value } = e.target;
    setForm((prev) => ({ ...prev, [name]: value }));
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (!cleanEmail) {
      setStatus({ loading: false, error: "Email is required", success: "" });
      return;
    }
    if (!form.password) {
      setStatus({ loading: false, error: "Password is required", success: "" });
      return;
    }

    setStatus({ loading: true, error: "", success: "" });

    try {
      const response = await loginAccount({ email: cleanEmail, password: form.password });
      if (response?.success) {
        // Persist full auth data including profile
        persistAuth(response);
        setStatus({ loading: false, error: "", success: "Welcome back!" });
        setTimeout(() => navigate("/lobby", { replace: true }), 600);
      } else {
        setStatus({ loading: false, error: response?.error || "Login failed", success: "" });
      }
    } catch (err) {
      setStatus({
        loading: false,
        error: err?.error || "Could not sign in right now",
        success: "",
      });
    }
  };

  return (
    <div className="register-shell">


      <div className="register-card">
        <div className="card-head">
          <h2>Sign In</h2>
        </div>

        <form className="form" onSubmit={handleSubmit}>
          <label className="field">
            <span>Email</span>
            <input
              name="email"
              type="email"
              value={form.email}
              onChange={updateField}
              placeholder="you@example.com"
              autoComplete="email"
              required
            />
          </label>

          <label className="field">
            <span>Password</span>
            <input
              name="password"
              type="password"
              value={form.password}
              onChange={updateField}
              placeholder="Your password"
              autoComplete="current-password"
              required
            />
          </label>

          {status.error && <div className="banner error">{status.error}</div>}
          {status.success && <div className="banner success">{status.success}</div>}

          <button type="submit" className="cta" disabled={status.loading || !socketReady}>
            {status.loading ? "Signing in..." : "Sign in"}
          </button>
        </form>

        <div className="card-foot">
          <span>No account yet?</span>
          <Link to="/register">Register here</Link>
        </div>
      </div>
    </div>
  );
}
