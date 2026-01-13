import {
  BrowserRouter as Router,
  Routes,
  Route,
  Navigate,
  useLocation,
} from "react-router-dom";
import React, { useEffect } from "react";
import { AnimatePresence } from "framer-motion";

import Lobby from "./components/Lobby/Lobby";
import WaitingRoom from "./components/Room/WaitingRoom/WaitingRoom";
import GameContainer from "./components/Game/GameContainer";
import HistoryPage from "./components/History/historyPage";
import TutorialPage from "./components/Tutorial/TutorialPage";
import Login from "./components/Auth/Login";
import Register from "./components/Auth/Register";
import PlayerSettings from "./components/Profile/PlayerSettings";

import LobbyLayout from "./layout/layout1";
import WaitingRoomLayout from "./layout/layout2";
import GameLayout from "./layout/layout3";

import { initSocket } from "./network/socketClient";
import MatchDetailPage from "./components/History/MatchDetailPage";
import Round1TestWrapper from "./components/Game/Round1TestWrapper"; // [TEST] - Uses production Round1.js
import Round2Test from "./components/Game/Round2Test"; // [TEST]
import {
  isAuthenticated,
  RequireAuth,
  RedirectIfAuthed,
} from "./services/authGuard";
import { useAuthBootstrap } from "./services/authBootstrap";

import "./App.css";

function AnimatedRoutes() {
  const location = useLocation();

  // ✅ ĐÚNG CHỖ
  useAuthBootstrap(false);

  const defaultRoute = isAuthenticated() ? "/lobby" : "/login";

  return (
    <AnimatePresence mode="wait">
      <Routes location={location} key={location.pathname}>
        {/* ===== ROOT ===== */}
        <Route path="/" element={<Navigate to={defaultRoute} replace />} />

        {/* ===== AUTH ===== */}
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

        {/* ===== LOBBY + HISTORY (PRIVATE) ===== */}
        <Route
          element={
            <RequireAuth>
              <LobbyLayout />
            </RequireAuth>
          }
        >
          <Route path="/lobby" element={<Lobby />} />
          <Route path="/history" element={<HistoryPage />} />
          <Route path="/tutorial" element={<TutorialPage />} />
          <Route path="/match/:id" element={<MatchDetailPage />} />
          <Route path="/settings" element={<PlayerSettings />} />
        </Route>

        {/* ===== WAITING ROOM (PRIVATE) ===== */}
        <Route
          element={
            <RequireAuth>
              <WaitingRoomLayout />
            </RequireAuth>
          }
        >
          <Route path="/waitingroom" element={<WaitingRoom />} />
        </Route>

        {/* ===== GAME (PRIVATE) ===== */}
        <Route
          element={
            <RequireAuth>
              <GameLayout />
            </RequireAuth>
          }
        >
          <Route path="/round" element={<GameContainer />} />
        </Route>

        {/* ===== TEST MODE (No Auth Required) ===== */}
        <Route path="/test" element={<Round1TestWrapper />} />
        <Route path="/test-round2" element={<Round2Test />} />

        {/* ===== FALLBACK ===== */}
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </AnimatePresence>
  );
}

function App() {
  useEffect(() => {
    initSocket();
  }, []);

  return (
    <Router>
      <AnimatedRoutes />
    </Router>
  );
}

export default App;
