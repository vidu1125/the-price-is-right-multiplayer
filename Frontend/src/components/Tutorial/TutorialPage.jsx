import "./TutorialPage.css";
import { useNavigate } from "react-router-dom";
import { useState } from "react";

const modes = [
  {
    name: "Scoring Mode",
    players: "4 - 6 players",
    goal: "Highest total points after 3 rounds wins",
  },
  {
    name: "Elimination Mode",
    players: "Exactly 4 players",
    goal: "Survive eliminations and be the last player",
  },
];

const mechanics = [
  {
    title: "Wager x2",
    detail:
      "Activate before answering. Correct → +2x points. Wrong → -2x points (never below 0).",
  },
  {
    title: "Perfect Guess",
    detail: "Round 2 only. Exact product price → +2x that question's points.",
  },
];

const rounds = [
  {
    title: "Round 1 — Speed Challenge",
    bullets: [
      "10 multiple-choice questions about prices or combos.",
      "15 seconds each question.",
      "Score = 200 - (answer_time / 15) * 100 (floored to nearest ten).",
    ],
  },
  {
    title: "Round 2 — The Bid",
    bullets: [
      "5 products, enter your numeric price guess.",
      "15 seconds each product.",
      "100 points to the closest guess without going over.",
      "If everyone is over, 50 points to the smallest overbid.",
    ],
  },
  {
    title: "Round 3 — Bonus Wheel",
    bullets: [
      "Spin values from 5 to 100.",
      "Up to 2 spins: after the first spin, choose to stop or spin again.",
      "If total ≤ 100: you score the total. If total > 100: score total - 100.",
      "Total exactly 100 or 200: you earn 100 points.",
    ],
  },
];

const elimination = [
  "Applies only in Elimination Mode.",
  "After each main round the lowest total is eliminated.",
  "Round order: after R1 → 3 players, after R2 → 2 players, after R3 → 1 winner.",
];

const bonusDuel = [
  "Used to break ties for elimination or for the final winner.",
  "Game shows a product and a reference price.",
  "Choose HIGHER or LOWER than the real price.",
  "Fastest correct wins; any wrong answer is eliminated.",
  "Ends when only one player remains.",
];

const badges = [
  "Spin Master — reach a total spin of 100.",
  "Perfect Bidder — exact product price in Round 2.",
  "Sudden Death Winner — win a Bonus Duel.",
];

const sections = [
  { id: "mechanics", title: "Core Mechanics", type: "mechanics" },
  { id: "round1", title: "Round 1 — Speed Challenge", type: "round", index: 0 },
  { id: "round2", title: "Round 2 — The Bid", type: "round", index: 1 },
  { id: "round3", title: "Round 3 — Bonus Wheel", type: "round", index: 2 },
  { id: "elimination", title: "Elimination Flow", type: "elimination" },
  { id: "bonusduel", title: "Bonus Duel", type: "bonusduel" },
  { id: "badges", title: "Badges", type: "badges" },
];

export default function TutorialPage() {
  const navigate = useNavigate();
  const [currentSection, setCurrentSection] = useState(0);

  const handlePrev = () => {
    setCurrentSection((prev) => Math.max(0, prev - 1));
  };

  const handleNext = () => {
    setCurrentSection((prev) => Math.min(sections.length - 1, prev + 1));
  };

  const renderSection = () => {
    const section = sections[currentSection];
    
    if (section.type === "mechanics") {
      return (
        <div className="section-content">
          <div className="stack">
            {mechanics.map((m) => (
              <div className="row" key={m.title}>
                <div className="dot" />
                <div>
                  <p className="row-title">{m.title}</p>
                  <p className="row-body">{m.detail}</p>
                </div>
              </div>
            ))}
          </div>
        </div>
      );
    }
    
    if (section.type === "round") {
      const r = rounds[section.index];
      return (
        <div className="section-content">
          <div className="round-card-single">
            <span className="round-number">ROUND {section.index + 1}</span>
            <p className="round-title">{r.title.split('—')[1].trim()}</p>
            <ul>
              {r.bullets.map((b) => (
                <li key={b}>{b}</li>
              ))}
            </ul>
          </div>
        </div>
      );
    }
    
    if (section.type === "elimination") {
      return (
        <div className="section-content">
          <ul className="bullets">
            {elimination.map((item) => (
              <li key={item}>{item}</li>
            ))}
          </ul>
        </div>
      );
    }
    
    if (section.type === "bonusduel") {
      return (
        <div className="section-content">
          <ul className="bullets">
            {bonusDuel.map((item) => (
              <li key={item}>{item}</li>
            ))}
          </ul>
        </div>
      );
    }
    
    if (section.type === "badges") {
      return (
        <div className="section-content">
          <ul className="badges">
            {badges.map((item) => (
              <li key={item}>{item}</li>
            ))}
          </ul>
        </div>
      );
    }
  };

  return (
    <div className="tutorial-page">
      <div className="tutorial-hero">
        <div>
          <p className="eyebrow">Learn the rules</p>
          <h1>The Price Is Right Tutorial</h1>
          <p className="lede">
            Quick recap of modes, mechanics, and scoring so you can jump into the
            lobby with confidence.
          </p>
          <div className="hero-actions">
            <button className="primary" onClick={() => navigate("/lobby")}>Back to Lobby</button>
            <button className="ghost" onClick={() => navigate(-1)}>Go Back</button>
          </div>
        </div>
        <div className="hero-pillars">
          {modes.map((m) => (
            <div className="pill" key={m.name}>
              <span className="pill-title">{m.name}</span>
              <span className="pill-meta">{m.players}</span>
              <span className="pill-text">{m.goal}</span>
            </div>
          ))}
        </div>
      </div>

      <div className="tutorial-carousel">
        <section className="card">
          <div className="carousel-header">
            <button 
              className="nav-btn prev" 
              onClick={handlePrev}
              disabled={currentSection === 0}
            >
              ← PREV
            </button>
            <h2>{sections[currentSection].title}</h2>
            <button 
              className="nav-btn next" 
              onClick={handleNext}
              disabled={currentSection === sections.length - 1}
            >
              NEXT →
            </button>
          </div>
          
          <div className="section-indicator">
            {currentSection + 1} / {sections.length}
          </div>
          
          {renderSection()}
        </section>
      </div>
    </div>
  );
}
