import { BrowserRouter as Router, Routes, Route, Navigate } from "react-router-dom";
import Lobby from "./components/Lobby/Lobby";
import WaitingRoom from "./components/Room/WaitingRoom/WaitingRoom";
import "./App.css";

function App() {
  return (
    <Router>
      <div className="App">
        <Routes>
          <Route path="/" element={<Navigate to="/lobby" replace />} />
          <Route path="/lobby" element={<Lobby />} />
          <Route path="/waitingroom" element={<WaitingRoom />} />
        </Routes>
      </div>
    </Router>
  );
}

export default App;