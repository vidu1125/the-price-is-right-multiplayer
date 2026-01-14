import {
  BrowserRouter as Router,
  Routes,
  Route,
  Navigate,
  useLocation,
  useNavigate,
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
import FriendsManager from "./components/Friends/FriendsManager";

import LobbyLayout from "./layout/layout1";
import WaitingRoomLayout from "./layout/layout2";
import GameLayout from "./layout/layout3";

import { initSocket } from "./network/socketClient";
import MatchDetailPage from "./components/History/MatchDetailPage";
import {
  isAuthenticated,
  RequireAuth,
  RedirectIfAuthed,
} from "./services/authGuard";
import { useAuthBootstrap } from "./services/authBootstrap";

// Import game services to register NTF_ELIMINATION handler early
import "./services/gameService";

import "./App.css";

function AnimatedRoutes() {
  const location = useLocation();

  // ✅ ĐÚNG CHỖ
  useAuthBootstrap();

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
          <Route path="/friends" element={<FriendsManager />} />
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

        {/* ===== FALLBACK ===== */}
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </AnimatePresence>
  );
}

// ============================================================================
// EliminationHandler: Listen for elimination events and redirect to lobby
// ============================================================================
function EliminationHandler() {
  const navigate = useNavigate();

  useEffect(() => {
    const handleElimination = (e) => {
      const data = e.detail;
      console.log('[App] 🚫 Player eliminated:', data);
      
      // Show alert with elimination info
      alert(`You have been eliminated!\n\nReason: ${data.reason}\nFinal Score: ${data.final_score}\nRound: ${data.round}`);
      
      // Redirect to lobby
      navigate('/lobby');
    };
    
    window.addEventListener('playerEliminated', handleElimination);
    return () => window.removeEventListener('playerEliminated', handleElimination);
  }, [navigate]);

  return null; // This component doesn't render anything
}

function App() {
  useEffect(() => {
    initSocket();
  }, []);

  return (
    <Router>
      <EliminationHandler />
      <AnimatedRoutes />
    </Router>
  );
}

export default App;
