// GameRulesPanel.js
import "./WaitingRoom.css";
import { setRules } from "../../../services/hostService";
import { useState, useEffect } from "react";

export default function GameRulesPanel({ isHost, roomId, gameRules, onRulesChange }) {
  const [editMode, setEditMode] = useState(false);
  
  // Hàm xử lý thay đổi rule chung
  const handleRuleChange = async (ruleKey, value) => {
    let newRules = { ...gameRules, [ruleKey]: value };

    // LOGIC ĐẶC BIỆT: Khi thay đổi Mode
    if (ruleKey === "mode") {
      if (value === "eliminate") {
        // Nếu chọn Eliminate -> Mặc định là 4 người
        newRules.maxPlayers = 4;
      } else if (value === "scoring") {
        // Nếu chọn Scoring -> Đảm bảo nằm trong khoảng 4-6
        // Nếu đang là 4 thì giữ nguyên, nếu khác thì reset về 4 (hoặc logic khác tùy bạn)
        if ((newRules.maxPlayers || 0) < 4) newRules.maxPlayers = 4;
        if ((newRules.maxPlayers || 0) > 6) newRules.maxPlayers = 6;
      }
    }

    onRulesChange(newRules);
    
    // Send to server if host
    if (isHost) {
      try {
        await setRules(roomId, newRules);
        console.log("✅ Game rules updated", newRules);
      } catch (error) {
        console.error("❌ Failed to update rules:", error);
      }
    }
  };

  // Helper để tăng/giảm số lượng người chơi
  const handleMaxPlayersChange = (delta) => {
    const currentMode = gameRules?.mode || "scoring";
    const currentMax = gameRules?.maxPlayers || 5;
    const newMax = currentMax + delta;
    
    let min = 4;
    let max = 6;

    // Nếu là Eliminate, không cho chỉnh (hoặc logic khác nếu bạn muốn)
    // Theo yêu cầu "mặc định là 4", mình sẽ khóa cứng ở 4 cho chế độ này
    if (currentMode === "eliminate") {
       return; 
    }

    if (currentMode === "scoring") {
      min = 4;
      max = 6;
    }

    if (newMax >= min && newMax <= max) {
      handleRuleChange("maxPlayers", newMax);
    }
  };
  
  return (
    <div className="wr-panel game-rules-panel">
      <div className="panel-header-with-action">
        <h3>GAME RULES</h3>
        {isHost && (
          <button 
            className="settings-icon-btn"
            onClick={() => setEditMode(!editMode)}
          >
            {editMode ? "done" : "edit"}
          </button>
        )}
      </div>
      
      {/* 1. MAX PLAYER */}
      <div className="rule-group rule-group-row">
        <label>Max Players:</label>
        {isHost && editMode ? (
          <div className="number-control" style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
            {/* Nút Giảm */}
            <button 
              className="control-btn"
              onClick={() => handleMaxPlayersChange(-1)}
              disabled={gameRules?.mode === "eliminate" || (gameRules?.maxPlayers <= 4)}
              style={{ 
                width: '28px', height: '28px', cursor: 'pointer',
                opacity: (gameRules?.mode === "eliminate" || gameRules?.maxPlayers <= 4) ? 0.3 : 1 
              }}
            >
              -
            </button>
            
            {/* Số hiển thị - Dùng lại class rule-value-static để giữ nguyên font */}
            <span className="rule-value-static">
              {gameRules?.maxPlayers || 5}
            </span>
            
            {/* Nút Tăng */}
            <button 
              className="control-btn"
              onClick={() => handleMaxPlayersChange(1)}
              disabled={gameRules?.mode === "eliminate" || (gameRules?.maxPlayers >= 6)}
              style={{ 
                width: '28px', height: '28px', cursor: 'pointer',
                opacity: (gameRules?.mode === "eliminate" || gameRules?.maxPlayers >= 6) ? 0.3 : 1 
              }}
            >
              +
            </button>
          </div>
        ) : (
          <span className="rule-value-static">{gameRules?.maxPlayers || 5}</span>
        )}
      </div>

      {/* 1.5 VISIBILITY */}
      <div className="rule-group">
        <label>Visibility:</label>
        <div className="mode-options">
          <button 
            className={gameRules?.visibility === "public" ? "active" : ""}
            onClick={() => isHost && editMode && handleRuleChange("visibility", "public")}
            disabled={!isHost || !editMode}
          >
            Public
          </button>
          <button 
            className={gameRules?.visibility === "private" ? "active" : ""}
            onClick={() => isHost && editMode && handleRuleChange("visibility", "private")}
            disabled={!isHost || !editMode}
          >
            Private
          </button>
        </div>
      </div>

      {/* 2. MODE */}
      <div className="rule-group">
        <label>Mode:</label>
        <div className="mode-options">
          <button 
            className={gameRules?.mode === "eliminate" ? "active" : ""}
            onClick={() => isHost && editMode && handleRuleChange("mode", "eliminate")}
            disabled={!isHost || !editMode}
          >
            Eliminate
          </button>
          <button 
            className={gameRules?.mode === "scoring" ? "active" : ""}
            onClick={() => isHost && editMode && handleRuleChange("mode", "scoring")}
            disabled={!isHost || !editMode}
          >
            Scoring
          </button>
        </div>
      </div>
      
      {/* 3. WAGER MODE */}
      <div className="rule-group rule-group-row">
        <label>Wager Mode:</label>
        <div className="status-badges-container">
          <button 
            className={`status-badge on ${gameRules?.wagerMode ? "active-badge" : ""}`}
            onClick={() => handleRuleChange("wagerMode", true)}
            disabled={!isHost || !editMode}
            style={{ opacity: (!isHost || !editMode) ? 0.6 : 1 }}
          >
            ON
          </button>
          <button 
            className={`status-badge off ${!gameRules?.wagerMode ? "active-badge" : ""}`}
            onClick={() => handleRuleChange("wagerMode", false)}
            disabled={!isHost || !editMode}
            style={{ opacity: (!isHost || !editMode) ? 0.6 : 1 }}
          >
            OFF
          </button>
        </div>
      </div>

      {/* 4. ROUND TIME */}
      <div className="rule-group">
        <label>Round Time:</label>
        <div className="time-options">
          <button 
            className={gameRules?.roundTime === "slow" ? "active" : ""}
            onClick={() => isHost && editMode && handleRuleChange("roundTime", "slow")}
            disabled={!isHost || !editMode}
          >
            Slow
          </button>
          <button 
            className={gameRules?.roundTime === "normal" ? "active" : ""}
            onClick={() => isHost && editMode && handleRuleChange("roundTime", "normal")}
            disabled={!isHost || !editMode}
          >
            Normal
          </button>
          <button 
            className={gameRules?.roundTime === "fast" ? "active" : ""}
            onClick={() => isHost && editMode && handleRuleChange("roundTime", "fast")}
            disabled={!isHost || !editMode}
          >
            Fast
          </button>
        </div>
      </div>
    </div>
  );
}