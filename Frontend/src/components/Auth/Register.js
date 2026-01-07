import React, { useEffect, useMemo, useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import "./Register.css";
import { persistAuth, registerAccount } from "../../services/authService";
import { initSocket, isConnected } from "../../network/socketClient";

const MIN_PASSWORD = 6;

export default function Register() {
  const navigate = useNavigate();
  const [form, setForm] = useState({
    name: "",
    email: "",
    password: "",
    confirm: "",
  });
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

  const validate = () => {
    if (!cleanEmail) return "Email is required";
    if (!form.password) return "Password is required";
    if (form.password.length < MIN_PASSWORD) {
      return `Password must be at least ${MIN_PASSWORD} characters`;
    }
    if (form.password !== form.confirm) return "Passwords do not match";
    return "";
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    const validationError = validate();
    if (validationError) {
      setStatus({ loading: false, error: validationError, success: "" });
      return;
    }

    setStatus({ loading: true, error: "", success: "" });

    try {
      const response = await registerAccount({
        name: form.name.trim() || undefined,
        email: cleanEmail,
        password: form.password,
        confirm: form.confirm,
      });

      if (response?.success) {
        // Persist full auth (account, session, profile) then forward to lobby
        persistAuth(response);
        setStatus({
          loading: false,
          error: "",
          success: response.message || "Registration successful",
        });
        setTimeout(() => navigate("/lobby", { replace: true }), 900);
      } else {
        setStatus({
          loading: false,
          error: response?.error || "Registration failed",
          success: "",
        });
      }
    } catch (err) {
      setStatus({
        loading: false,
        error: err?.error || "Could not register right now",
        success: "",
      });
    }
  };

  return (
    <div className="register-shell">


      <div className="register-card">
        <div className="card-head">
          <h2>Register</h2>
        </div>

        <form className="form" onSubmit={handleSubmit}>
          <label className="field">
            <span>Display name (optional)</span>
            <input
              name="name"
              type="text"
              value={form.name}
              onChange={updateField}
              placeholder="Contestant name"
              autoComplete="nickname"
            />
          </label>

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

          <label className="field inline">
            <div>
              <span>Password</span>
              <input
                name="password"
                type="password"
                value={form.password}
                onChange={updateField}
                placeholder="At least 6 characters"
                autoComplete="new-password"
                required
              />
            </div>
            <div>
              <span>Confirm</span>
              <input
                name="confirm"
                type="password"
                value={form.confirm}
                onChange={updateField}
                placeholder="Repeat password"
                autoComplete="new-password"
                required
              />
            </div>
          </label>

          {status.error && <div className="banner error">{status.error}</div>}
          {status.success && <div className="banner success">{status.success}</div>}

          <button type="submit" className="cta" disabled={status.loading || !socketReady}>
            {status.loading ? "Registering..." : "Create account"}
          </button>
        </form>

        <div className="card-foot">
          <span>Already have an account?</span>
          <Link to="/login">Sign in here</Link>
        </div>
      </div>
    </div>
  );
}