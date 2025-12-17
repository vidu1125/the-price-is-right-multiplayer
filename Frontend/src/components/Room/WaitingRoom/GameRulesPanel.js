// GameRulesPanel.js
import "./WaitingRoom.css";

export default function GameRulesPanel({ isHost }) {
  return (
    <div className="wr-panel game-rules-panel">
      {/* Header chứa tiêu đề và nút Settings ẩn bên góc phải */}
      <div className="panel-header-with-action">
        <h3>GAME RULES</h3> {/* Viết hoa tiêu đề */}
        {isHost && (
          <button className="settings-icon-btn">
            edit
          </button>
        )}
      </div>
      
      {/* 1. MAX PLAYER */}
      <div className="rule-group rule-group-row">
        <label>Max Players:</label>
        <span className="rule-value-static">5</span>
      </div>

      {/* 2. MODE */}
      <div className="rule-group">
        <label>Mode:</label>
        <div className="mode-options">
          <button className="active">Eliminate</button>
          <button>Scoring</button>
        </div>
      </div>
      
      {/* 3. WAGER MODE */}
      <div className="rule-group rule-group-row">
        <label>Wager Mode:</label>
        <div className="status-badges-container">
          <span className="status-badge on">ON</span>
          <span className="status-badge off">OFF</span>
        </div>
      </div>

      {/* 4. ROUND TIME */}
      <div className="rule-group">
        <label>Round Time:</label>
        <div className="time-options">
          <button>Slow</button>
          <button className="active">Normal</button>
          <button>Fast</button>
        </div>
      </div>
    </div>
  );
}