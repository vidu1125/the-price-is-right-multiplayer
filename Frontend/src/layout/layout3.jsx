import { Outlet } from "react-router-dom";

export default function GameLayout() {
  return (
    <div
      style={{
        minHeight: "100vh",
        backgroundImage: "url('/bg/game-bg.png')",
        backgroundSize: "cover",
        backgroundPosition: "center",
        backgroundRepeat: "no-repeat",
      }}
    >
      {/* Nội dung GameContainer sẽ render ở đây */}
      <Outlet />
    </div>
  );
}
