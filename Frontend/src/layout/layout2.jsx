import { Outlet } from "react-router-dom";

export default function WaitingRoomLayout() {
  return (
    <div
      style={{
        minHeight: "100vh",
        backgroundImage: "url('/bg/waitingroom.png')",
        backgroundSize: "cover",
        backgroundPosition: "center",
        backgroundRepeat: "no-repeat",
      }}
    >
      <Outlet />
    </div>
  );
}
