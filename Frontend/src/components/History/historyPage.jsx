import "./historyPage.css";
import UserCard from "../Lobby/UserCard";
import AppTitle from "../Lobby/AppTitle";
import PageTransition from "../PageTransition";
import { useNavigate } from "react-router-dom";
import { useEffect, useState } from "react";
import { viewHistory, getCachedHistory } from "../../services/historyService";


// msg: mockHistoryList removed


export default function HistoryPage() {
  const navigate = useNavigate();
  // Initialize from cache if available
  const cachedData = getCachedHistory();
  const [historyList, setHistoryList] = useState(cachedData || []);
  const [loading, setLoading] = useState(!cachedData);

  useEffect(() => {
    console.log("HistoryPage mounted, fetching history...");
    let mounted = true;

    const fetchHistory = (retryCount = 0) => {
      if (!mounted) return;
      viewHistory({ limit: 10, offset: 0 })
        .then(data => {
          if (!mounted) return;
          console.log("History received:", data);
          setHistoryList(data);
          setLoading(false);
        })
        .catch(err => {
          if (!mounted) return;
          console.error("Failed to fetch history:", err);
          // Retry only if it's potentially a connection issue and we haven't retried too much
          if (retryCount < 3) {
            console.log(`Retrying history fetch (${retryCount + 1}/3)...`);
            setTimeout(() => fetchHistory(retryCount + 1), 1500);
          } else {
            setLoading(false);
          }
        });
    }

    fetchHistory();

    return () => { mounted = false; };
  }, []);

  // Calculate stats from real data
  const totalMatches = historyList.length;
  const top1Wins = historyList.filter(m => m.ranking === "1st").length;
  const totalScore = historyList.reduce((acc, curr) => acc + curr.score, 0);

  return (
    <PageTransition>
      <div className="history-page">
        {/* Giữ nguyên Style của Lobby */}
        <UserCard />
        <div className="history-title-wrapper"><AppTitle title="HISTORY" /></div>

        <div className="history-content">
          <div className="history-container">

            {/* PHẦN THỐNG KÊ OVERALL */}
            <div className="overall-stats">
              <div className="stat-box">
                <span className="stat-label">MATCHES</span>
                <span className="stat-value">{totalMatches}</span>
              </div>
              <div className="stat-box">
                <span className="stat-label">TOP 1</span>
                <span className="stat-value highlight">{top1Wins}</span>
              </div>
              <div className="stat-box">
                <span className="stat-label">Total SCORE</span>
                <span className={`stat-value ${totalScore >= 0 ? 'win-text' : 'lose-text'}`}>
                  {totalScore > 0 ? `+${totalScore}` : totalScore}
                </span>
              </div>
            </div>

            <div className="history-header">
              <span>ID</span>
              <span>MODE</span>
              <span>FINAL SCORE</span>
              <span>RANKING</span>
              <span>ACTION</span>
            </div>

            <div className="history-list-fixed">
              {loading ? (
                <div className="no-more-matches">Loading...</div>
              ) : historyList.length > 0 ? (
                historyList.map((match) => (
                  <HistoryItem key={match.matchId} match={match} />
                ))
              ) : (
                <div className="no-more-matches">No history available</div>
              )}
            </div>
            <div className="detail-footer">
              <button className="view-btn back-btn" onClick={() => navigate("/lobby")}>
                BACK TO LOBBY
              </button>
            </div>
          </div>
        </div>
      </div>
    </PageTransition>
  );
}
function HistoryItem({ match }) {
  const navigate = useNavigate();

  return (
    <div className="history-item">
      <div className="match-id-text">#{match.matchId}</div>

      <div className="mode-col">
        <span className={`mode-badge ${match.mode.toLowerCase()}`}>
          {match.mode.toUpperCase()}
        </span>
      </div>

      <div className={`score-text ${match.isWinner ? "win" : "lose"}`}>
        {match.score > 0 ? `+${match.score}` : match.score}
      </div>

      <div className="ranking-badge">{match.ranking}</div>

      <div className="action-col">
        <button
          className="view-btn"
          onClick={() => navigate(`/match/${match.matchId}`, { state: { match } })}
        >
          VIEW
        </button>
      </div>
    </div>
  );
}
