import { Outlet } from "react-router-dom";

export default function LobbyLayout() {
  return (
    <div
      style={{
        minHeight: "100vh",
        backgroundImage: "url('/bg/lobby-bg.png')",
        backgroundSize: "cover",
        backgroundPosition: "center",
      }}
    >
      <Outlet />
    </div>
  );
}
