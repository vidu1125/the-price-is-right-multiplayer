import { BrowserRouter as Router, Routes, Route, Navigate } from "react-router-dom";
import React, { useEffect } from "react";

import Lobby from "./components/Lobby/Lobby";
import WaitingRoom from "./components/Room/WaitingRoom/WaitingRoom";
import GameContainer from "./components/Game/GameContainer";
import HistoryPage from "./components/History/historyPage";
import { AnimatePresence } from "framer-motion"; // Thêm dòng này
import LobbyLayout from "./layout/layout1";
import WaitingRoomLayout from "./layout/layout2";
import GameLayout from "./layout/layout3";
import {useLocation } from "react-router-dom";
import { initSocket } from "./network/socketClient";
import "./App.css";
function AnimatedRoutes() {
  const location = useLocation();

  return (
    <AnimatePresence mode="wait"> {/* mode="wait" giúp trang cũ biến mất xong trang mới mới hiện ra */}
      <Routes location={location} key={location.pathname}>
        {/* REDIRECT */}
        <Route path="/" element={<Navigate to="/lobby" replace />} />

        {/* ===== LOBBY + HISTORY ===== */}
        <Route element={<LobbyLayout />}>
          <Route path="/lobby" element={<Lobby />} />
          <Route path="/history" element={<HistoryPage />} />
        </Route>

        {/* ===== WAITING ROOM ===== */}
        <Route element={<WaitingRoomLayout />}>
          <Route path="/waitingroom" element={<WaitingRoom />} />
        </Route>

        {/* ===== GAME ===== */}
        <Route element={<GameLayout />}>
          <Route path="/round" element={<GameContainer />} />
        </Route>
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
      <AnimatedRoutes /> {/* Sử dụng component chứa hiệu ứng */}
    </Router>
  );
}

export default App;
