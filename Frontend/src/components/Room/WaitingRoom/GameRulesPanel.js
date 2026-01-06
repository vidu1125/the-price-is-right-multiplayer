// GameRulesPanel.js
import "./WaitingRoom.css";
import { setRules } from "../../../services/hostService";
import { useState, useEffect } from "react";

export default function GameRulesPanel({ isHost, roomId, gameRules }) {
  const [editMode, setEditMode] = useState(false);
  const [loading, setLoading] = useState(false);
  const [localRules, setLocalRules] = useState(gameRules); // ← Add local state

  // Sync local rules when prop changes
  useEffect(() => {
    setLocalRules(gameRules);
    setLoading(false);
  }, [gameRules]);

  // Send rules to server - DON'T update local state
  const handleRuleChange = async (ruleKey, value) => {
    if (!isHost || loading) return;

    let newRules = { ...localRules, [ruleKey]: value };

    // LOGIC ĐẶC BIỆT: Khi thay đổi Mode
    if (ruleKey === "mode") {
      if (value === "elimination") {
        newRules.maxPlayers = 4;
      } else if (value === "scoring") {
        if ((newRules.maxPlayers || 0) < 4) newRules.maxPlayers = 4;
        if ((newRules.maxPlayers || 0) > 6) newRules.maxPlayers = 6;
      }
    }

    // Send to server and wait for NTF_RULES_CHANGED to update UI
    try {
      setLoading(true);
      await setRules(roomId, newRules);
      console.log("✅ Rules update request sent, waiting for server confirmation");
      // UI will update when NTF_RULES_CHANGED arrives
    } catch (error) {
      console.error("❌ Failed to send rules update:", error);
      setLoading(false);
    }
  };

  // Helper để tăng/giảm số lượng người chơi
  const handleMaxPlayersChange = (delta) => {
    if (loading) return;

    const currentMode = localRules?.mode || "scoring";
    const currentMax = localRules?.maxPlayers || 5;
    const newMax = currentMax + delta;

    let min = 4;
    let max = 6;

    if (currentMode === "elimination") {
      return; // Không cho chỉnh ở eliminate mode
    }

    if (newMax >= min && newMax <= max) {
      handleRuleChange("maxPlayers", newMax);
    }
  };

  // Listen for rules-changed event to clear loading state
  useEffect(() => {
    const handleRulesChanged = () => {
      setLoading(false);
    };

    window.addEventListener('rules-changed', handleRulesChanged);
    return () => window.removeEventListener('rules-changed', handleRulesChanged);
  }, []);

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
            <button
              className="control-btn"
              onClick={() => handleMaxPlayersChange(-1)}
              disabled={localRules?.mode === "elimination" || (localRules?.maxPlayers <= 4)}
              style={{
                width: '28px', height: '28px', cursor: 'pointer',
                opacity: (localRules?.mode === "elimination" || localRules?.maxPlayers <= 4) ? 0.3 : 1
              }}
            >
              -
            </button>

            <span className="rule-value-static">
              {localRules?.maxPlayers || 5}
            </span>

            <button
              className="control-btn"
              onClick={() => handleMaxPlayersChange(1)}
              disabled={localRules?.mode === "elimination" || (localRules?.maxPlayers >= 6)}
              style={{
                width: '28px', height: '28px', cursor: 'pointer',
                opacity: (localRules?.mode === "elimination" || localRules?.maxPlayers >= 6) ? 0.3 : 1
              }}
            >
              +
            </button>
          </div>
        ) : (
          <span className="rule-value-static">{localRules?.maxPlayers || 5}</span>
        )}
      </div>

      {/* 1.5 VISIBILITY */}
      <div className="rule-group">
        <label>Visibility:</label>
        <div className="mode-options">
          <button
            className={localRules?.visibility === "public" ? "active" : ""}
            onClick={() => isHost && editMode && handleRuleChange("visibility", "public")}
            disabled={!isHost || !editMode}
          >
            Public
          </button>
          <button
            className={localRules?.visibility === "private" ? "active" : ""}
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
            className={localRules?.mode === "elimination" ? "active" : ""}
            onClick={() => isHost && editMode && handleRuleChange("mode", "elimination")}
            disabled={!isHost || !editMode}
          >
            Elimination
          </button>
          <button
            className={localRules?.mode === "scoring" ? "active" : ""}
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
            className={`status-badge on ${localRules?.wagerMode ? "active-badge" : ""}`}
            onClick={() => handleRuleChange("wagerMode", true)}
            disabled={!isHost || !editMode}
            style={{ opacity: (!isHost || !editMode) ? 0.6 : 1 }}
          >
            ON
          </button>
          <button
            className={`status-badge off ${!localRules?.wagerMode ? "active-badge" : ""}`}
            onClick={() => handleRuleChange("wagerMode", false)}
            disabled={!isHost || !editMode}
            style={{ opacity: (!isHost || !editMode) ? 0.6 : 1 }}
          >
            OFF
          </button>
        </div>
      </div>


    </div>
  );
}