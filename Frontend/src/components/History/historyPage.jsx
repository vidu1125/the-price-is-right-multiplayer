import "./historyPage.css";
import UserCard from "../Lobby/UserCard";
import AppTitle from "../Lobby/AppTitle";
import PageTransition from "../PageTransition";
const mockHistoryList = [
  { matchId: 1023, score: 120, mode: "Scoring", ranking: "1st", isWinner: true },
  { matchId: 1022, score: -40, mode: "Eliminated", ranking: "4th", isWinner: false },
  { matchId: 1021, score: 80, mode: "Scoring", ranking: "2nd", isWinner: true },
  { matchId: 1020, score: 150, mode: "Eliminated", ranking: "1st", isWinner: true },
  { matchId: 1019, score: 60, mode: "Scoring", ranking: "3rd", isWinner: true },
  { matchId: 1018, score: -20, mode: "Scoring", ranking: "5th", isWinner: false },
  { matchId: 1017, score: 200, mode: "Eliminated", ranking: "1st", isWinner: true },
];

export default function HistoryPage() {
  const MAX_MATCHES = 10;
  const displayedMatches = mockHistoryList.slice(0, MAX_MATCHES);

  // Tính toán Overall Stats
  const totalMatches = mockHistoryList.length;
  const top1Wins = mockHistoryList.filter(m => m.ranking === "1st").length;
  const totalScore = mockHistoryList.reduce((acc, curr) => acc + curr.score, 0);

  return (
    <PageTransition>
    <div className="history-page">
      {/* Giữ nguyên Style của Lobby */}
      <UserCard />
      <div className="history-title-wrapper"><AppTitle title="HISTORY" /></div>

      {/* <AppTitle title="HISTORY" /> */}
      {/* <h2 className="table-inner-title">HISTORY</h2> */}

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
            {displayedMatches.map((match) => (
              <HistoryItem key={match.matchId} match={match} />
            ))}
            
            <div className="no-more-matches">
              no more matches available
            </div>
          </div>
        </div>
      </div>
    </div>
    </PageTransition>
  );
}

function HistoryItem({ match }) {
  return (
    <div className="history-item">
      {/* Style ID giống Score */}
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
        <button className="view-btn">VIEW</button>
      </div>
    </div>
  );
}