import { BrowserRouter as Router, Routes, Route, Navigate } from "react-router-dom";
import Lobby from "./components/Lobby/Lobby";
import WaitingRoom from "./components/Room/WaitingRoom/WaitingRoom";
import GameContainer from "./components/Game/GameContainer"; // Import mới
import "./App.css";
import { initSocket } from "./network/socketClient";
import React, { useEffect } from "react";

function App() {
  useEffect(() => {
    initSocket(); // ✅ connect 1 lần duy nhất
  }, []);
  
  return (
    <Router>
      <div className="App">
        <Routes>
          <Route path="/" element={<Navigate to="/lobby" replace />} />
          <Route path="/lobby" element={<Lobby />} />
          <Route path="/waitingroom" element={<WaitingRoom />} />
          
          {/* Đường dẫn cho toàn bộ các Round của game */}
          <Route path="/round" element={<GameContainer />} /> 
          
        </Routes>
      </div>
    </Router>
  );
}

export default App;