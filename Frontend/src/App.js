import { BrowserRouter as Router, Routes, Route, Navigate } from "react-router-dom";
import Lobby from "./components/Lobby/Lobby";
import WaitingRoom from "./components/Room/WaitingRoom/WaitingRoom";
import GameContainer from "./components/Game/GameContainer"; // Import mới
import Register from "./components/Auth/Register";
import Login from "./components/Auth/Login";
import "./App.css";
import React from "react";

function isAuthenticated() {
  return Boolean(localStorage.getItem("account_id"));
}

function RequireAuth({ children }) {
  return isAuthenticated() ? children : <Navigate to="/login" replace />;
}

function RedirectIfAuthed({ children }) {
  return isAuthenticated() ? <Navigate to="/lobby" replace /> : children;
}

function App() {
  const defaultRoute = isAuthenticated() ? "/lobby" : "/login";
  
  return (
    <Router>
      <div className="App">
        <Routes>
          <Route path="/" element={<Navigate to={defaultRoute} replace />} />
          <Route
            path="/login"
            element={
              <RedirectIfAuthed>
                <Login />
              </RedirectIfAuthed>
            }
          />
          <Route
            path="/register"
            element={
              <RedirectIfAuthed>
                <Register />
              </RedirectIfAuthed>
            }
          />
          <Route
            path="/lobby"
            element={
              <RequireAuth>
                <Lobby />
              </RequireAuth>
            }
          />
          <Route
            path="/waitingroom"
            element={
              <RequireAuth>
                <WaitingRoom />
              </RequireAuth>
            }
          />
          
          {/* Đường dẫn cho toàn bộ các Round của game */}
          <Route
            path="/round"
            element={
              <RequireAuth>
                <GameContainer />
              </RequireAuth>
            }
          /> 
          
        </Routes>
      </div>
    </Router>
  );
}

export default App;